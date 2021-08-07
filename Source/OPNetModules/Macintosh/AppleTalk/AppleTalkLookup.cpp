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

//	------------------------------	Includes

#include "AppleTalkLookup.h"
#include "OTUtils.h"
#include "String_Utils.h"

//	------------------------------	Private Definitions


//	------------------------------	Private Types

enum NBPConstants
{
	kMaxRepliesCount		= 256,		// how many replies we expect
	kReplyBufferLength		= kAppleTalkAddressLength,
	kIndexListChunkSize		= 256,		// allocation chunk for the index list
	kRepliesListChunkSize	= 10*1024,	// allocation chunk for the replies list
	kNBPLookupTimeout		= 60*1000,	// milliseconds
	kPreflightPoolSize		= 50*1024	// amount of memory we'd like to have available rrom the notifier
};

//	------------------------------	Private Functions

//	notifier for asynchronous LookupName.  this method simply calls the AppleTalkLookup::Notify method,
//	passing the appropriate arguments.

static pascal void AppleTalkLookupNotifierProc(void* contextPtr, OTEventCode code, NMErr theErr, void* cookie);
static void CreateLookup(char* lookup, StringPtr entityName, StringPtr nbpType, StringPtr theZone);

//	------------------------------	Private Variables

static NMSInt16			gInNotifier = 0;
static NetNotifierUPP	gATLookupNotifier(AppleTalkLookupNotifierProc);

//	------------------------------	Public Variables

//----------------------------------------------------------------------------------------
// AppleTalkLookup::AppleTalkLookup
//----------------------------------------------------------------------------------------

AppleTalkLookup::AppleTalkLookup()
{
	fMapper				= NULL;
	fReplyBuffer		= NULL;
	fNotifier			= NULL;
	fNotifierContext	= NULL;
	fIndexList			= NULL;
	fIndexListSize		= 0;
	fRepliesList		= NULL;
	fRepliesListEndPtr	= NULL;
	fNextReplyPtr		= NULL;
	fRepliesCount		= 0;
	fLookupString[0]	= 0;
	fErr				= kNMNoError;
	
	// Fields used for searching
	fZonesEnumerator	= NULL;
	fZonesCount			= 0;
	fNextZoneIndex		= 1;
	fLookingForZones	= false;
		
	// Init requests list
	for (NMSInt16 i = 0; i < kConcurrentLookups; i++)
	{
		fReply[i].fBusy = false;
		fReply[i].fReply.names.buf = NULL;
		fReply[i].fReply.names.len = fReply[i].fReply.names.maxlen = 0;
		fReply[i].fReply.rspcount = 0;
		fReply[i].fZone[0] = 0;
		fReply[i].fReplyNumber = i;
	
		fRequest[i].name.buf = NULL;
		fRequest[i].name.len = fRequest[i].name.maxlen = 0;
		fRequest[i].addr.buf = NULL;
		fRequest[i].addr.len = fRequest[i].addr.maxlen = 0;
		fRequest[i].flags = fRequest[i].maxcnt = fRequest[i].timeout = 0;
	}
	
	// Initialize the request hint
	fFreeReplyHint	= 0;
	fPendingLookups	= 0;
}

//----------------------------------------------------------------------------------------
// AppleTalkLookup::~AppleTalkLookup
//----------------------------------------------------------------------------------------

AppleTalkLookup::~AppleTalkLookup()
{
	CloseMapper();
	
	delete []fReplyBuffer;
	
	DeleteIndexList();
	DeleteRepliesList();
	
	if (fZonesEnumerator)
		ATDeleteZonesEnumerator(&fZonesEnumerator);
}

//----------------------------------------------------------------------------------------
// AppleTalkLookup::Init
//----------------------------------------------------------------------------------------

