/*
 * Copyright (c) 2002 Lane Roathe and Eric Froemlig. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * Portions Copyright (c) 1999-2004 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.1 (the "License").  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON- INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 *
 */

/* NOTES:

	- This example is designed so that all networking code is in this one file. The purpose is to make
		it easier for a programmer to study the basic use of OpenPlay's Protocol API and not to
		demonstrate good programming style or OOP practices.

	- This file is not game independent (hey, that's what OpenPlay is about!), it simply helped us keep
		the example use of OpenPlay's more managable ... we hope :)

	- This version of the network code utilizes OpenPlay's protocol api (low level functions prefixed with "Protocol")
	  It takes a bit more machinery to implement this way as opposed to the NetSprocket API; namely the ability to receive
	  network messages asynchronously. Note that this version of the app is not compatable with the NetSprocket version,
	  as NetSprocket adds message headers when sending data and whatnot.

	- This app uses datagrams exclusively. This is for simplicity, as a datagram is always received as a chunk the same size as it
	  was sent. The downside is that it is not guaranteed to arrive at the other end. This app assumes it does, and might malfunction
	  if any packets arrive out of order or are dropped. If we wanted to be more robust, we would implement a scheme to re-send
	  data if necessary. If we had used stream transfers (ProtocolSend instead of ProtocolSendPacket) we'd be sure
	  that everything would arrive in order, but we'd have to break the incoming data into packets manually, as it is not guaranteed
	  to arrive in chunks of any certain size.
*/

/* ----------------------------------------------------------- Includes */

#include <ctype.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#include "main.h"
#include "network.h"
#include "LinkedList.h"
#include "OPUtils.h"

/* ----------------------------------------------------------- Macro definitions */

enum
{
	kMessageType_Question,
	kMessageType_Answer,
	kMessageType_Name,
	kMessageType_Information
};

#define kPlayerType 1234	/* just defines a way to allow/reject players of different types */

#define kMaxMessageSize 256 /* the maximum datagram size we can send or retrieve */

#define kOurGameType 'OPe1' /* identifier for our games on the network (simply an abbreviation for OpenPlay Example 1)*/

/* ----------------------------------------------------------- Type definitions */

/* Questions are sent out in message packets from server to players */

/* NOTE	Message string is ZERO terminated!	*/
/*		This allows us to strcpy, strcmp, etc. on raw packet data	*/
/*		Meaning packet size = sizeof(packet_t) + strlen(string)	*/

typedef struct MessagePacket_t
{
	NMUInt32 	type;			/* distinguishes our info,question,and answer messages */
	char		str[1];			/* message (question, info, etc) string */

}MessagePacket_t, *MessagePacketPtr;

typedef struct MessageStorageNode
{
	NMLink 				fNext;
	PEndpointRef 		endpoint;
	void 				*context;
	void				*cookie;
	NMCallbackCode 		code;
	NMUInt32			type;		/* we use this to distinguish between info, questions, etc being sent */
	char				str[1];		/* message (question, info, etc) string */

}MessageStorageNode, *MessageStorageNodePtr;

/* used to keep track of clients connected to us if we're the host */
typedef struct clientPlayer *clientPlayerPtr;
typedef struct clientPlayer
{
	clientPlayerPtr next;
	long 			id;
	char			name[32];
	PEndpointRef 	endpoint;
	NMBoolean		closed;

}clientPlayer, *clientPlayerPtr;

/* ----------------------------------------------------------- Local Prototypes */

//static		void 		_callBack		(PEndpointRef inEndpoint, void* inContext,NMCallbackCode inCode, NMErr inError,void* inCookie );
//static 		MessageStorageNode* 	GetNextNetMessage		(void);
//static 		MessageStorageNode*  NewMessageStorageNode(long dataSize);
//static		void	DisposeMessageStorageNode(MessageStorageNode* theNode);
//NMErr 		ReplaceConfigStringValue(char *theString, char *name, char *newValue);
//void*		InterruptSafeAllocate	(long size);
//void		InterruptSafeDispose	(void *data);
NMErr 		NetworkSendName			(char *name);

