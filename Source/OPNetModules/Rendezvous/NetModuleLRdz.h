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
 */

#ifndef __NETMODULERENDEZVOUS__
#define __NETMODULERENDEZVOUS__

//	------------------------------	Includes

	#include <OpenTptInternet.h>

	#include "Endpoint.h"
	#include "ip_enumeration.h"
	#include "NetModule.h"

//	------------------------------	Public Definitions
//	------------------------------	Public Types

	typedef struct NMIPEndpointPriv
	{
		NMType		id;
		Endpoint	*ep;
	} NMIPEndpointPriv;

	typedef struct NMIPConfigPriv
	{
		NMType			type;
		NMUInt32		version;
		NMUInt32		gameID;
		NMUInt32		connectionMode;
		NMBoolean		netSprocketMode;
		char			name[kMaxGameNameLen + 1];	// should be word-aligned
		char			gamename[kMaxGameNameLen + 1];	// should be word-aligned
		InetAddress		address;
		NMUInt32		customEnumDataLen;
		char			customEnumData[kNMEnumDataLen];
	} NMIPConfigPriv;

	typedef struct NMDialogData
	{
		InetPort	port;
		InetHost	host;
		Str32		portText;
		Str255		hostText;
	} NMDialogData;

	enum
	{
		kModuleID = 'Rdzv',
		kVersion = 0x00000100
	};

	enum
	{
		kMaxConfigStringLen = 512
	};

//	------------------------------	Public Variables

	extern const char *	kIPConfigAddress;
	extern const char *	kIPConfigPort;
	extern NMBoolean	gTerminating;

//	------------------------------	Public Functions

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

	extern NMErr	MakeNewIPEndpointPriv(NMConfigRef inConfig, NMEndpointCallbackFunction *inCallback, void *inContext, NMUInt32 inMode, NMBoolean inNetSprocketMode, NMIPEndpointPriv **theEP);
	extern void		DisposeIPEndpointPriv(NMIPEndpointPriv *inEP);
	extern void		CloseEPComplete(NMEndpointRef inEPRef);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif __NETMODULEIP__

