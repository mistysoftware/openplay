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

#include "OPUtils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "OPGetURL.h"

#ifdef __MWERKS__
	#include <SIOUX.h>
#endif

static const char *defaultURLs[10] = {
"http://www.apple.com",
"http://www.apple.com/developer/",
"http://developer.apple.com/darwin/projects/openplay/",
"http://developer.apple.com/technotes/index.html",
"",
"",
"",
"",
"",
""
};

int main(int argc, char **argv)
{
	char urlString[256];
	int success = true;
	int done = false;
	FILE *theFile;
	
#if defined(__MWERKS__)
	#if (OP_PLATFORM_MAC_CFM)
	    SIOUXSettings.autocloseonquit = FALSE;	// don't close the SIOUX window on program termination
	    SIOUXSettings.asktosaveonclose = FALSE;	// don't offer to save on a close
	#endif
#endif
	
	
	OPHTTPInit(NULL);
	
	//open an outfile
	remove("outfile");
	theFile = fopen("outfile","wb");
	if (theFile == NULL){
		printf("#error: can't open outfile\n");
		return 1;
	}
	
	//check for a url argument or prompt for one
	if (argc > 1)
		strcpy(urlString,argv[1]);
	else{
		int i;
		printf("OPDownloadHTTP!\n");
		printf("-- Downloads a URL using OpenPlay's TCPIP Module\n");
		printf("\n");
	    fflush(stdout);
		
		for (i = 0; i < 10; i++) {
			if ( *defaultURLs[i] != 0 ) {
				printf("%ld> %s\n", i, defaultURLs[i]);
                                fflush(stdout);
			}
		}
		printf("\n");
		printf("Enter a URL (or type a number from the above list):\n");
                fflush(stdout);
		scanf("%s",urlString);		
		if ( strlen(urlString) == 1 && isdigit(urlString[0])) {
			strcpy( urlString, defaultURLs[ urlString[0] - '0' ] );
		}
		
	}

	printf("attempting to download \"%s\"\n",urlString);
	
	{
		OPHTTPDownload *theDownload;
		char buffer[5];
		unsigned long bufferSize;
		OPHTTPDownloadStatus dlStatus;
		unsigned int bytesReceived = 0;
	
		theDownload = NewOPHTTPDownload(urlString,true);
	
		while (!done)
		{
			bufferSize = sizeof(buffer);
			dlStatus = ProcessOPHTTPDownload(theDownload,buffer,&bufferSize);
			switch (dlStatus){
				//all is well - see if we got new data
				case kDLStatusRunning:
					if (bufferSize > 0){
						bytesReceived += bufferSize;
						fwrite(buffer,bufferSize,1,theFile);
					}
					break;
				//initial connection failed
				case kDLStatusCantConnect:
					printf("#error - connection failed\n");
					fflush(stdout);
					success = false;
					done = true;
					break;
				//woohoo!
				case kDLStatusComplete:
					printf("download complete - %d bytes downloaded\n",bytesReceived);
					fflush(stdout);
					done = true;
					success = true;
					break;
				//died somewhere in the middle
				case kDLStatusFailed:
					printf("#error download failed\n");
					fflush(stdout);
					done = true;
					success = false;
					break;
			}
		}
		DisposeOPHTTPDownload(theDownload);
	}
	
	OPHTTPTerm();
	fclose(theFile);
	
	if (success)
		return 0;
	else
		return 1;
}

