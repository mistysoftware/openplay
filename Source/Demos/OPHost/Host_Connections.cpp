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


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "String_Utils.h"

#include "OpenPlay.h"

#include "LinkedList.h"
#include "OPUtils.h"

#include "Host_Connections.h"

#define kStorageNodeDataSize 10000

typedef struct ClientConnection
{
	char prevReceivalEnd[4];
	NMBoolean skipHeader;
	NMLink *currentList;
	NMLIFO messageList;
	ClientConnectionStatus currentStatus;
	PEndpointRef theEndpoint;
	char		*currentBufferBase;
	char		*currentBuffer;
	NMUInt32	currentBufferSize;
	NMBoolean announcedTransferring;
}ClientConnection;

typedef struct MessageStorageNode
{
	NMBoolean complete;
	NMLink fNext;
	ClientConnection		*download;
	void				*cookie;
	NMCallbackCode 		code;
	char				*buffer;
	NMUInt32			bufferSize;
}MessageStorageNode, *MessageStorageNodePtr;


/* we use open transport for interrupt-safe memory allocation */
#if TARGET_API_MAC_CARBON
	static OTClientContextPtr 	_myOTContext;
#endif

/* ================================================================================
	Disposes a message-storage node

	 EXIT:	none
*/
static void*	_interruptSafeAllocate(long size)
{
	
	void *chunk;

	#if (OP_PLATFORM_MAC_CFM)
		#if (TARGET_API_MAC_CARBON)
			chunk = OTAllocMemInContext(size,_myOTContext);
		#else
			chunk = OTAllocMem(size);
		#endif
	#elif (OP_PLATFORM_WINDOWS)
		chunk = GlobalAlloc(GPTR,size);
	#elif (OP_PLATFORM_UNIX)
		chunk =  malloc(size);
	#endif
	
	return chunk;
}

/* ================================================================================
	Disposes a message-storage node

	 EXIT:	none
*/
static void	_interruptSafeDispose(void *data)
{
	#if (OP_PLATFORM_MAC_CFM)
		OTFreeMem(data);
	#elif (OP_PLATFORM_WINDOWS)
		GlobalFree((HGLOBAL)data);
	#elif (OP_PLATFORM_UNIX)
		free(data);
	#endif

}

/* ================================================================================
	Disposes a message-storage node

	 EXIT:	none
*/
static MessageStorageNode*  _newMessageStorageNode(ClientConnection *theDL, NMUInt32 bufferSize)
{
	MessageStorageNode* theNode;
		
	theNode = (MessageStorageNode*)_interruptSafeAllocate(sizeof(MessageStorageNode));
	if (bufferSize > 0)
		theNode->buffer = (char*)_interruptSafeAllocate(bufferSize);
	else
		theNode->buffer = NULL;

	theNode->bufferSize = bufferSize;

	theNode->complete = false;
	
	theNode->fNext.Init();
	/*add it to our interrupt-safe list*/

	theDL->messageList.Enqueue(&theNode->fNext);

	return theNode;
}

/* ================================================================================
	Disposes a message-storage node

	 EXIT:	none
*/

static void	_disposeMessageStorageNode(MessageStorageNode* theNode)
{
	if (theNode->buffer)
		_interruptSafeDispose(theNode->buffer);
	_interruptSafeDispose(theNode);
}

/* ================================================================================
	Disposes a message-storage node

	 EXIT:	none
*/
static MessageStorageNode* _getNextNetMessage(ClientConnection *theDL)
{
	
	/*if theres nothing in our "current" list, steal the incoming list (which is a LIFO)
	and reverse it to make a nice FIFO */
	if (theDL->currentList == NULL)
	{
		theDL->currentList = theDL->messageList.StealList();
		if (theDL->currentList)
			theDL->currentList = NMReverseList(theDL->currentList);
	}
	
	/*if theres still no list, we return empty-handed*/
	if (!theDL->currentList)
		return NULL;

	/*now grab the first item off our current list and return it*/
	else
	{
		MessageStorageNode *theNode = NMGetLinkObject(theDL->currentList,MessageStorageNode,fNext);
		theDL->currentList = theDL->currentList->fNext;
		
		//if by chance we grab the node while its still being created, just wait
		while (theNode->complete == false)
		{}
		
		return theNode;
	}
}