/* ----------------------------------------------------------- Local Variables */

extern "C"{
	char *GameNameStr = "OpenPlay ProtocolAPI Example1"; 	/* name of this version of the app*/
}
/* Globals used to handle our interaction with OpenPlay's  API */

PEndpointRef					_ourHostEndpoint = NULL;		/* our openplay host endpoint - only used on hosting system */
PEndpointRef					_ourClientEndpoint = NULL;		/* our client endpoint - all systems will use this (even the hosting system creates a client to itself) */

static NMLIFO _messageList; 				/* our list of messages from openplay */
static clientPlayerPtr _clientList = NULL;	/* list of clients if we're host */
static int _maxPlayers;						/* the max # players for this game*/
static NMSInt32	  _playerCount;				/* current number of players in-game */
static NMUInt32   _nextPlayerID = 1;		/* we assign each player a unique id */
static NMBoolean _hostClosing = false; 		/* so we dont accidentally close our host endpoint twice */
static NMBoolean _clientClosing = false; 	/* same for our local active endpoint if we have one */
static char 	_gameName	[128];			/* name of the current hosted game */

#if TARGET_API_MAC_CARBON
	static OTClientContextPtr theOTContext; /* we use open transport for interrupt-safe memory allocation */
#endif

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
#if __MWERKS__
	#pragma mark -
	#pragma mark È Local Routines
	#pragma mark -
#endif

/* --------------------------------------------------------------------------------
	Disposes a pointer allocated using InterruptSafeAllocate

	 EXIT:	none
*/

static void _interruptSafeDispose(void *data)
{
	#if (OP_PLATFORM_MAC_CFM)
		OTFreeMem(data);
	#elif (OP_PLATFORM_WINDOWS)
		GlobalFree((HGLOBAL)data);
	#elif (OP_PLATFORM_UNIX)
		free(data);
	#endif

}

/* --------------------------------------------------------------------------------
	Allocates a pointer in a method safe to call at interrupt-time
	this code is platform dependent... doh

	 EXIT:	pointer to chunk of memory; NULL if out of memory
*/


static void * _interruptSafeAllocate(long size)
{
	#if (OP_PLATFORM_MAC_CFM)
		#if (TARGET_API_MAC_CARBON)
			return OTAllocMemInContext(size,theOTContext);
		#else
			return OTAllocMem(size);
		#endif
	#elif (OP_PLATFORM_WINDOWS)
		return GlobalAlloc(GPTR,size);
	#elif (OP_PLATFORM_UNIX)
		return malloc(size);
	#endif
}

/* --------------------------------------------------------------------------------
	Disposes a message-storage node

	 EXIT:	none
*/

static void _freeMessageStorageNode(MessageStorageNode* theNode)
{
	//_interruptSafeDispose(theNode);
}

/* --------------------------------------------------------------------------------
	Allocate a message storage node in an interrupt-safe manner and add it to the list
	
	EXIT:	returns a new message node with extra <dataSize> bytes at end
*/

static MessageStorageNode*  _newMessageStorageNode(long dataSize)
{
	MessageStorageNode* theNode;
	
	/*we need the size of the node struct plus whatever extra data they wanna stick in it*/
	dataSize += sizeof(*theNode);
	
	theNode = (MessageStorageNode*)_interruptSafeAllocate(dataSize);
	
	theNode->fNext.Init();

	return theNode;
}

/* --------------------------------------------------------------------------------
	openplay calls this function to inform us of new join request, incoming data, etc
	this code must be thread/interrupt safe.  In this example, we simply store all our messages in an interrupt-safe list
	and look at them at our leisure at system time (except for a select few actions we take
	immediately - responding to join requests and whatnot - thought we theoretically could wait and do that at system time too)
	
	 EXIT:	none
*/


