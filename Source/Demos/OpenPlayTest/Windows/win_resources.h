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

#define rMENU_BAR_ID (128)

enum { // MBAR
	rMENU_BAR_ID= 128
};

enum { // windows
	winDOCUMENT= 128
};

enum { // dlogs
	dlogSelectProtocol= 128,
	iPROTOCOL_LIST= 3,

	dlogConfigureProtocol= 129,
	iCreateGame= 1,
	iGameList= 3,
	iProtocolDataFrame,
	iJoinGame= 7,
	
	dlogSetTimeout= 130,
	iTimeout= 3
};

enum { // menus
	mApple= 128,
	iAbout= 1,
	
	mFile= 129,
	iOpen= 1,
	iOpenPassive,
	iOpenActiveUI,
	iOpenPassiveUI,
	iClose,
	iSep0,
	iQuit,
	
	mSpecial= 130,
	iSendPacket= 1,
	iSendStream,
	iProtocolAlive,
	iAcceptingConnections,
	iActiveEnumeration,
	iSetTimeout,
	iGetInfo
};