/* ================================================================================
	Disposes a message-storage node

	 EXIT:	none
*/
static void _OPCallback( PEndpointRef inEndpoint, void* inContext,NMCallbackCode inCode, NMErr inError,void* inCookie )
{
	ClientConnection* theDownload = (ClientConnection*)inContext;

	//ignore if its NULL (we set its context to NULL when disposing its associated data)
	if (theDownload == NULL)
		return;
	
	MessageStorageNode *theStorageNode;

	//if its data, we make however many data storage-nodes we need to contain it all (we store a fixed amount per node)
	if (inCode == kNMStreamData)
	{
		NMFlags theFlags = 0;
		char buffer[kStorageNodeDataSize];
		char *bufferStart = buffer;
		NMUInt32 bufferSize;
		NMUInt32 prevReceivalSize;
		NMErr result;
		
		
		do{
			bufferSize = sizeof(buffer);
			bufferStart = buffer;
			
			//if we saved the last bit of the last data received, stick that in the beginning of our buffer and clear it
			strcpy(buffer,theDownload->prevReceivalEnd);
			prevReceivalSize = strlen(theDownload->prevReceivalEnd);
			bufferStart += prevReceivalSize;
			bufferSize -= prevReceivalSize;
			memset(theDownload->prevReceivalEnd,0,4);
		
			//now read any new data onto the end
			result = ProtocolReceive(inEndpoint,bufferStart,&bufferSize,&theFlags);
			
			if (!result){
			
				//reset the buffer to include the beginning chunk
				bufferSize += prevReceivalSize;
				bufferStart = buffer;
				
				//if we're still trying to strip out the header and don't find it, save the last bit of data and abort
				//if we do find it, go ahead and store everything after it
				if (theDownload->skipHeader){
					char *dataStart = strstr(buffer,"\r\n\r\n");
					if (dataStart){
						dataStart += strlen("\r\n\r\n");
						bufferSize -= (dataStart - bufferStart);
						bufferStart = dataStart;
					}
					else{
						dataStart = strstr(buffer,"\n\n");
						if (dataStart){
							dataStart += strlen("\n\n");					
							bufferSize -= (dataStart - bufferStart);
							bufferStart = dataStart;
						}	
					}
					if (dataStart){
						//take everything from here on out
						theDownload->skipHeader = false; 
					}
					//didnt find the blank line - save up to 3 of our last chars in case the blank line falls on a boundary and fail
					else{
						if (bufferSize > 2){
							strncpy(theDownload->prevReceivalEnd,&bufferStart[bufferSize - 3],3);
						}
						else{
							strncpy(theDownload->prevReceivalEnd,bufferStart,bufferSize);
						}
						bufferSize = 0;
					}
				}
				if(bufferSize > 0){

					/* make a storage node and copy the info in */
					theStorageNode = _newMessageStorageNode(theDownload,bufferSize);
					memcpy(theStorageNode->buffer,bufferStart,bufferSize);
					theStorageNode->download = theDownload;
					theStorageNode->cookie = inCookie;
					theStorageNode->code = inCode;
					theStorageNode->complete = true;
				}
			}			
		}while (!result);
	}
	else{
		/*make a single storage node and copy the info in */
		theStorageNode = _newMessageStorageNode(theDownload,0);
		theStorageNode->download = theDownload;
		theStorageNode->cookie = inCookie;
		theStorageNode->code = inCode;
		theStorageNode->complete = true;
	}

	//DebugStr("\pcbexit");
	
}

#ifdef __MWERKS__
#pragma mark -
#endif