NMErr
AppleTalkLookup::Init(OTNotifyUPP notifier, void* contextPtr)
{
NMErr	status = kNMNoError;	
	
	fNotifier 			= notifier;
	fNotifierContext 	= contextPtr;	
	
	do
	{
	// Preflight memory: force pools to grow now because they may have a hard time growing from notifier
	void* temp = InterruptSafe_alloc(kPreflightPoolSize);

		if (temp)
			InterruptSafe_free(temp);
		
		// the reply buffer contains only room for one reply
		fReplyBuffer = new char[kReplyBufferLength];

		if (fReplyBuffer == NULL)
		{
			status = kOTOutOfMemoryErr;
			break;
		}
		
		// Allocate the buffer that contains the indexes to the replies
		status = CreateIndexList();	

		if (status != kNMNoError)
			break;
			
		// Allocate the buffer that actually stores all the replies
		status = CreateRepliesList();

		if (status != kNMNoError)
			break;
		
		// Open the Mapper		
		status = OpenMapper();
		
		if (status != kNMNoError)
		{
			break;
		}
		
	} while (false);
	
	return status;
}

//----------------------------------------------------------------------------------------
// AppleTalkLookup::StartLookup
//----------------------------------------------------------------------------------------

NMErr
AppleTalkLookup::StartLookup(Str32 name, Str32 type, Str32 zone)
{
NMErr	status = kNMNoError;
	
	do
	{
		if (fMapper == NULL)
		{
			status = kOTOutOfMemoryErr;
			break;
		}
		
		
		// Find a free lookup request
		TLookupRequest* request;
		TMyLookupReply*	reply;
		
		status = GetFreeLookup(&request, &reply);
		
		if (status != kNMNoError)
		{
			// No request available at this time
			break;
		}
		
		// prepare the lookup request
		::CreateLookup(fLookupString, name, type, zone);

		// Prepare the request: mark if busy and remember the zone
		
		// Prepare the actual lookup for OT
		request->name.maxlen		= 0;
		request->name.len 			= strlen(fLookupString);
		request->name.buf 			= (NMUInt8*)fLookupString;
		
		request->addr.maxlen		= sizeof(DDPAddress);
		request->addr.len			= 0;
		request->addr.buf 			= (NMUInt8*)&fDDPAddress;
		
		request->maxcnt 			= kMaxRepliesCount;
		request->timeout 			= kNBPLookupTimeout;
		request->flags 				= 0;
		
		// Prepare the reply buffer
		reply->fReply.names.maxlen	 = kReplyBufferLength;
		reply->fReply.names.len 	= 0;
		reply->fReply.names.buf 	= (NMUInt8*)fReplyBuffer;
		reply->fReply.rspcount		= 0;

		reply->fBusy				= true;
		doCopyPStr(zone, reply->fZone);
		
		// Start the lookup
		status = fMapper->LookupName(request, &reply->fReply);
		
		if (status != kNMNoError)
		{
			reply->fBusy = false;
			break;
		}
		else
		{
			fPendingLookups++;		
		}

	} while (false);
	
	return status;
}

//----------------------------------------------------------------------------------------
// AppleTalkLookup::StartSearch
//----------------------------------------------------------------------------------------

NMErr
AppleTalkLookup::StartSearch(Str32 name, Str32 type)
{
NMErr	err;
	
	doCopyPStr(name, fSearchName);
	doCopyPStr(type, fSearchType);
	
	fLookingForZones = true;
	
	err = ATCreateZonesEnumerator(kATAppleTalkDefaultPortRef, gATLookupNotifier.fUPP, this, &fZonesEnumerator);

	if (err != kNMNoError)
		fLookingForZones = false;

	return err;
}

//----------------------------------------------------------------------------------------
// AppleTalkLookup::SearchNextZones
//----------------------------------------------------------------------------------------

void
AppleTalkLookup::SearchNextZones()
{
NMErr	err			= kNMNoError;
Str32		zoneName;
Str32		myZone;
	
	 err = ATGetMachineZone(fZonesEnumerator, myZone);

	if (err != kNMNoError)
	{
		DEBUG_PRINT("Couldn't determine our zone.  Err %d", err);
		return;
	}
	
	//	first search our zone, then search everyone else's
	while (err == kNMNoError && HasMoreZonesToSearch())
	{	
		ATGetIndexedZone(fZonesEnumerator, fNextZoneIndex, zoneName);
		
		if (EqualString(zoneName, myZone, true, true))
			err = kNMNoError;
		else
			err = StartLookup(fSearchName, fSearchType, zoneName);

		if (err == kNMNoError)
			fNextZoneIndex++;
		else
			DEBUG_PRINT("Problem with lookup.  Err %d", err);
	}
}

//----------------------------------------------------------------------------------------
// AppleTalkLookup::HasMoreZonesToSearch
//----------------------------------------------------------------------------------------

