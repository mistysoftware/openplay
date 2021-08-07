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

#ifndef __ATPASSTHRU__
#define __ATPASSTHRU__

//	------------------------------	Includes

	#include "NetModule.h"

//	------------------------------	Public Definitions
//	------------------------------	Public Types

	enum
	{
		kUseConfig						= 0,
		kSearchSpecifiedZones
	};

	enum
	{
		kSetLookupMethodSelector		= 1,
		kSetSearchZonesSelector			= 2,
		kGetZoneListSelector			= 3,
		kDisposeZoneListSelector		= 4,
		kGetMyZoneSelector				= 5
	};

	enum
	{
		kNMMaxZoneNameLen				= 32
	};

	typedef struct SetLookupMethodPB
	{
		NMSInt32	method;
	} SetLookupMethodPB;


	typedef struct SetSearchZonesPB
	{
		unsigned char	*zones;	// packed pascal strings, terminated with a 0
	} SetSearchZonesPB;

	typedef struct GetZoneListPB
	{
		unsigned char	*zones;	// packed pascal strings, terminated with a 0
		NMUInt32			timeout;	// milliseconds
	} GetZoneListPB;

	typedef struct GetMyZonePB
	{
		unsigned char	zoneName[kNMMaxZoneNameLen + 1];
	} GetMyZonePB;



#endif	// __ATPASSTHRU__