/* ================================================================================
	Disposes a message-storage node

	 EXIT:	none
*/
void	DisposeClientConnection(ClientConnection *theDL)
{
	MessageStorageNode *theMessage;

	//close our endpoint if we have one
	//set its context to NULL so we know to ignore it from here on out
	if (theDL->theEndpoint){
		ProtocolSetEndpointContext(theDL->theEndpoint,NULL);
		ProtocolCloseEndpoint(theDL->theEndpoint,true);
	}
		
	//clean our list of messages
	theMessage = _getNextNetMessage(theDL);
	while (theMessage)
	{
		_disposeMessageStorageNode(theMessage);
		theMessage = _getNextNetMessage(theDL);
	}
	delete theDL;
}


/* ================================================================================
	Disposes a message-storage node

	 EXIT:	none
*/
ClientConnectionStatus	ProcessClientConnection(ClientConnection *theDL, char *buffer, NMUInt32 *bufferSize)
{
	MessageStorageNode *theMessage;
	NMFlags theFlags = 0;
	
	ProtocolIdle(theDL->theEndpoint);

	//if we have a current buffer, suck data out of it.
	if (theDL->currentBufferSize > 0){
	
		//if we havn't yet said we're downloading...
		if (theDL->announcedTransferring == false){
			theDL->announcedTransferring = true;
			printf("receiving data...\n"); fflush(stdout);
		}
	
		//it won't all fit - copy what we can
		if (theDL->currentBufferSize > *bufferSize){
			memcpy(buffer,theDL->currentBuffer,*bufferSize);
			theDL->currentBuffer += *bufferSize;
			theDL->currentBufferSize -= *bufferSize;
		}
		//it all fits - copy the buffer and kill it
		else{
			memcpy(buffer,theDL->currentBuffer,theDL->currentBufferSize);
			*bufferSize = theDL->currentBufferSize;
			theDL->currentBufferSize = 0;
			_interruptSafeDispose(theDL->currentBufferBase);
			theDL->currentBufferBase = NULL;
			theDL->currentBuffer = NULL;
		}
	}
	else{
		theMessage = _getNextNetMessage(theDL);
		if (theMessage)
		{
			switch (theMessage->code){
				//data has come in - store it as our current buffer for next time
				case kNMStreamData:
					{
						*bufferSize = 0;
						theDL->currentBufferBase = theMessage->buffer;
						theDL->currentBufferSize = theMessage->bufferSize;
						theDL->currentBuffer = theDL->currentBufferBase;
						theMessage->buffer = NULL;
						theMessage->bufferSize = 0;
					}
					break;
				case kNMEndpointDied:
					*bufferSize = 0;
					theDL->currentStatus = kConnectionStatusComplete;
					break;
				default:
					*bufferSize = 0;
					break;
			}
			_disposeMessageStorageNode(theMessage);
		}
		else
			*bufferSize = 0;
	}		
	return theDL->currentStatus;
}