NMBoolean
AppleTalkLookup::HasMoreZonesToSearch()
{
	return fNextZoneIndex <= fZonesCount;
}

//----------------------------------------------------------------------------------------
// AppleTalkLookup::Reset
//----------------------------------------------------------------------------------------

void
AppleTalkLookup::Reset()
{
	DeleteRepliesList();
	DeleteIndexList();
	
	CreateRepliesList();
	CreateIndexList();
}

//----------------------------------------------------------------------------------------
// AppleTalkLookup::GetFreeLookup
//----------------------------------------------------------------------------------------

NMErr
AppleTalkLookup::GetFreeLookup(TLookupRequest** request, TMyLookupReply** reply)
{
	*request	= NULL;
	*reply		= NULL;
	
	if (fReply[fFreeReplyHint].fBusy == false)
	{
		// Use hint
		*request	= &fRequest[fFreeReplyHint];
		*reply		= &fReply[fFreeReplyHint];
		
		if (fFreeReplyHint < kConcurrentLookups)
			fFreeReplyHint++;
		else
			fFreeReplyHint = 0;
	}
	else
	{
		// Search for free request
		for (NMSInt16 i = 0; *reply == NULL && i < kConcurrentLookups; i++)
		{
			if (fReply[i].fBusy == false)
			{
				*request	= &fRequest[i];
				*reply		= &fReply[i];
			}
		}
	}
	
	return *request == NULL || *reply == NULL ? kOTOutOfMemoryErr : kNMNoError;
}

//----------------------------------------------------------------------------------------
// AppleTalkLookup::GetCount
//----------------------------------------------------------------------------------------

NMErr
AppleTalkLookup::GetCount(NMUInt32* count, NMBoolean* done)
{
	*done = HasPendingLookups() == false && (fZonesEnumerator == NULL || fLookingForZones == false);
	*count = fRepliesCount;
	
	return fErr;
}

//----------------------------------------------------------------------------------------
// AppleTalkLookup::GetIndexedItem
//----------------------------------------------------------------------------------------

NMErr
AppleTalkLookup::GetIndexedItem(OneBasedIndex index, Str32 name, Str32 type, Str32 zone, DDPAddress* address)
{
NMErr	err = kNMNoError;
	
	if (index < 1 || index >  fRepliesCount)
	{
		name[0] = 0;
		type[0] = 0;
		zone[0] = 0;
		address->Init(0, 0, 0);
		
		err = -1;
	}
	else
	{
	NMUInt8	*entry = fIndexList[index - 1];
		
		// Entry format is: entity name | entity type | zone name | address (every bit of data in long aligned)
		doCopyPStr(entry, name);
		
		entry += *entry + 1;
		ALIGNLONG(entry);
		doCopyPStr(entry, type);
		
		entry += *entry + 1;
		ALIGNLONG(entry);
		doCopyPStr(entry, zone);
		
		entry += *entry + 1;
		ALIGNLONG(entry);
		machine_move_data(entry, address, sizeof(DDPAddress));
	}
	
	return err;
}

//----------------------------------------------------------------------------------------
// AppleTalkLookup::SetNotifier
//----------------------------------------------------------------------------------------

void
AppleTalkLookup::SetNotifier(OTNotifyUPP proc, void *cookie)
{
	fNotifier			= proc;
	fNotifierContext	= cookie; 	
}

//----------------------------------------------------------------------------------------
// AppleTalkLookup::CancelLookup
//----------------------------------------------------------------------------------------

void
AppleTalkLookup::CancelLookup()
{
	CloseMapper();

	for (NMSInt16 i = 0; i < kConcurrentLookups; i++)
	{
		fReply[i].fBusy = false;
	}
	
	OpenMapper();
}

//----------------------------------------------------------------------------------------
// AppleTalkLookup::Notify
//----------------------------------------------------------------------------------------

