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

#ifndef __ENUMERATORS__
#define __ENUMERATORS__

//	------------------------------	Includes


	#include "AppleTalkNavigationAPI.h"

//	------------------------------	Public Types


	class CZonesEnumerator
	{
		public:
		
								CZonesEnumerator()														{};
			virtual				~CZonesEnumerator()														{};
							
			virtual	NMErr	Initialize(ATPortRef port, OTNotifyUPP notifier, void* contextPtr)	= 0;	
			virtual NMErr	GetCount(NMBoolean* allFound, NMUInt32* count)								= 0;
			virtual NMErr	GetMachineZone(StringPtr zoneName)										= 0;
			virtual NMErr	GetIndexedZone(OneBasedIndex index, StringPtr zoneName)					= 0;
			virtual NMErr	Sort()																	= 0;

		private:
	};

	class CEntitiesEnumerator
	{
		public:
		
								CEntitiesEnumerator()																						{};
			virtual				~CEntitiesEnumerator()																						{};
								
			virtual	NMErr	Initialize(ATPortRef port, OTNotifyUPP notifier, void* contextPtr)										= 0;
			virtual NMErr	StartLookup(Str32 zone, Str32 type, Str32 prefix, NMBoolean clearPreviousResults)								= 0;
			virtual NMErr	GetCount(NMBoolean* allFound, NMUInt32* count)																	= 0;
			virtual NMErr	GetIndexedEntity(OneBasedIndex index, StringPtr zone, StringPtr type, StringPtr name, ATAddress* address)	= 0;
			virtual NMErr	Sort()																										= 0;
			virtual NMErr	CancelLookup()																								= 0;


		private:
	};

#endif	// __ENUMERATORS__


