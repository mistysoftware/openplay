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

#ifndef __APPLETALKNAVIGATIONAPI__
#define __APPLETALKNAVIGATIONAPI__

//	------------------------------	Public Definitions


	enum
	{
		// Constants to use with OTCreateZonesEnumerator / OTCreateNBPEntitiesEnumerator
		kATAppleTalkDefaultPortRef	= 0,
		
		// Invalid enumeratorRef	
		kATInvalidEnumeratorRef		= 0,
		
		// Constants to use with OTStartNBPEntitiesLookup
		kATClearPreviousResults		= true,
		kATAddToPreviousResults		= false,
		
		// Event codes for notifiers
		
		// Returned by the Zones enumeraor
		AT_ZONESLIST_COMPLETE		= 'zone',
		
		// Returned by the entites enumerator
		AT_ENTITIESLIST_COMPLETE	= 'name',
		AT_NEW_ENTITIES				= 'more',
		
		// Should NEVER be returned
		AT_UNEXPECTED_EVENT			= '????',
		
		// Error codes
		kNavigationAPIErrorBase		= 1,
		kATEnumeratorBadIndexErr	= kNavigationAPIErrorBase,
		kATEnumeratorBadReferenceErr,
		kATEnumeratorBadZoneErr,
		kATEnumeratorBadPortErr,
		
		
		kLastAndMeaningLessItemInEnum
	};

// 19990124 sjb move these out of the enum so they have the
//  right types
	const StringPtr kATSearchAllZones = NULL;
	const StringPtr kATSearchAllNames = NULL;
	const StringPtr kATSearchAllEntityTypes = NULL;

	typedef NMUInt32	ATEventCode;
	typedef NMUInt32	ATPortRef;
	typedef	NMUInt32	ATZonesEnumeratorRef;
	typedef	NMUInt32	ATNBPEntitiesEnumeratorRef;
	typedef	NMUInt32	OneBasedIndex;

//	------------------------------	Public Types


	struct	ATAddress
	{
		UInt16			fNetwork;
		NMUInt8			fNodeID;
		NMUInt8			fSocket;
	};
	typedef struct ATAddress ATAddress;

//	------------------------------	Public Functions


#if __cplusplus
extern "C" {
#endif

//-----------------------------------------------------------------------------
//	Enumerating zones
//-----------------------------------------------------------------------------

	// 
	// OTCreateZonesEnumerator: creates an enumerator for the zones. Accepts an optional notifier
	// 
	extern NMErr	ATCreateZonesEnumerator(ATPortRef port, OTNotifyUPP notifier, void* contextPtr, ATZonesEnumeratorRef* ref);

	//
	//	OTGetZonesCount: returns the number of zones found so far. Indicates if all zones have been found
	//
	extern NMErr	ATGetZonesCount(ATZonesEnumeratorRef ref, NMBoolean* allFound, NMUInt32* count);

	//
	//	OTGetIndexedZone: returns the name of zones by index
	//
	extern NMErr	ATGetIndexedZone(ATZonesEnumeratorRef ref, OneBasedIndex index, StringPtr zoneName);

	//
	//	OTSortZones: sorts the list of zones by alphabetical order
	//
	extern NMErr	ATSortZones(ATZonesEnumeratorRef ref);

	//
	//	OTGetMachineZone returns the name of the zone in which the machine is located
	//
	extern NMErr	ATGetMachineZone(ATZonesEnumeratorRef ref, StringPtr zoneName);

	// 
	// OTDeleteZonesEnumerator: deletes the enumerator
	// 

	extern NMErr	ATDeleteZonesEnumerator(ATZonesEnumeratorRef* ref);

//-----------------------------------------------------------------------------
//	Enumerating NBP entities
//-----------------------------------------------------------------------------

//
//	OTCreateNBPEntitiesEnumerator:	creates an ATNBPEntitiesEnumeratorRef.
//
//	port is an ATPortRef specifying the port where the enumeration should take place.  pass kATAppleTalkDefaultPortRef to use the
//	default AppleTalk port.
//	
//	notifier is an OTNotifyUPP that will be called when data is available, when the lookup has completed, or if an error occurs.
//	when the notifier is called, the cookie will be the ATNBPEntitiesEnumeratorRef
//	
//	contextPtr is a void* which is passed as the contextPtr argument when the notifier is called.  
//	
//	upon exit, ref will contain a pointer to an ATNBPEntitiesEnumeratorRef.  this reference must be passed to all other functions
//	which take an ATNBPEntitiesEnumeratorRef

	extern NMErr	ATCreateNBPEntitiesEnumerator(ATPortRef port, OTNotifyUPP notifier, void* contextPtr, ATNBPEntitiesEnumeratorRef* ref);

//
// OTStartNBPEntitiesLookup: starts looking for entities if the specified type and optionally prefix in the specified zone
// 
// This routine can be invoqued several times either to 
//		(1) perform successive independant lookups
//		(2) perform simultaneous lookups on several zones and/or types
//		(3) search the whole network for a given type and/or name
//
// For case (1) pass true for the clearPreviousResults parameter to delete any result of the previous lookup. Do not start a new lookup
// before the previous one has either completed or has been cancelled by calling OTCancelNBPEntitiesLookup.
//
// For case (2) pass false for the clearPreviousResults parameter to indicate that the results of the new lookup should augment the current
// result list. In this case OTStartNBPEntitiesLookup can be called before the previous lookup completes.
//
// For case (3) pass true for the clearPreviousResults parameter to discard any entities that may have been found previously
// A word of caution: network wide search generates a HUGE network traffic. You should refrain to perform such a lookup unless
// the user explicitely chooses to do so.
//

	extern NMErr	ATStartNBPEntitiesLookup(ATNBPEntitiesEnumeratorRef ref, Str32 zone, Str32 type, Str32 prefix, NMBoolean clearPreviousResults);

//
// OTGetNBPEntitiesCount: returns the number of objects found so far. Indicates if all have been found.
//

	extern NMErr	ATGetNBPEntitiesCount(ATNBPEntitiesEnumeratorRef ref, NMBoolean* allFound, NMUInt32* count);

//
// OTGetIndexedNBPEntity: returns objects by index
//

	extern NMErr	ATGetIndexedNBPEntity(ATNBPEntitiesEnumeratorRef ref, OneBasedIndex index, StringPtr zone, StringPtr type, StringPtr name, ATAddress* address);

//
// OTSortNBPEntities: sorts objects fgound so far by name
//

	extern NMErr	ATSortNBPEntities(ATNBPEntitiesEnumeratorRef ref);

//
// OTCancelNBPEntitiesLookup: cancels ongoing lookups (all of them)
//

	extern NMErr	ATCancelNBPEntitiesLookup(ATNBPEntitiesEnumeratorRef ref);

//
// OTDeleteNBPEntitiesEnumerator: deletes the enumerator
//

	extern NMErr	ATDeleteNBPEntitiesEnumerator(ATNBPEntitiesEnumeratorRef* ref);


#if __cplusplus
}
#endif

#endif	// __APPLETALKNAVIGATIONAPI__