static void _callBack( PEndpointRef inEndpoint, void* inContext,NMCallbackCode inCode, NMErr inError,void* inCookie )
{
	/*we have to respond to connect requests immediately, because when we connect to ourself, we'll be in a
	ProtocolOpenEndpoint() call and won't be able to call our message checking function*/
	if (inCode == kNMConnectRequest)
	{
		/*this is a tiny bit sloppy to be comparing values at interrupt time - we should use some sort of mutual-exclusion setup
		so we can't have two threads manipulating these values simultaneously.  Anyway, for our simple purposes this is fine*/
		if (_playerCount < _maxPlayers)
		{
			clientPlayer *thePlayer;
		
			/*create a player struct and pass it as our context for this new endpoint 
			we dont', however, add it to our list yet - we gotta wait for our accept complete
			(we shouldnt at interrupt time anyway)*/
			thePlayer = (clientPlayer*)_interruptSafeAllocate(sizeof(*thePlayer));
			strcpy(thePlayer->name,"mr. untitled"); /*until we get a name message from the guy, we hereby dub him mr. untitled*/
			ProtocolAcceptConnection(inEndpoint,inCookie,_callBack,thePlayer);
			_playerCount++;
		}
		else
			/* Rejected!!! */
			ProtocolRejectConnection(inEndpoint,inCookie);
	}
	else
	{
		MessageStorageNode *theStorageNode;
				
		/*theres one or more datagrams waiting*/
		if (inCode == kNMDatagramData){
			NMUInt32 dataLength = kMaxMessageSize;
			NMFlags flags = 0;
			char buffer[kMaxMessageSize];
			NMErr err;
			
			/* receive packets until there are no more to be found */
			err = ProtocolReceivePacket(inEndpoint,(void*)buffer,&dataLength,&flags);
			while (!err)
			{
				/* we got a packet - make a node big enough to hold it, save all the message info,
				and copy the data we received (in this case just a string) to it */
				theStorageNode = _newMessageStorageNode(dataLength);
				theStorageNode->endpoint = inEndpoint;
				theStorageNode->context = inContext;
				theStorageNode->cookie = inCookie;
				theStorageNode->code = inCode;
				theStorageNode->type = ((MessagePacketPtr)buffer)->type;
			
				/* we just pulled a 4 byte integer off the network - if we're on a little-endian system (x86,etc)
				we need to swap it out of big-endian format */
				#if (little_endian)
					theStorageNode->type = SWAP4(theStorageNode->type);
				#endif
				
				strcpy(theStorageNode->str,((MessagePacketPtr)buffer)->str);
			
				/*add it to our interrupt-safe list and try for the next one*/
				_messageList.Enqueue(&theStorageNode->fNext);	
				
				err = ProtocolReceivePacket(inEndpoint,buffer,&dataLength,&flags);					
			}
		}
		/*its some other message - make a storage node without any extra space for data and copy the info in */
		else
		{
			theStorageNode = _newMessageStorageNode(0);
			theStorageNode->endpoint = inEndpoint;
			theStorageNode->context = inContext;
			theStorageNode->cookie = inCookie;
			theStorageNode->code = inCode;

			/*add it to our interrupt-safe list*/
			_messageList.Enqueue(&theStorageNode->fNext);	
			
		}
	}
}

/* --------------------------------------------------------------------------------
	Pulls the next node off our interrupt-safe incoming message list

	 EXIT:	next available message, NULL if none available
*/

static MessageStorageNode* _getNextNetMessage(void)
{
	static NMLink *currentList = NULL;
	
	/*if theres nothing in our "current" list, steal the incoming list (which is a LIFO)
	and reverse it to make a nice FIFO */
	if (currentList == NULL)
	{
		currentList = _messageList.StealList();
		if (currentList)
			currentList = NMReverseList(currentList);
	}
	
	/*if theres still no list, we return empty-handed*/
	if (!currentList)
		return NULL;

	/*now grab the first item off our current list and return it*/
	else
	{
		MessageStorageNode *theNode = NMGetLinkObject(currentList,MessageStorageNode,fNext);
		currentList = currentList->fNext;
		return theNode;
	}
}

/* --------------------------------------------------------------------------------
	Finds the member <name> in the config string <theString> and replaces its value with <newValue>

	 EXIT:	0 == no error, else error code
*/