void
AppleTalkLookup::Notify(OTEventCode event, NMErr theErr, void* cookie)
{
	//	if there was no data returned from the lookup
	//	we just turn it into a lookup complete event.
	
	if (theErr == kOTNoDataErr)
	{
		event = T_LKUPNAMECOMPLETE;
		theErr = kNMNoError;
	}
	
	switch (event)
	{
		case AT_ZONESLIST_COMPLETE:
			//
			//	We are searching and the list of zones is built
			//
			{
			NMBoolean dummy;
			
			theErr = ATGetZonesCount(fZonesEnumerator, &dummy, &fZonesCount);
			
			fLookingForZones = false;
			}
			break;
			
		case T_LKUPNAMECOMPLETE:
			//
			//	the lookup has completed
			//
			{
			TMyLookupReply*	reply = (TMyLookupReply*)cookie;
			
			// Mark the reply free and set the hint to it
			reply->fBusy = false;
			fFreeReplyHint = reply->fReplyNumber;
			
			fPendingLookups--;	

			// If there is no reply in the buffer this is the end
			if (reply->fReply.rspcount == 0)
				break;
			}
			
			//	we fall through to handle the special case where OT puts
			//	data in the buffer, but doesn't issue a T_LKUPNAMERESULT, only a T_LKUPNAMECOMPLETE
			
		
		case T_LKUPNAMERESULT:
			//	
			//	OT has put data into our buffer
			//
			//  Copy it to our list in the form: name | type | zone | address
			//	where name, type and zone are pascal strings and address is a DDDPAddress
			//
			{
				TMyLookupReply*		reply = (TMyLookupReply*)cookie;
				
				do {
					// Parse the reply buffer
					NMSInt16 len;
		
					TLookupBuffer*		replyBuffer	= (TLookupBuffer*)fReplyBuffer;
					
					// Let OT parse the reply because of its weird format that we don't even want to know about
					NBPEntity	entity;
					OTSetNBPEntityFromAddress(&entity, replyBuffer->fAddressBuffer + replyBuffer->fAddressLength, replyBuffer->fNameLength);

					// Extract components of the address
					char name[kNBPMaxNameLength + 1];
					char type[kNBPMaxTypeLength + 1];
					
					OTExtractNBPName(&entity, name);
					OTExtractNBPType(&entity, type);

					// Add the new entry
					// Grow lists as needed
					theErr = GrowIndexList(1);
					if (theErr != kNMNoError)
						break;
					
					theErr = GrowRepliesList(RoundLong(name[0] + 1) 
								  + RoundLong(type[0] + 1) 
								  + RoundLong(reply->fZone[0] + 1) 
								  + RoundLong(sizeof(DDPAddress)));
					if (theErr != kNMNoError)
						break;
					
					// Copy this reply
					fIndexList[fRepliesCount] = fNextReplyPtr;

					// Copy name
					doCopyC2PStr(name, fNextReplyPtr);
					fNextReplyPtr += *fNextReplyPtr + 1;
					
					ALIGNLONG(fNextReplyPtr);
					
					// Copy type
					doCopyC2PStr(type, fNextReplyPtr);
					fNextReplyPtr += *fNextReplyPtr + 1;
					
					ALIGNLONG(fNextReplyPtr);
					
					// Copy zone
					len = reply->fZone[0];
					*fNextReplyPtr++ = len;
					machine_move_data(reply->fZone + 1, fNextReplyPtr, len);
					fNextReplyPtr += len;
					
					ALIGNLONG(fNextReplyPtr);
						
					// Copy address
					len = sizeof(DDPAddress);
					machine_move_data(replyBuffer->fAddressBuffer, fNextReplyPtr, len);
					fNextReplyPtr += len;
					
					ALIGNLONG(fNextReplyPtr);

					// Reset the reply buffer
					fRepliesCount++;
				
					// reset the relies buffer
					reply->fReply.rspcount	= 0;
					reply->fReply.names.len = 0;
					
				} while(false);
			}
			break;
		
		
		default:
			break;
	}
	
	// If searching we need to search the next zones
	if (fZonesEnumerator != NULL)
	{
		if ((event == T_LKUPNAMECOMPLETE || event == AT_ZONESLIST_COMPLETE) && HasMoreZonesToSearch())
		{
			SearchNextZones();
		}
		else if (event == AT_ZONESLIST_COMPLETE && fZonesCount == 0)	// there are no zones on our network
		{
		}
		
		
		// For search clients should be notified for each new result and ONE lookup complete
		if ((event == T_LKUPNAMECOMPLETE && HasPendingLookups()) || event == AT_ZONESLIST_COMPLETE)
		{
			event = NULL;
		}
	}
	
	if (theErr != kNMNoError)
	{
		// An error occured:		
		if (theErr == kOTOutOfMemoryErr)
		{
			event	= T_LKUPNAMECOMPLETE;
			theErr	= kNMNoError;
			CloseMapper();
		}

		// Return the error
		fErr = theErr;
	}
	
	if (fNotifier != NULL && event != NULL)
	{
		// remap the event
		switch (event)
		{
			case T_LKUPNAMECOMPLETE:
				event = AT_ENTITIESLIST_COMPLETE;
				break;
				
			case T_LKUPNAMERESULT:
				event = AT_NEW_ENTITIES;
				break;
				
			default:
				event = AT_UNEXPECTED_EVENT;
		}
		
//		(*fNotifier)(fNotifierContext, event, fErr, this);
		InvokeOTNotifyUPP(fNotifierContext, event, fErr, this, fNotifier);
	}
}

