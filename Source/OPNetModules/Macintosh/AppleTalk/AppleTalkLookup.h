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

#ifndef __APPLETALKLOOKUP__
#define __APPLETALKLOOKUP__

//	------------------------------	Includes


/*
	Overview:
	
	Requirements:  	You must call InitOpenTransport before creating AppleTalkLookup objects.
					You must not create or call any methods of AppleTalkLookup after
					calling CloseOpenTransport.
*/


	#include "OpenTransport.h"
	#include "OpenTptAppleTalk.h"
	#include "AppleTalkNavigationAPI.h"


//	------------------------------	Public Types


	typedef	NMUInt32	OneBasedIndex;


	typedef struct
	{
		TLookupReply	fReply;	// Note that this field should be the first one as we are going to cast TMyLookupRequest to TLookupRequest
		Str32			fZone;
		NMBoolean		fBusy;
		NMUInt32			fReplyNumber;
	} TMyLookupReply;



	class AppleTalkLookup
	{

		public:
							AppleTalkLookup();
			
							
							~AppleTalkLookup();
							//	frees all memory and deletes all items in the lookup list.
			
			
			NMErr		Init(OTNotifyUPP notifier, void* contextPtr);
			//	initializes the AppleTalk lookup.  the notifier is called when data is available,
			//	when the lookup is complete, and when a error occurs.  contextPtr will be passed
			//	as the contextPtr parameter when the notifier is called.
			

			NMErr		StartLookup(Str32 name, Str32 type, Str32 zone);
			//	begins an AppleTalk lookup.
			//	name may be a fully specified name, a partial name, or an empty name
			//	a partial name must contain the 'Å' character.
			//	an empty name is converted to match anything, i.e., "\p="
			
			
			
			NMErr		StartSearch(Str32 name, Str32 type);
			//	begins anAppleTalk lookup.
			//	name may be a fully specified name, a partial name, or an empty name
			//	a partial name must contain the 'Å' character.
			//	an empty name is converted to match anything, i.e., "\p="
			
			
			
			NMErr		GetCount(NMUInt32* count, NMBoolean* done);
			//	returns the number of entities found so far.  if the lookup
			//	has completed, then done will be set to true, otherwise it will
			//	be set to false.
			
			
			NMErr		GetIndexedItem(OneBasedIndex index, Str32 name, Str32 type, Str32 zone, DDPAddress* address);
			//	returns the indexth item in the list.
			
			
			void			CancelLookup();
			//	cancels the current lookup, if one is in progress, and removes
			//	all remaining items in the lookup list.		
			

			void			Reset();
			//	Deletes the entities currently in the list.		
			

			void			Sort();
			//	sorts the list.		
			

			void			Notify(OTEventCode event, NMErr theErr, void* cookie);
			//	this method is called by the OpenTransport notifier.  do not call this method.
			
			
			void			SetNotifier(OTNotifyUPP proc, void* cookie);
			//	sets the client notifier.  The notifier is called when a client calls Lookup
			//	and data has arrived.  The notifier is also called when the lookup completes.
			//	if a call to GetCount returns a positive number of items available, then the
			//	notifier will be called again when more data is available.
			
		private:

			NMErr		OpenMapper();
			//	opens the mapper, making it ready for a lookup to start
			 
			void 			CloseMapper();
			//	closes the Mapper

			NMErr		GetFreeLookup(TLookupRequest** request, TMyLookupReply** reply);
			// returns a free lookup request, returns NULL if none avaialble
			
			inline NMBoolean		HasPendingLookups() { return fPendingLookups > 0; }
			// returns true if some lookups are still pending
			
			NMErr		CreateIndexList();
			NMErr		GrowIndexList(NMSInt16 growBy);
			void			DeleteIndexList();
			
			NMErr		CreateRepliesList();
			NMErr		GrowRepliesList(NMSInt16 growBy);
			void			DeleteRepliesList();
			
			NMBoolean		HasMoreZonesToSearch();
			void			SearchNextZones();
			
			
			enum
			{
				kConcurrentLookups	= 32
			};
			
			NMErr				fErr;
			DDPAddress				fDDPAddress;
			TLookupRequest			fRequest[kConcurrentLookups];
			TMyLookupReply			fReply[kConcurrentLookups];
			NMUInt32				fFreeReplyHint;
			NMUInt32				fPendingLookups;
			TMapper*				fMapper;
			char*					fReplyBuffer;
			OTNotifyUPP				fNotifier;	
			void*					fNotifierContext;
			unsigned char**			fIndexList;
			NMUInt32				fIndexListSize;
			unsigned char*			fRepliesList;
			unsigned char*			fRepliesListEndPtr;
			NMUInt32				fRepliesCount;
			unsigned char*			fNextReplyPtr;
			char					fLookupString[kNBPEntityBufferSize];

			// For searching
			ATZonesEnumeratorRef	fZonesEnumerator;
			NMUInt32				fZonesCount;
			NMUInt32				fNextZoneIndex;
			NMBoolean				fLookingForZones;
			Str32					fSearchType;
			Str32					fSearchName;
	};


#endif	//	__APPLETALKLOOKUP__