static NMErr _replaceConfigStringValue(char *theString, char *name, char *newValue)
{
	char buffer[256];
	char *nameStart;
	
	/*copy all to buffer first*/
	strcpy(buffer,theString);
	
	nameStart = strstr(buffer, name);
	if (nameStart == NULL)
		return kNMInvalidConfigErr;
		
	/*replace first char of namestart with a term char so the first part is its own string*/
	*nameStart = 0;
	nameStart++;
	
	/*now move past the current contents till we hit a space, tab, or end*/
	while ((*nameStart != ' ') && (*nameStart != '\n') && (*nameStart != '\r') && (*nameStart != 0) && (*nameStart != 0x09)){
		nameStart++;
	}
	
	/*print it back to the original*/	
	sprintf(theString,"%s%s=%s%s",buffer,name,newValue,nameStart);
	return kNMNoError;
}

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
#if __MWERKS__
	#pragma mark -
	#pragma mark È Global Routines
	#pragma mark -
#endif

/* ================================================================================
	Handle all of the network messages that were sent to this application at system time

	 EXIT:	0 == no error, else error code
*/

void NetworkHandleMessage( void )
{
	MessageStorageNode *theMessage;
	
	/*allow openplay some idle time.  we could check the protocol to see if it actually
	needs us to provide it idle time, but it never hurts to do so anyway
	we only idle one endpoint - this suffices.  Hopefully the API will be modified so
	we don't need to pass an endpoint.  We also check _host/_clientClosing, as Idling
	an endpoint that ProtocolCloseEndpoint has been called on is not safe, since
	 the endpoint might not exist anymore.  This is another
	good argument for a non-endpoint-specific function, as some modules may need idle time when
	shutting their endpoints down.  ok enough ranting.*/
	if ((_ourHostEndpoint) && (!_hostClosing))
		ProtocolIdle(_ourHostEndpoint);
	else if ((_ourClientEndpoint) && (!_clientClosing))
		ProtocolIdle(_ourClientEndpoint);
	
	theMessage = _getNextNetMessage();
	while (theMessage)
	{
		/*sort by openplay code*/
		switch (theMessage->code)
		{
			/* a new datagram has come in */
			case kNMDatagramData:
			{			
				char str[255];
				
				/* sort by our custom message types */
				switch (theMessage->type)
				{
					case kMessageType_Name:
					{
						clientPlayer *thePlayer = (clientPlayer*)theMessage->context;
						strcpy(thePlayer->name,theMessage->str);
						printf("===> Player %ld's name is %s\n",thePlayer->id,thePlayer->name);
						fflush(stdout);
						break;
					}
					case kMessageType_Answer:
					{
						clientPlayer *thePlayer = (clientPlayer*)theMessage->context;
						sprintf( str, "Player #%ld(%s) answered with '%c', which is %s", thePlayer->id,thePlayer->name, theMessage->str[0],
							GameCheckAnswer( theMessage->str[0] ) ? "Correct!" : "WRONG!" );
						NetworkSendInformation( str );	
						break;			
					}
					case kMessageType_Question:
						GameDisplayQuestion( theMessage->str );
						break;
						
					case kMessageType_Information:
						printf( "%s\n\n", theMessage->str );
						fflush(stdout);
						break;

					default:
						break;
				}
				break;
			}
			
			/* new stream data has arrived.  We don't use streams in this example, just datagrams.
			(see top of this file for further explanation) */
			case kNMStreamData:
			{
				break;
			}
			/* if we had received a flow control error when sending data
			it means that we've stuffed the network with all the data it can take at the moment,
			and this message will be sent to us once its ready to send data again */ 
			case kNMFlowClear:
			{
				break;
			}
			
			/* this is sent to a passive(host) endpoint when it has successfully accepted a connection and "handed it off" to
			a new client endpoint */
			case kNMAcceptComplete:
			{
				clientPlayer *thePlayer = (clientPlayer*)theMessage->context;
				
				/* we're ready to roll - add our endpoint to the list of live clients */
				thePlayer->next = _clientList;
				_clientList = thePlayer;
				thePlayer->id = _nextPlayerID++; /*assign it the next available id */
				thePlayer->endpoint = theMessage->endpoint;
				thePlayer->closed = false;
				
				/*announce the arrival - we don't have a name for the guy until his name message arrives though*/
				printf("===> Player %ld joined game '%s', %ld players now!\n",thePlayer->id,_gameName,NetworkGetPlayerCount());
				fflush(stdout);
				
				/*Lets go ahead and re-send the current question, as the new guy deserves a shot at it too*/
				{
					QuestionPtr theQuestion = GameGetCurrentQuestion();
					if (theQuestion)
						NetworkSendQuestion(theQuestion->question);
					else
					{
						printf("NULL current question!");
						fflush(stdout);
					}
				}
			}
			
			/* this message is sent to a passive(host) endpoint's spawned client endpoint when it is fully formed and ready to roll */
			case kNMHandoffComplete:
			{
				break;
			}
			
			/*Connection has been lost.  When you get an endpoint died message, you still should close the endpoint*/
			case kNMEndpointDied:
			{
				clientPlayer *thePlayer = (clientPlayer*)theMessage->context;
			
				/*if our client connection died, we just kill the game*/
				if (thePlayer == (clientPlayer*)-1)
				{
					printf("===> Connection to server lost\n");
					fflush(stdout);
					GameOver();
					break;
				}
				
				/*otherwise its a client disconnecting from us*/
				/* inform of their departure */
				_playerCount--;
				printf("===> Player %ld left game '%s', %ld players now!\n",thePlayer->id,_gameName,NetworkGetPlayerCount());
				fflush(stdout);

				/* go ahead and close it, and mark it as closing so we don't send it any more datagrams - we leave it on the list
				though, until we get its close-complete message */
				if (thePlayer == NULL) /*its our host endpoint */
				{
					if (!_hostClosing)
						ProtocolCloseEndpoint(_ourHostEndpoint,true); /*specifiy an "orderly" disconnect*/
					_hostClosing = true;
				}
				else if (thePlayer == (clientPlayer*)-1) /*its our client(active) endpoint*/
				{
					if (!_clientClosing)
						ProtocolCloseEndpoint(_ourClientEndpoint,true); /*specifiy an "orderly" disconnect*/
					_clientClosing = true;
				}
				else /* its someone connected to our host endpoint */
				{
					if (thePlayer->closed == false)
						ProtocolCloseEndpoint(thePlayer->endpoint, true); /* specifies an "orderly" disconnect */
					thePlayer->closed = true;
				}
				break;
			}
			
			/* Immediately after you close an endpoint, you still may receive messages to it (as that happens asynchronously)
			the only way to be sure its done sending you messages is to wait for this message to come in from it
			you may also call ProtocolIsAlive() to determine if the endpoint is still in existence.*/ 
			case kNMCloseComplete:
			{
				clientPlayer *thePlayer = (clientPlayer*)theMessage->context;
				
				/* we can remove them from our list or NULL them out now, as they're truly dead.*/
				
				if (thePlayer == NULL) /*its our host endpoint*/
					_ourHostEndpoint = NULL;
				else if (thePlayer == (clientPlayer*)-1) /*its our active(client) endpoint*/
					_ourClientEndpoint = NULL; 
				else
				{
					if (thePlayer == _clientList)
						_clientList = thePlayer->next;
					else
					{
						clientPlayer *iterator = _clientList;
						while (iterator)
						{
							if (iterator->next == thePlayer)
							{
								iterator->next = thePlayer->next;						
								break;
							}
							iterator = iterator->next;
						}
					}
					_interruptSafeDispose(thePlayer);				
				}
				break;
			}
			default:
				break;
		}
		/* trash this message and grab the next */
		_freeMessageStorageNode(theMessage);
		theMessage = _getNextNetMessage();
	}
}

