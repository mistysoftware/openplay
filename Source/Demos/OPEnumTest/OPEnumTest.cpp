/*
 * Copyright (c) 1999-2004 Apple Computer, Inc. All rights reserved.
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

// this demo asks the user to pick a protocol, creates a passive(host) endpoint, and then scans the network
// for other games of this type in existence.  It shows how to present a list of games on the network to the user,
// and to pick one of those games to join.  (though here, connection attempts are always rejected)

//includes
#include "OpenPlay.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "LinkedList.h"
#include <string.h>

//typedefs
typedef struct OPEnumMessage
{
	NMLink fNext;
	NMEnumerationCommand command;
	NMEnumerationItem item;
}OPEnumMessage;

typedef class OPEnumGame *OPEnumGamePtr;

//prototypes
void ourOPCallback( PEndpointRef inEndpoint, void* inContext, NMCallbackCode inCode, NMErr inError, void* inCookie);
void  ourEnumerationCallback( void *inContext, NMEnumerationCommand inCommand, NMEnumerationItem *item );
void storeEnumMessage(NMEnumerationCommand command, NMEnumerationItem *item);
NMBoolean getEnumMessage(NMEnumerationCommand *command, NMEnumerationItem *item);

//constants
#define kOurGameType 'enmt'

//global variables
NMBoolean endpointClosed = false;
NMLIFO enumItemList;
OPEnumGamePtr enumGameList;
NMBoolean someoneTriedToConnect = false;
#ifdef OP_PLATFORM_MAC_CARBON_FLAG
	static OTClientContextPtr theOTContext;
#endif

class OPEnumGame
{
	public:
		OPEnumGame(){
						next = enumGameList;
						enumGameList = this;
					}
		~OPEnumGame(){
						OPEnumGame* temp = enumGameList;
						if (temp == this){
							enumGameList = enumGameList->next;
							return;
						}
						while (temp->next != this)
							temp = temp->next;
						temp->next = temp->next->next;
					}

		OPEnumGame *next;
		char name[32];
		NMUInt32 hostID;
};

int main(int argc, char **argv)
{
	NMUInt32 protocolType;
	PEndpointRef ourEndPoint;
	PConfigRef config;
	
	NMErr err;
		
	printf("OpenPlay Enumeration Test\n");
	printf("This app creates a passive (host) endpoint and searches the\n");
	printf("network for other endpoints of this same type.\n");
	fflush(stdout);
	
	enumItemList.Init();
	enumGameList = NULL;
	
	//on mac we use OpenTransport's interrupt-safe allocation routines, so we need to init it
	#ifdef OP_PLATFORM_MAC_CFM
		#ifdef OP_PLATFORM_MAC_CARBON_FLAG
			err = InitOpenTransportInContext(kInitOTForApplicationMask,&theOTContext);
		#else
			err = InitOpenTransport();
		#endif
		if (err){
			printf("err initing OpenTransport: %d\n",err);
			fflush(stdout);
			return 0;
		}
	#endif //OP_PLATFORM_MAC_CFM	

	//list our protocols and let the user choose one
	while (true)
	{
		NMProtocol ourProtocol;
		char choice;
		printf("choose a protocol:\n");
		fflush(stdout);
		ourProtocol.version = CURRENT_OPENPLAY_VERSION;
		int index = 0;
		err = GetIndexedProtocol(index,&ourProtocol);
		while (err == 0){
			printf("%d) %s\n",index,ourProtocol.name);
			fflush(stdout);
			err = GetIndexedProtocol(++index,&ourProtocol);
		}
		printf("<list end>\n");
		fflush(stdout);
		choice = getchar();
		getchar();//carriage return;
		choice -= '0'; //change from ascii to actual value
		err = GetIndexedProtocol(choice,&ourProtocol);
		if (err){
			printf("invalid protocol selection - try again\n");
			fflush(stdout);
		}else{
			protocolType = ourProtocol.type;
			printf("chose protocol type: '%c%c%c%c'\n",(protocolType >> 24) & 0xFF,(protocolType >> 16) & 0xFF,
				(protocolType >> 8) & 0xFF,(protocolType >> 0) & 0xFF);
			fflush(stdout);
			break;
		}
	}
	
	//make a passive endpoint(host) with the protocol type they chose
	{
		char ourGameName[32];		
		long theTime;
		time(&theTime);
		//make a custom name for our game using a prefix and the current value of time for randomness()
		sprintf(ourGameName,"CoolGame%ld",theTime & 0xFFFF);
		printf("our game name is %s\n",ourGameName);
		fflush(stdout);
		
		//create a default config for our game (dont supply a config string)
		err = ProtocolCreateConfig(protocolType, 		//which protocol to use
									kOurGameType,		//"key" to identify our games on network
									ourGameName,		//our game's name
									NULL,
									0,
									NULL,
									&config);
		if (err){
			printf("err %d on doProtocolCreateConfig\n",err);
			return 0;
		}
		
		//use our config to launch a game
		err = ProtocolOpenEndpoint(config,ourOPCallback,(void*)NULL, &ourEndPoint, (NMOpenFlags)0);
		if (err){
			printf("err %d on ProtocolOpenEndpoint\n",err);
			return 0;
		}
		
		//we need to advertise so others can see our game
		ProtocolStartAdvertising(ourEndPoint);
	}
	
	//Ok, we've got a game up and running. Now we sit back and look for other games
	//until we're quit
	err = ProtocolStartEnumeration(config,ourEnumerationCallback,NULL,true);
	if (err){
		printf("err %d on ProtocolStartEnumeration",err);
		fflush(stdout);
		return 0;
	}
	printf("Host endpoint created. Started searching for games.\n");
	fflush(stdout);
	while (true){
		printf("\ntype 'q' to quit, 'j' to join a game, 'return' to update list\n");
		printf("note: if too much time passes between list updates, enum entries might disappear due to timeout\n");
		fflush(stdout);
		char choice = getchar();
		//quit: stop enumerating, close our endpoint, kill the config, and wait till the close completes...
		if (choice == 'q'){
			printf("stopping search for games\n");
			fflush(stdout);
			ProtocolEndEnumeration(config);
			printf("closing host endpoint...\n");
			fflush(stdout);
			ProtocolCloseEndpoint(ourEndPoint,true);
			while (endpointClosed == false){}
			printf("got close-complete\n");
			fflush(stdout);
			ProtocolDisposeConfig(config);
			break;
		}
		//they wanna join a game
		else if (choice == 'j')
		{
			OPEnumGamePtr theGame = enumGameList;
			char choice;
			printf("game number to join?\n");
			fflush(stdout);
			while ((choice < '0') || (choice > '9'))
				choice = getchar();
			printf("entered %c (%d)\n",choice,choice);
			choice -= '0'; //convert from ascii to actual value
			getchar();
			while (choice > 1){
				if (theGame)
					theGame = theGame->next;
				choice--;
			}
			if (theGame){
				//we've got a game - bind to it and try connecting
				err = ProtocolBindEnumerationToConfig(config,theGame->hostID);
				if (err){
					printf("err %d binding to config\n",err);
					fflush(stdout);
					return 0;
				}
				printf("connecting to game \"%s\"...\n",theGame->name);
				fflush(stdout);
				err = ProtocolOpenEndpoint(config,ourOPCallback,(void*)NULL, &ourEndPoint, kOpenActive);
				
				if (err == kNMAcceptFailedErr)
					printf("darn. rejected again. (this is supposed to happen)\n");
				else if (err == kNMNoError)
					printf("uh..they're supposed to reject us and didn't...\n");
				else if (err == kNMOpenFailedErr){
					printf("got a kNMOpenFailedErr.\non mac this is currently returned in place of kNMAcceptFailedErr\n");
					printf("in that case, check the machine you tried to join for a connection attempt notice\n");
				}
				else
					printf("error %d on ProtocolOpenEndpoint\n",err);
			}
			else
				printf("invalid game number\n");	
			fflush(stdout);	
		}
		else
		{
			NMEnumerationCommand command;
			NMEnumerationItem item;
			//give the enumeration some processing time
			printf("calling ProtocolIdleEnumeration()\n");
			fflush(stdout);
			err = ProtocolIdleEnumeration(config);
			if (err){
				printf("error type %d on ProtocolIdleEnumeration\n",err);
				fflush(stdout);
				return 0;
			}
			printf("-----------NEW-MESSAGES--------------\n");
			fflush(stdout);
			//if someone tried to connect, make note of it
			if (someoneTriedToConnect){
				printf("*   got a connection attempt and SMACKED 'EM DOWN!!!\n");
				fflush(stdout);
				someoneTriedToConnect = false;
			}
			//process all our  enumeration messages that have been lining up
			while (getEnumMessage(&command,&item)){
				switch (command){
					case kNMEnumAdd:{
						printf("*   got a kNMEnumAdd\n");
						fflush(stdout);
						OPEnumGame *theGame = new OPEnumGame;
						strcpy(theGame->name,item.name);
						theGame->hostID = item.id;
						break;
					}
					case kNMEnumDelete:{
						printf("*   got a kNMEnumDelete\n");
						fflush(stdout);
						OPEnumGame *theGame = enumGameList;
						while (theGame){
							if (theGame->hostID == item.id){
								delete theGame;
								break;
							}
							theGame = theGame->next;
						}
						break;
					}	
					case kNMEnumClear:{
						printf("*   got a kNMEnumClear\n");
						fflush(stdout);
						OPEnumGame *theGame = enumGameList;
						while (theGame){
							delete theGame;
							theGame = enumGameList;
						}
						break;
					}
					default:
						printf("*   got an unknown enumeration message\n");
						fflush(stdout);
						break;
				}
			}
			printf("-----------CURRENT-GAMES-----------\n");
			fflush(stdout);
			{
				OPEnumGame *theGame = enumGameList;
				int count = 1;
				while (theGame){
					printf("%d) %s\n",count++,theGame->name);
					fflush(stdout);					
					theGame = theGame->next;
				}
			}
			printf("-----------------------------------\n");
			fflush(stdout);			

		}
		
	}
	printf("done. have a nice day.\n");
	fflush(stdout);
}

void ourOPCallback( PEndpointRef inEndpoint, void* inContext, NMCallbackCode inCode, NMErr inError, void* inCookie)
{
	switch (inCode){
		case kNMCloseComplete:
			endpointClosed = true;
		break;
		
		//denied!!!!!!!!!!!!!  we do make note of the attempt though.
		case kNMConnectRequest:
			someoneTriedToConnect = true;
			ProtocolRejectConnection(inEndpoint,inCookie);
		break;
		default:
		break;
	}
}

void  ourEnumerationCallback( void *inContext, NMEnumerationCommand inCommand, NMEnumerationItem *item )
{
	//we can't process this at interrupt time, so we store it for later..
	storeEnumMessage(inCommand,item);
}

//this asynchronously allocates and stores an enumeration command at interrupt-time
//to be used later at system time
void storeEnumMessage(NMEnumerationCommand command, NMEnumerationItem *item)
{
	OPEnumMessage *theMessage;
	#ifdef OP_PLATFORM_MAC_CFM
		#ifdef OP_PLATFORM_MAC_CARBON_FLAG
			theMessage = (OPEnumMessage*)OTAllocMemInContext(sizeof(OPEnumMessage),theOTContext);
		#else
			theMessage = (OPEnumMessage*)OTAllocMem(sizeof(OPEnumMessage));
		#endif
	#elif defined(OP_PLATFORM_WINDOWS)
		theMessage = (OPEnumMessage*)GlobalAlloc(GPTR,sizeof(OPEnumMessage));
	#elif defined(OP_PLATFORM_UNIX)
		theMessage = (OPEnumMessage*)malloc(sizeof(OPEnumMessage));
	#endif
	
	//fill it in and add it to the list
	theMessage->fNext.Init();
	theMessage->command = command;
	if (item)
		theMessage->item = *item;
	enumItemList.Enqueue(&theMessage->fNext);
}

//retrieves the next enumeration command that's come in
NMBoolean getEnumMessage(NMEnumerationCommand *command, NMEnumerationItem *item)
{
	static NMLink *currentList = NULL;
	
	//if theres no current list, steal the incoming LIFO and reverse it to make it a nice FIFO
	if (currentList == NULL){
		currentList = enumItemList.StealList();
		if (currentList)
			currentList = NMReverseList(currentList);
	}
	
	//now return the first thing on our current list and dispose of it
	if (currentList){
		OPEnumMessage *theMessage = NMGetLinkObject(currentList,OPEnumMessage,fNext);
		*command = theMessage->command;
		*item = theMessage->item;
		currentList = currentList->fNext;
		//dispose of our interrupt-safely allocated pointer
		#if (OP_PLATFORM_MAC_CFM)
			OTFreeMem(theMessage);
		#endif //OP_PLATFORM_MAC_CFM
		#if (OP_PLATFORM_WINDOWS)
			GlobalFree((HGLOBAL)theMessage);
		#endif //OP_PLATFORM_WINDOWS
		#if (OP_PLATFORM_UNIX)
			free(theMessage);
		#endif //OP_PLATFORM_UNIX
		return true;
	}
	return false;
}