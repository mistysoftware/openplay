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

// This demo uses the OpenPlay TCPIP NetModule and "NetSprocketMode" to download a file via a http server.
// NetSprocketMode was so named because it was created to allow OpenPlay to act as a wrapper for NetSprocket.
// By specifying "NetSprocketMode" as true in an endpoint's configuration string, an endpoint will function in a slightly
// modified mode of operation where all datagram messages sent by a client arrive at the original host endpoint instead of its spawned
// endpoints.  Thus you lose the ability to see which client sent a datagram message, but there are a few advantages in return:
// 1> both stream and datagram endpoints are always created on the same port, which enables hosting behind a firewall using only one "blessed" port
// 2> in this mode, the openplay protocol sends no data over the network itself, enabling openplay to act simply as a wrapper for sockets/Open-Transport.


// this second aspect of NetSprocketMode is used in this sample - we simply open a connection to a http server, send out a request, and accept data in return, which
// shows up as stream data on our OpenPlay endpoint.

//includes

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef __MWERKS__
	#include <SIOUX.h>
#endif

#include "OpenPlay.h"

#include "Host_Connections.h"

/* ================================================================================
	Disposes a message-storage node

	 EXIT:	none
*/
int main(int argc, char **argv)
{
	char portString[256];
	int success = true;
	int done = false;
	
#if defined(__MWERKS__)
	#if (OP_PLATFORM_MAC_CFM)
	    SIOUXSettings.autocloseonquit = FALSE;	// don't close the SIOUX window on program termination
	    SIOUXSettings.asktosaveonclose = FALSE;	// don't offer to save on a close
	#endif
#endif
	
	
	OPHostInit();
	
	//check for a url argument or prompt for one
	if (argc > 1)
		strcpy(portString,argv[1]);
	else
	{
		printf("OPDownloadHTTP!\n");
		printf("-- Downloads a URL using OpenPlay's TCPIP Module\n");
		printf("\n");
		
		printf("Enter the port # to listen on:\n");
		fflush(stdout);

		scanf("%s",portString);		

		if ( strlen(portString) == 1 && isdigit(portString[0]))
		{
			strcpy( portString, defaultURLs[ portString[0] - '0' ] );
		}
	}

	printf("attempting to start listening for a connection on port \"%s\"\n",portString);
	{
		ClientConnectionPtr clientConnection;
		char buffer[5];
		unsigned long bufferSize;
		ClientConnectionStatus dlStatus;
		unsigned int bytesReceived = 0;
	
		clientConnection = NewClientConnection(portString,true);
	
		while (!done)
		{
			bufferSize = sizeof(buffer);
			dlStatus = ProcessClientConnection(clientConnection,buffer,&bufferSize);
			switch (dlStatus){
				//all is well - see if we got new data
				case kConnectionStatusRunning:
					if (bufferSize > 0){
						bytesReceived += bufferSize;
						fwrite(buffer,bufferSize,1,theFile);
					}
					break;
				//initial connection failed
				case kConnectionStatusCantConnect:
					printf("#error - connection failed\n");
					fflush(stdout);
					success = false;
					done = true;
					break;
				//woohoo!
				case kConnectionStatusComplete:
					printf("download complete - %d bytes downloaded\n",bytesReceived);
					fflush(stdout);
					done = true;
					success = true;
					break;
				//died somewhere in the middle
				case kConnectionStatusFailed:
					printf("#error download failed\n");
					fflush(stdout);
					done = true;
					success = false;
					break;
			}
		}
		DisposeClientConnection(clientConnection);
	}
	
	OPHostTerm();
	fclose(theFile);
	
	if (success)
		return 0;
	else
		return 1;
}