/* ================================================================================
	Return the # of players in the current game
	Note: in this version of the code, this function only works for the server,
	as we keep track of the player count manually when new connections are made to us.
	We could define a method for clients to ask the server for the current player count, but
	we don't need to for this app
	 EXIT:	none
*/

NMUInt32 NetworkGetPlayerCount( void )
{
	return _playerCount;
}

/* ================================================================================
	Send an answer to the server

	 EXIT:	Error code
*/

NMErr NetworkSendAnswer
(
	char answer			/* the answer to send (just a char!) */
)
{
	NMErr err = kNMNoError;
	MessagePacket_t theMessage;
	
	theMessage.type = kMessageType_Answer;
	theMessage.str[0] = answer;
	
	/* on little_endian systems (x86,etc) we need to swap our 4 byte type value into network(big-endian) format */
	#if (little_endian)
		theMessage.type = SWAP4(theMessage.type);
	#endif
	
	/* To send messages to the host, we just use our local client endpoint */
	err = ProtocolSendPacket(	_ourClientEndpoint, /* which endpoint */
								&theMessage,		/* data */
								sizeof(theMessage),	/* data size */
								0);					/* flags */
	return err;
}

/* ================================================================================
	Send our name to the server

	 EXIT:	Error code
*/

NMErr NetworkSendName
(
	char *name			/* the name to send */
)
{
	char buffer[256];
	MessagePacketPtr theMessage = (MessagePacketPtr)buffer;
	NMErr err = kNMNoError;

	strcpy(theMessage->str,name);
	theMessage->type = kMessageType_Name;
	
	/* on little_endian systems (x86,etc) we need to swap our 4 byte type value into network(big-endian) format */
	#if (little_endian)
		theMessage->type = SWAP4(theMessage->type);
	#endif	
	
	err = ProtocolSendPacket(	_ourClientEndpoint, 							/* which endpoint */
								theMessage,										/* data */
								sizeof(*theMessage) + strlen(theMessage->str),	/* data size */
								0);												/* flags */
	
	return err;
}