/* ================================================================================
	Disposes a message-storage node

	 EXIT:	none
*/
ClientConnection* NewClientConnection(char *portString, NMBoolean skipHeader)
{
	char httpGetCommand[1024];
	ClientConnection *theDL;
	size_t hostCharCount;
	char hostName[256];
	int port = 1345; //random start value...
	
	theDL = new ClientConnection;
	memset(theDL->prevReceivalEnd,0,4);
	theDL->skipHeader = skipHeader;
	theDL->currentStatus = kConnectionStatusCantConnect;
	theDL->theEndpoint = NULL;
	theDL->messageList.Init();
	theDL->currentList = NULL;
	theDL->currentBuffer = NULL;
	theDL->currentBufferBase = NULL;
	theDL->currentBufferSize = 0;
	theDL->announcedTransferring = false;
	
	do{
		// First check that the portString begins with "http://"
		if ( strspn(portString, "http://") != strlen("http://") )
			break;	

		// Now skip over the "http://" and extract the host name.
		portString += strlen("http://");
		
		//count the chars before the next slash
		hostCharCount = strcspn(portString, "/");

		// Extract those characters from the URL into hostName
		//  and then make sure it's null terminated.
		(void) strncpy(hostName, portString, hostCharCount);
		hostName[hostCharCount] = 0;
		portString += hostCharCount;

		// Add a ":80" to the host name if necessary.
		if ( strchr( hostName, ':' ) == NULL ) {
			strcat( hostName, ":80" );
		}

		//now strip the port number right back off it..
		{
			char *portStart = strchr(hostName,':');
			*portStart = 0;
			portStart++;
			sscanf(portStart,"%d",&port);			
		}
		
		// Now place the URL into the HTTP command that we'll send
		if ( *portString == 0 ) {
			portString = "/";
		}
		sprintf(httpGetCommand, "GET %s HTTP/1.0%c%c%c%c", portString,13,10,13,10);

		//now open our OpenPlay connection to the host
		{
			PConfigRef theConfig;
			char configString[256];
			NMErr err = 0;

			//create a default tcpip config first
			err = ProtocolCreateConfig(kIPModuleType,'gtrl',"dummyGameName",NULL,0,NULL,&theConfig);
			if (err)
				break;
			
			//get a definition string for the config, modify it with our values, and remake it
			err = ProtocolGetConfigString(theConfig,configString,sizeof(configString));
			ProtocolDisposeConfig(theConfig);
			if (err)
				break;
			
			{
				char buffer[128];
				doSetConfigSubString(configString,"IPaddr",hostName);
				sprintf(buffer,"%d",port);
				doSetConfigSubString(configString,"IPport",buffer);
				
				//enable netsprocket mode - this causes the openplay netmodule to bypass its standard 
				//practice of sending confirmations right off the bat, which would of course confuse the
				//http server.  See the documentation for more info on the side-effects of netsprocket-mode.
				doSetConfigSubString(configString,"netSprocket","true");
				
				//TODO:  default "mode" obtains both a stream and datagram endpoint - we only use the stream endpoint here,
				//so it might be better to set the "mode" config-string item to stream-only.  This is largely untested at the moment, however.
			}
			printf("config string:\n%s\n\n",configString);
			
			//remake
			err = ProtocolCreateConfig(kIPModuleType,'gtrl',"dummyGameName",NULL,0,configString,&theConfig);
			if (err)
				break;
					
			
			printf("establishing connection...\n"); fflush(stdout);
			//now open the connection			
			err = ProtocolOpenEndpoint(theConfig,_OPCallback,theDL,&theDL->theEndpoint,kOpenActive);
			if (err)
				break;
			
				
			printf("sending request...\n"); fflush(stdout);
			//we're connected - send our request.
			{
				NMSInt32 result = ProtocolSend(theDL->theEndpoint,httpGetCommand,strlen(httpGetCommand),kNMBlocking);
				if (result != strlen(httpGetCommand))
					break;
			}
							
			//ok all's well.
			theDL->currentStatus = kConnectionStatusRunning;
		}
		
	}while (false);
	
	return theDL;
}

/* ================================================================================
	Disposes a message-storage node

	 EXIT:	none
*/
void OPHostTerm( void )
{
	NMErr err = kNMNoError;
		
	#if (OP_PLATFORM_MAC_CFM)
		#if TARGET_API_MAC_CARBON
			CloseOpenTransportInContext(_myOTContext);
		#else
			CloseOpenTransport();
		#endif
	#endif
}

/* ================================================================================
	Disposes a message-storage node

	 EXIT:	none
*/
void OPHostInit( void )
{
	NMErr err = kNMNoError;
	
		
	/*on mac, we gotta set up OpenTransport, which we use for interrupt-safe allocations (since OpenPlay's messages come in at interrupt time)*/
	#if (OP_PLATFORM_MAC_CFM)
		#if (TARGET_API_MAC_CARBON)
			err = InitOpenTransportInContext(kInitOTForApplicationMask,&_myOTContext);
		#else
			err = InitOpenTransport();
		#endif
	#endif
	
}