//----------------------------------------------------------------------------------------
// AppleTalkLookup::OpenMapper
//----------------------------------------------------------------------------------------

NMErr
AppleTalkLookup::OpenMapper()
{
NMErr	status = kNMNoError;
	
	if (fMapper != NULL)
	{
		CloseMapper();
	}
	
	do
	{
		fMapper = NM_OTOpenMapper(OTCreateConfiguration(kNBPName), 0, &status);
		
		if ((fMapper == NULL) || (status != kNMNoError))
		{
			if (fMapper == NULL)
				status = kOTOutOfMemoryErr;
				
			break;
		}

		status = fMapper->InstallNotifier(gATLookupNotifier.fUPP, this);

		if (status != kNMNoError)
			break;
	
		status = fMapper->SetAsynchronous();
		
		if (status != kNMNoError)
			break;
		
	} while (false);
	

	if (status != kNMNoError)
	{
		CloseMapper();
	}
	
	return status;
}

//----------------------------------------------------------------------------------------
// AppleTalkLookup::CloseMapper
//----------------------------------------------------------------------------------------

void
AppleTalkLookup::CloseMapper()
{
	if (fMapper != NULL)
	{
		fMapper->Close();
		fMapper = NULL;
	}
}

//----------------------------------------------------------------------------------------
// AppleTalkLookup::Sort
//----------------------------------------------------------------------------------------

void
AppleTalkLookup::Sort()
{
	RadixSort(fIndexList, fRepliesCount, 32);
}

//----------------------------------------------------------------------------------------
// AppleTalkLookupNotifierProc
//----------------------------------------------------------------------------------------

static pascal void
AppleTalkLookupNotifierProc(void* contextPtr, OTEventCode code, NMErr theErr, void* cookie)
{
AppleTalkLookup	*theLookup = (AppleTalkLookup *) contextPtr;
	
	gInNotifier ++;					
	theLookup->Notify(code, theErr, cookie);
	gInNotifier --;
}

//----------------------------------------------------------------------------------------
// CreateLookup
//----------------------------------------------------------------------------------------

static void
CreateLookup(char* lookup, StringPtr entityName, StringPtr nbpType, StringPtr theZone)
{
	if (entityName[0] == 0)
	{
		//	an empty name means search for everything.
		entityName[0] = 1;
		entityName[1] = '=';
	}
	
	if (nbpType[0] == 0)
	{
		nbpType[0] = 1;
		nbpType[1] = '=';
	}
	
	if (theZone[0] == 0)
	{
		theZone[0] = 1;
		theZone[1] = '*';
	}
	
	machine_move_data(entityName+1, lookup, entityName[0]);
	lookup[entityName[0]] = ':';
	
	machine_move_data(nbpType+1, lookup + entityName[0] + 1, nbpType[0]);
	lookup[entityName[0] + 1 + nbpType[0]] = '@';
	
	machine_move_data(theZone + 1, lookup + entityName[0] + 1 + nbpType[0] + 1, theZone[0]);
	lookup[entityName[0] + 1 + nbpType[0] + 1 + theZone[0]] = '\0';
}

//----------------------------------------------------------------------------------------
// AppleTalkLookup::CreateIndexList
//----------------------------------------------------------------------------------------