/* ================================================================================
	Send a message to all players

	 EXIT:	none
*/

NMErr NetworkSendPlayerMessage
(
	const char *message,		/* ptr to message string to send */
	NMUInt32 messageType		/* type of message (question, info, etc. */
)
{
	char buffer[256];
	MessagePacketPtr theMessage = (MessagePacketPtr)buffer;
	clientPlayer *theClient;
	NMErr err = kNMNoError;

	strcpy(theMessage->str,message);
	theMessage->type = messageType;
	
	/* on little_endian systems (x86,etc) we need to swap our 4 byte type value into network(big-endian) format */
	#if (little_endian)
		theMessage->type = SWAP4(theMessage->type);
	#endif	
	
	/* As a host, to send a message to all players, we just go through our list of client endpoints
	and send to each of them */
	theClient = _clientList;
	while (theClient != NULL)
	{
		/* If we've closed a client and are waiting for its close-complete, we don't send stuff to it*/
		if (theClient->closed == false)
		{
			//printf("sending packat to %p\n",theClient->endpoint);
			err = ProtocolSendPacket(	theClient->endpoint, 							/* which endpoint */
										theMessage,										/* data */
										sizeof(*theMessage) + strlen(theMessage->str),	/* data size */
										0);												/* flags */
		}
		theClient = theClient->next;
	}
	
	return err;
}

/* ================================================================================
	Send a question to all players

	 EXIT:	none
*/

NMErr NetworkSendQuestion
(
	const char *question		/* ptr to question string to send */
)
{
	return( NetworkSendPlayerMessage( question, kMessageType_Question ) );
}

/* ================================================================================
	Send information to all players

	 EXIT:	none
*/

NMErr NetworkSendInformation
(
	const char *message		/* ptr to info string to send */
)
{
	return( NetworkSendPlayerMessage( message, kMessageType_Information ) );
}

#if __MWERKS__
	#pragma mark -
#endif

/* ================================================================================
	Initialize server networking

	 EXIT:	0 == no error, else error code
*/

