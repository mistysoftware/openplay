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

#ifndef __NETMODULEAT__
#define __NETMODULEAT__

//	------------------------------	Includes


	#include <OpenTptAppleTalk.h>

	#include "NetModule.h"
	#include "Endpoint.h"
	#include "AppleTalkNavigationAPI.h"
	#include "ATPassThrus.h"

//	------------------------------	Public Definitions


	extern const char *kATNBPConfigAddress;

	enum {kMaxGameNameLen = 31, kNMEnumDataLen = 256};

	enum {kLDEFOffsetFakeID = 256};

//	------------------------------	Public Types


	typedef struct NMATEndpointPriv
	{
		NMUInt32	id;
		Endpoint		*ep;
	} NMATEndpointPriv;

	typedef struct NMATConfigPriv
	{
		NMType			type;
		NMUInt32			version;
		NMUInt32			gameID;
		NMUInt32			connectionMode;
		NMBoolean		netSprocketMode;
	// 19990139 make below cstr for SURE
		char			name[kMaxGameNameLen + 1];	// should be word-aligned
	//	DDPAddress		address;
		Str32			nbpName;
		Str32			nbpType;
		Str32			nbpZone;
		NMUInt32	customEnumDataLen;
		char			customEnumData[kNMEnumDataLen];
	} NMATConfigPriv;


	enum {kModuleID = 'Atlk', kVersion = 0x00000100};

	enum {kMaxConfigStringLen = 512};


	typedef	struct ATEnumerationItemPriv
	{
		NMUInt32				enumIndex;
		Str63				name;
		NMEnumerationItem 	userEnum;
	} ATEnumerationItemPriv;


/*	Format of the enumeration response is:

	4 byte response code (including version)
	8 byte game ID
	4 byte host id
	2 byte port
	2 byte pad
	32 byte game name (null terminated)
	4 byte enum data len
	variable length custom enum data, up to kNMEnumDataLen
*/

	typedef struct ATEnumerationResponsePacket
	{
		NMUInt32			responseCode;
		UnsignedWide	gameID;
		DDPAddress		address;
		char			name[32];
		NMUInt32	customEnumDataLen;
		char			customEnumData[kNMEnumDataLen];
	} ATEnumerationResponsePacket;

#ifdef __cplusplus
extern "C" {
#endif

	NMErr MakeNewATEndpointPriv(NMConfigRef inConfig, NMEndpointCallbackFunction *inCallback, void *inContext, NMUInt32 inMode, NMBoolean inNetSprocketMode, NMATEndpointPriv **theEP);
	void DisposeATEndpointPriv(NMATEndpointPriv *inEP);
	void CloseEPComplete(NMEndpointRef inEPRef);
	NMErr	DoPassThrough(NMEndpointRef inEndpoint, NMType inSelector, void *inParamBlock);
	NMErr InternalSetLookupMethod(NMSInt32 method);
	NMErr InternalSetSearchZone(unsigned char *zone);
	NMErr InternalSetSearchZones(unsigned char *zones);
	NMErr InternalGetZoneList(unsigned char **toZones, NMUInt32 timeout);
	NMErr InternalDisposeZoneList(unsigned char *toZones);
	NMErr InternalGetMyZone(unsigned char *zoneStore);

#ifdef __cplusplus
}
#endif

#endif
