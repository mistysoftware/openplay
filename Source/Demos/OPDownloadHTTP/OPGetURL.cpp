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


#include "OPGetURL.h"
#include "OpenPlay.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "String_Utils.h"
#include "LinkedList.h"
#include "OPUtils.h"

#define kStorageNodeDataSize 10000

typedef struct OPHTTPDownload
{
	char prevReceivalEnd[4];
	NMBoolean skipHeader;
	NMLink *currentList;
	NMLIFO messageList;
	OPHTTPDownloadStatus currentStatus;
	PEndpointRef theEndpoint;
	char		*currentBufferBase;
	char		*currentBuffer;
	NMUInt32	currentBufferSize;
	NMBoolean announcedTransferring;
}OPHTTPDownload;

typedef struct MessageStorageNode
{
	NMBoolean complete;
	NMLink fNext;
	OPHTTPDownload		*download;
	void				*cookie;
	NMCallbackCode 		code;
	char				*buffer;
	NMUInt32			bufferSize;
}MessageStorageNode, *MessageStorageNodePtr;


#if TARGET_API_MAC_CARBON
	static OTClientContextPtr 	gOTContext; /* we use open transport for interrupt-safe memory allocation */
	static NMBoolean 		gInitedOTContext;
#endif

#if (OP_PLATFORM_MAC_CARBON_FLAG)
	void OPHTTPInit(OTClientContextPtr gOTContextIn)
#else
	void OPHTTPInit(void *unused)
#endif
{
	NMErr err = kNMNoError;
	
		
	/*on mac, we gotta set up OpenTransport, which we use for interrupt-safe allocations (since OpenPlay's messages come in at interrupt time)*/
	#if (OP_PLATFORM_MAC_CFM)
		#if (TARGET_API_MAC_CARBON)
			if (gOTContext){
				gOTContext = gOTContextIn;
				gInitedOTContext = false;
			}
			else{
				err = InitOpenTransportInContext(kInitOTForApplicationMask,&gOTContext);
				gInitedOTContext = true;
			}
		#else
			err = InitOpenTransport();
		#endif
	#endif
	
}

void OPHTTPTerm( void )
{
	NMErr err = kNMNoError;
		
	#if (OP_PLATFORM_MAC_CFM)
		#if TARGET_API_MAC_CARBON
			if (gInitedOTContext)
				CloseOpenTransportInContext(gOTContext);
		#else
			CloseOpenTransport();
		#endif
	#endif
}

static void*	InterruptSafeAllocate(long size)
{
	
	void *chunk;

	#if (OP_PLATFORM_MAC_CFM)
		#if (TARGET_API_MAC_CARBON)
			chunk = OTAllocMemInContext(size,gOTContext);
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

static void	InterruptSafeDispose(void *data)
{
	#if (OP_PLATFORM_MAC_CFM)
		OTFreeMem(data);
	#elif (OP_PLATFORM_WINDOWS)
		GlobalFree((HGLOBAL)data);
	#elif (OP_PLATFORM_UNIX)
		free(data);
	#endif

}

static MessageStorageNode*  NewMessageStorageNode(OPHTTPDownload *theDL, NMUInt32 bufferSize)
{
	MessageStorageNode* theNode;
		
	theNode = (MessageStorageNode*)InterruptSafeAllocate(sizeof(MessageStorageNode));
	if (bufferSize > 0)
		theNode->buffer = (char*)InterruptSafeAllocate(bufferSize);
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

static void	DisposeMessageStorageNode(MessageStorageNode* theNode)
{
	if (theNode->buffer)
		InterruptSafeDispose(theNode->buffer);
	InterruptSafeDispose(theNode);
}

static MessageStorageNode* GetNextNetMessage(OPHTTPDownload *theDL)
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

static void OpenPlay_Callback( PEndpointRef inEndpoint, void* inContext,NMCallbackCode inCode, NMErr inError,void* inCookie )
{
	OPHTTPDownload* theDownload = (OPHTTPDownload*)inContext;

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
					theStorageNode = NewMessageStorageNode(theDownload,bufferSize);
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
		theStorageNode = NewMessageStorageNode(theDownload,0);
		theStorageNode->download = theDownload;
		theStorageNode->cookie = inCookie;
		theStorageNode->code = inCode;
		theStorageNode->complete = true;
	}

	//DebugStr("\pcbexit");
	
}

OPHTTPDownload* NewOPHTTPDownload(char *urlString, NMBoolean skipHeader)
{
	char httpGetCommand[1024];
	OPHTTPDownload *theDL;
	size_t hostCharCount;
	char hostName[256];
	int port = 1345; //random start value...
	
	theDL = new OPHTTPDownload;
	memset(theDL->prevReceivalEnd,0,4);
	theDL->skipHeader = skipHeader;
	theDL->currentStatus = kDLStatusCantConnect;
	theDL->theEndpoint = NULL;
	theDL->messageList.Init();
	theDL->currentList = NULL;
	theDL->currentBuffer = NULL;
	theDL->currentBufferBase = NULL;
	theDL->currentBufferSize = 0;
	theDL->announcedTransferring = false;
	
	do{
		// First check that the urlString begins with "http://"
		if ( strspn(urlString, "http://") != strlen("http://") )
			break;	

		// Now skip over the "http://" and extract the host name.
		urlString += strlen("http://");
		
		//count the chars before the next slash
		hostCharCount = strcspn(urlString, "/");

		// Extract those characters from the URL into hostName
		//  and then make sure it's null terminated.
		(void) strncpy(hostName, urlString, hostCharCount);
		hostName[hostCharCount] = 0;
		urlString += hostCharCount;

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
		if ( *urlString == 0 ) {
			urlString = "/";
		}
		sprintf(httpGetCommand, "GET %s HTTP/1.0%c%c%c%c", urlString,13,10,13,10);

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
			err = ProtocolOpenEndpoint(theConfig,OpenPlay_Callback,theDL,&theDL->theEndpoint,kOpenActive);
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
			theDL->currentStatus = kDLStatusRunning;
		}
		
	}while (false);
	
	return theDL;
}

void	DisposeOPHTTPDownload(OPHTTPDownload *theDL)
{
	MessageStorageNode *theMessage;

	//close our endpoint if we have one
	//set its context to NULL so we know to ignore it from here on out
	if (theDL->theEndpoint){
		ProtocolSetEndpointContext(theDL->theEndpoint,NULL);
		ProtocolCloseEndpoint(theDL->theEndpoint,true);
	}
		
	//clean our list of messages
	theMessage = GetNextNetMessage(theDL);
	while (theMessage)
	{
		DisposeMessageStorageNode(theMessage);
		theMessage = GetNextNetMessage(theDL);
	}
	delete theDL;
}


OPHTTPDownloadStatus	ProcessOPHTTPDownload(OPHTTPDownload *theDL, char *buffer, NMUInt32 *bufferSize)
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
			InterruptSafeDispose(theDL->currentBufferBase);
			theDL->currentBufferBase = NULL;
			theDL->currentBuffer = NULL;
		}
	}
	else{
		theMessage = GetNextNetMessage(theDL);
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
					theDL->currentStatus = kDLStatusComplete;
					break;
				default:
					*bufferSize = 0;
					break;
			}
			DisposeMessageStorageNode(theMessage);
		}
		else
			*bufferSize = 0;
	}		
	return theDL->currentStatus;
}