NMErr NetworkStartServer
(
	NMUInt16 port,					/* port clients will connect on */
	NMUInt32 maxPlayers,			/* max # of players to allow into game */
	const unsigned char *gameNameIn,	/* name of game (displayed to clients on connect) */
	const unsigned char *playerNameIn	/* name of player on server computer */
)
{
	char playerName[128];
	PConfigRef config;
	NMErr err = kNMNoError;
	char configString[255];
	
	/*first off, we want our game name and player name in c string format*/
	GameP2CStr((NMUInt8*)gameNameIn,_gameName);
	GameP2CStr((NMUInt8*)playerNameIn,playerName);
	
	/* save our max number of players ourself, since we have to handle that manually */
	_maxPlayers = maxPlayers;
	
	/* make a default configuration */
	err = ProtocolCreateConfig( 'Inet',  		/* we wanna use the TCPIP module */
								kOurGameType, 	/* identifier for our games on the network */
								_gameName,
								NULL,			/* custom data for enumeration callbacks - don't need it */
								0,				/* length of aforementioned data */
								NULL,			/* config string - we just want default values */
								&config);		/* we get our config here	*/

	/* ok, we've got a default tcpip configuration, but we wanna change the port number.
	to do that, we have to get the config's config-string, find the part that designates port number,
	change it to what we want, and remake the config.  We could have just saved a predefined config string
	and used that as a base, but this is safer, as it preserves any new config items that may
	be added in later as defaults */
	if (!err)
	{
		err = ProtocolGetConfigString(config,configString,sizeof(configString));
		ProtocolDisposeConfig(config); /* won't be needing that any more */
	}
	if (!err)
	{
		char portString[20];
		sprintf(portString,"%d",port);
		err = _replaceConfigStringValue(configString,"IPport",portString);
	}
	
	fflush(stdout);

	/* remake our config - this time passing in our shiny new config string*/
	if (!err)
	{
		err = ProtocolCreateConfig( 'Inet',  		/* we wanna use the TCPIP module */
									kOurGameType, 	/* identifier for our games on the network */
									(char*)_gameName,
									NULL,			/* custom data for enumeration callbacks - don't need it */
									0,				/* length of aforementioned data */
									configString,	/* we have a config string this time */
									&config);		/* we get our config here	*/
	}
	
	/* ok we're set - launch a game with our config */
	if (!err)
	{
		err = ProtocolOpenEndpoint(	config,					/* our config */
									_callBack,  	/* our callback function*/
									(void*)NULL,			/* context - we use NULL to specify our server endpoint */
									&_ourHostEndpoint,		/* returns the endpoint */
									(NMOpenFlags)0);		/* flags - passing kOpenActive denotes an active(client) connection but we want a passive(host) so we pass none */
	
		/* done with the config */
		ProtocolDisposeConfig(config);
	}
	
	/*now, if there was no error, we create a client to ourself*/
	if (!err)
	{
		char portString[10];
		sprintf(portString,"%d",port);
		err = NetworkStartClient("127.0.0.1",portString,playerNameIn);
	}
	
	return err;
}


/* ================================================================================
	Shutdown the networking, release resources, etc.

	 EXIT:	0 == no error, else error code
*/

