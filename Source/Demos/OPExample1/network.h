/*
 * Copyright (c) 2002 Lane Roathe. All rights reserved.
 *	Developed in cooperation with Freeverse, Inc.
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


#ifdef __cplusplus
	extern "C"{
#endif

/* ----------------------------------------------------------- Macro Definitions */

#include "OpenPlay.h"	/* Networking library */

/* ----------------------------------------------------------- Type Definitions */


/* ----------------------------------------------------------- Function Prototypes */

void NetworkHandleMessage( void );

NMUInt32 NetworkGetPlayerCount( void );

NMErr NetworkSendAnswer
(
	char answer					/* the answer to send (just a char!) */
);

NMErr NetworkSendPlayerMessage
(
	const char *message,		/* ptr to message string to send */
	NMUInt32 messageType		/* type of message (question, info, etc. */
);

NMErr NetworkSendQuestion
(
	const char *question		/* ptr to question string to send */
);

NMErr NetworkSendInformation
(
	const char *message			/* ptr to question string to send */
);

NMErr NetworkStartServer
(
	NMUInt16 port,					/* port clients will connect on */
	NMUInt32 maxPlayers,			/* max # of players to allow into game */
	const unsigned char *gameName,	/* name of game (displayed to clients on connect) */
	const unsigned char *playerName	/* name of player on server computer */
);

NMErr NetworkStartClient
(
	char *ipAddr,					/* IP address (or domain name) to look for server (host) on */
	char *port,						/* Port to talk to server via */
	const unsigned char *playerName	/* name of player wanting to join */
);

NMErr NetworkShutdown( void );
NMErr NetworkStartup( void );

#ifdef __cplusplus
	}
#endif