NMErr
AppleTalkLookup::CreateIndexList()
{
NMErr	err = kNMNoError;
	
	fIndexList = (NMUInt8 **) InterruptSafe_alloc(kIndexListChunkSize * sizeof (NMUInt8 *));
	
	if (fIndexList != NULL)
		fIndexListSize	= kIndexListChunkSize;
	else
		err = kOTOutOfMemoryErr;
	
	fRepliesCount = 0;
	
	return err;
}

//----------------------------------------------------------------------------------------
// AppleTalkLookup::DeleteIndexList
//----------------------------------------------------------------------------------------

void
AppleTalkLookup::DeleteIndexList()
{
	if (fIndexList)
		InterruptSafe_free(fIndexList);

	fIndexList		= NULL;
	fIndexListSize	= 0;
}

//----------------------------------------------------------------------------------------
// AppleTalkLookup::GrowIndexList
//----------------------------------------------------------------------------------------

NMErr
AppleTalkLookup::GrowIndexList(NMSInt16 growBy)
{
NMErr	err = kNMNoError;
	
	if (fRepliesCount + growBy - 1 >= fIndexListSize)
	{
		// List is full grow it
		NMUInt8	**newList = (NMUInt8 **) InterruptSafe_alloc((fIndexListSize + kIndexListChunkSize) * sizeof (NMUInt8 *));
		
		if (newList != NULL)
		{
			// Allocated larger list: copy to it and delete smaller list
			machine_move_data(fIndexList, newList, fRepliesCount * sizeof(unsigned char*));
			
			InterruptSafe_free(fIndexList);
			
			fIndexList		= newList;
			fIndexListSize	= fIndexListSize + kIndexListChunkSize;
			
		}
		else
		{
			// Failed to allocate
			err = kOTOutOfMemoryErr;
		}
	}
	
	return err;
}

//----------------------------------------------------------------------------------------
// AppleTalkLookup::CreateRepliesList
//----------------------------------------------------------------------------------------

NMErr
AppleTalkLookup::CreateRepliesList()
{
NMErr	err = kNMNoError;
	
	fRepliesList = (NMUInt8 *) InterruptSafe_alloc(kRepliesListChunkSize);
	
	if (fRepliesList != NULL)
	{
		// Mark end of current buffer
		fRepliesListEndPtr	= (NMUInt8 *) ((NMUInt32) fRepliesList + kRepliesListChunkSize - 4);
		
		// Clear last long to indicate the end of list
		* (NMSInt32 *) fRepliesListEndPtr = NULL;
	}
	else
	{
		// Failed to allocate
		fRepliesListEndPtr = NULL;
		
		err = kOTOutOfMemoryErr;
	}
	
	fNextReplyPtr = fRepliesList;
		
	return err;
}

//----------------------------------------------------------------------------------------
// AppleTalkLookup::GrowIndexList
//----------------------------------------------------------------------------------------

void
AppleTalkLookup::DeleteRepliesList()
{
NMUInt8	*buffer = fRepliesList;
	
	while (buffer != NULL)
	{
	NMUInt8	*nextBuffer = * ((NMUInt8 **) ((NMUInt32) buffer + kRepliesListChunkSize - 4));
		
		InterruptSafe_free(buffer);
		buffer = nextBuffer;
	}
}

//----------------------------------------------------------------------------------------
// AppleTalkLookup::GrowRepliesList
//----------------------------------------------------------------------------------------

NMErr
AppleTalkLookup::GrowRepliesList(NMSInt16 growBy)
{
NMErr	err = kNMNoError;
	
	if (fNextReplyPtr + growBy >= fRepliesListEndPtr)
	{
	// Buffer full: need to grow
	NMUInt8	*newBuffer = (NMUInt8 *) InterruptSafe_alloc(kRepliesListChunkSize);
		
		if (newBuffer != NULL)
		{
			// Got a new buffer: chain it
			* (NMUInt8 **) fRepliesListEndPtr = newBuffer;
			
			// Point to it for the next insertion
			fNextReplyPtr = newBuffer;
			
			// New end pointer is at the end of the new buffer
			fRepliesListEndPtr	= (NMUInt8 *) ((NMUInt32) newBuffer + kRepliesListChunkSize - 4);
			
			// Clear the last long of the buffer to mark it is the end of the list
			* (NMSInt32 *) fRepliesListEndPtr = NULL;
		}
		else
		{
			// Failed to allocate
			err = kOTOutOfMemoryErr;
		}
	}
	
	return err;
}