NMErr NetworkStartClient
(
	char *ipAddr,					/* IP address (or domain name) to look for server (host) on */
	char *port,						/* Port to talk to server via */
	const unsigned char *playerNameIn	/* name of player wanting to join */
)
{
	char playerName[128];
	PConfigRef config;
	NMErr err = kNMNoError;
	char configString[255];
		
	/*first off, we want our player name in c string format*/
	GameP2CStr((NMUInt8*)playerNameIn,playerName);
		
	/* make a default configuration */
	err = ProtocolCreateConfig( 'Inet',  		/* we wanna use the TCPIP module */
								kOurGameType, 	/* identifier for our games on the network */
								"dummyName",	/* game name not used when joining (we join by ip address)*/
								NULL,			/* custom data for enumeration callbacks - don't need it */
								0,				/* length of aforementioned data */
								NULL,			/* config string - we just want default values */
								&config);		/* we get our config here	*/

	/* ok, we've got a default tcpip configuration, but we wanna change the port number and ipaddress.
	to do that, we have to get the config's config-string, find the parts that designate port number/ip address,
	change them to what we want, and remake the config.  We could have just saved a predefined config string
	and used that as a base, but this is safer, as it preserves any new config items that may
	be added in later as defaults */
	if (!err)
	{
		err = ProtocolGetConfigString(config,configString,sizeof(configString));
		ProtocolDisposeConfig(config); /* won't be needing that any more */
	}
	if (!err)
		err = _replaceConfigStringValue(configString,"IPport",port);
	if (!err)
		err = _replaceConfigStringValue(configString,"IPaddr",ipAddr);
		
	/* remake our config - this time passing in our shiny new config string*/
	if (!err)
	{
		err = ProtocolCreateConfig( 'Inet',  		/* we wanna use the TCPIP module */
									kOurGameType, 	/* identifier for our games on the network */
									"dummyName",	/* game name not used here - we join by IP address */
									NULL,			/* custom data for enumeration callbacks - don't need it */
									0,				/* length of aforementioned data */
									configString,	/* we have a config string this time */
									&config);		/* we get our config here	*/
	}
	
	/* ok we're set - launch a game with our config */
	if (!err)
	{
		printf("Attempting to join game at %s:%s\n",ipAddr,port);
		fflush(stdout);
		err = ProtocolOpenEndpoint(	config,				/* our config */
									_callBack,  /* our callback function*/
									(void*)-1,				/* context - we use -1 to specify our local client endpoint */
									&_ourClientEndpoint,	/* returns the endpoint */
									kOpenActive);		/* flags - passing kOpenActive denotes an active(client) connection, which we want */
	

		/* done with the config */
		ProtocolDisposeConfig(config);
	}
	
	/*if we connected, the first thing we do is send our name.  we're more than just a number!! */
	if (!err)
	{
		NetworkSendName(playerName);
	}
	
	return err;
}


#if __MWERKS__
	#pragma mark -
#endif

/* ================================================================================
	Shutdown the networking, release resources, etc.

	 EXIT:	0 == no error, else error code
*/

NMErr NetworkShutdown( void )
{
	NMErr err = kNMNoError;
	
	/*if we have a client connection to someone, close it*/
	if (_ourClientEndpoint)
	{
		if (!_clientClosing) /*so we don't somehow close twice*/
			ProtocolCloseEndpoint(_ourClientEndpoint,true); /*specify an orderly disconnect*/
		_clientClosing = true; 
	}
	
	/*if we're the server, close all our endpoints and then ourself
	we could probably get away with just closing ourself, but just to be safe...*/
	if (_ourHostEndpoint)
	{
		clientPlayer *thePlayer = _clientList;
		while (thePlayer)
		{
			ProtocolCloseEndpoint(thePlayer->endpoint,true);/*specifies an "orderly" disconnect*/
			thePlayer = thePlayer->next;
		}
		if (!_hostClosing) /*so we don't somehow close twice*/
			ProtocolCloseEndpoint(_ourHostEndpoint,true); /*specifies an "orderly" disconnect*/
		_hostClosing = true; 
	}
	
	/* Here we wait for all of our endpoints to finish closing. Otherwise our callback might be called while our app is shutting down*/
	
	printf("\n-closing all endpoints-\n");
	fflush(stdout);
	{
		while ((_clientList) || _ourHostEndpoint || _ourClientEndpoint)
		{
			NetworkHandleMessage();
		}
	}
	printf("-done-\n");
	fflush(stdout);
	
	#if (OP_PLATFORM_MAC_CFM)
		#if TARGET_API_MAC_CARBON
			CloseOpenTransportInContext(theOTContext);
		#else
			CloseOpenTransport();
		#endif
	#endif
	
	return err;
}

/* ================================================================================
	Startup the networking system (OpenPlay in this case)

	 EXIT:	0 == no error, else error code
*/

NMErr NetworkStartup( void )
{
	NMErr err = kNMNoError;
	
	_playerCount = 0;
	_nextPlayerID = 1;
	
	/*on mac, we gotta set up OpenTransport, which we use for interrupt-safe allocations (since OpenPlay's messages come in at interrupt time)*/
	#if (OP_PLATFORM_MAC_CFM)
		#if (TARGET_API_MAC_CARBON)
			err = InitOpenTransportInContext(kInitOTForApplicationMask,&theOTContext);
		#else
			err = InitOpenTransport();
		#endif
	#endif
	
	return err;
}
