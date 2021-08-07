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

#include <OpenTptAppleTalk.h>

#include "AppleTalkLookup.h"
#include "OTEnumerators.h"
#include "ZoneList.h"
#include "String_Utils.h"

//	------------------------------	Private Definitions
//	------------------------------	Private Types
//	------------------------------	Private Variables
//	------------------------------	Private Functions
//	------------------------------	Public Variables


//----------------------------------------------------------------------------------------
// COTZonesEnumerator::COTZonesEnumerator
//----------------------------------------------------------------------------------------

COTZonesEnumerator::COTZonesEnumerator()
{
	fZonesList 			= NULL;
	fNotifier			= NULL;
	fNotifierContext	= NULL;
}

//----------------------------------------------------------------------------------------
// COTZonesEnumerator::~COTZonesEnumerator
//----------------------------------------------------------------------------------------

COTZonesEnumerator::~COTZonesEnumerator()
{
	delete fZonesList;
}

//----------------------------------------------------------------------------------------
// COTZonesEnumerator::ZonesListNotifier
//----------------------------------------------------------------------------------------

pascal void
COTZonesEnumerator::ZonesListNotifier(
	void		*contextPtr,
	ATEventCode	code,
	NMErr	result,
	void		*cookie)
{
COTZonesEnumerator	*enumerator = (COTZonesEnumerator *) contextPtr;
	
	if (enumerator->fNotifier != NULL)
	{
		// Notify
		
		// Remap event codes
		if (code == T_GETZONELISTCOMPLETE)
		{
			code = AT_ZONESLIST_COMPLETE;
		}
		else
		{
			code = AT_UNEXPECTED_EVENT;
		}

		// Notify
//		(*enumerator->fNotifier)(enumerator->fNotifierContext, code, result, cookie);
		InvokeOTNotifyUPP(enumerator->fNotifierContext, code, result, cookie, enumerator->fNotifier);
	}
}

//----------------------------------------------------------------------------------------
// COTZonesEnumerator::Initialize
//----------------------------------------------------------------------------------------

NMErr
COTZonesEnumerator::Initialize(
	ATPortRef		portRef,
	OTNotifyUPP		notifier,
	void			*contextPtr)
{
OTPortRef		port	= (OTPortRef)portRef;
OTPortRecord	portRec;
TZoneList*		scanner = NULL;
NMErr		err		= kNMNoError;
	
	if (port == kATAppleTalkDefaultPortRef || OTFindPortByRef(&portRec, port))
	{
	OTConfiguration	*config = (port == kATAppleTalkDefaultPortRef ? kDefaultAppleTalkServicesPath : OTCreateConfiguration(portRec.fPortName));
		
		fZonesList = new TZoneList(config, false);
		
		if (fZonesList != NULL)
		{
			if (notifier != NULL)
			{
				fNotifier = notifier;
				fNotifierContext = contextPtr;
				
				// Pass our own notifier so that we can map event codes
				fZonesList->SetNotifier(ZonesListNotifier, this);
			}
			
			if (fZonesList->ScanForZones() == false)
			{
				// Can't scan
				delete fZonesList;
				fZonesList = NULL;
				
				err = kOTNotFoundErr;
			}
		}
		else
		{
			// Couldn't allocate
			err = kOTOutOfMemoryErr;
		}
	}
	else
	{
		// Can't find port
		err = kOTNotFoundErr;
	}

	return err;
}

//----------------------------------------------------------------------------------------
// COTZonesEnumerator::GetCount
//----------------------------------------------------------------------------------------

NMErr
COTZonesEnumerator::GetCount(NMBoolean* done, NMUInt32* count)
{
NMErr	err = kNMNoError;
	
	*done	= fZonesList->ScanComplete();
	*count	= fZonesList->Count();
	
	return err;
}

//----------------------------------------------------------------------------------------
// COTZonesEnumerator::GetMachineZone
//----------------------------------------------------------------------------------------

NMErr
COTZonesEnumerator::GetMachineZone(StringPtr zoneName)
{
NMErr	err = kNMNoError;
	
	fZonesList->GetLocalZone((char*)zoneName);
	
	return err;
}

//----------------------------------------------------------------------------------------
// COTZonesEnumerator::GetIndexedZone
//----------------------------------------------------------------------------------------

NMErr
COTZonesEnumerator::GetIndexedZone(OneBasedIndex index, StringPtr zoneName)
{
NMErr	err = kNMNoError;

	fZonesList->GetIndexedZone(index - 1, (char*)zoneName);
	
	return err;
}

//----------------------------------------------------------------------------------------
// COTZonesEnumerator::Sort
//----------------------------------------------------------------------------------------

NMErr
COTZonesEnumerator::Sort()
{
NMErr	err = kNMNoError;

	fZonesList->SortList();
	
	return err;
}

//----------------------------------------------------------------------------------------
// COTEntitiesEnumerator::COTEntitiesEnumerator
//----------------------------------------------------------------------------------------

COTEntitiesEnumerator::COTEntitiesEnumerator()
{
	fLookup = NULL;
}

//----------------------------------------------------------------------------------------
// COTZonesEnumerator::~COTEntitiesEnumerator
//----------------------------------------------------------------------------------------

COTEntitiesEnumerator::~COTEntitiesEnumerator()
{
	delete fLookup;
}

//----------------------------------------------------------------------------------------
// COTZonesEnumerator::Initialize
//----------------------------------------------------------------------------------------

NMErr
COTEntitiesEnumerator::Initialize(
	ATPortRef		portRef,
	OTNotifyUPP		notifier,
	void			*contextPtr)
{ 
OTPortRef			port		= (OTPortRef)portRef;
OTPortRecord 		portRec;
NMErr 			err 		= kNMNoError;
		
	do
	{
		if ((port == kATAppleTalkDefaultPortRef) || OTFindPortByRef(&portRec, port))
		{
			OTConfiguration* config = (port == kATAppleTalkDefaultPortRef ? kDefaultAppleTalkServicesPath: OTCreateConfiguration(portRec.fPortName));
			
			fLookup = new AppleTalkLookup();
			
			if (fLookup != NULL)
			{
				err = fLookup->Init(notifier, contextPtr);
				
				if (err != kNMNoError)
					break;
			}
			else
			{
				err = kOTOutOfMemoryErr;
			}
		}
		else
		{
			// Can't find port
			err = kOTNotFoundErr;
		}

	} while (false);

	return err;
}

//----------------------------------------------------------------------------------------
// COTEntitiesEnumerator::StartLookup
//----------------------------------------------------------------------------------------

NMErr
COTEntitiesEnumerator::StartLookup(
	Str32	theZone,
	Str32	theType,
	Str32	thePrefix,
	NMBoolean	clearPreviousResults)
{
NMErr 	err		= kNMNoError;
Str32		prefix;
Str32		type;
			
	if (theZone == (unsigned char*)kATSearchAllZones || theZone[0] != 0)
	{
		// Clear previous results if asked to do so
		if (clearPreviousResults)
			fLookup->Reset();
			
		// Map constants
		if (theType == (unsigned char*)kATSearchAllEntityTypes)
		{
			type[0] = 1;
			type[1] = '=';
		}
		else
		{
			doCopyPStr(theType, type);
		}
		
		if (thePrefix == (unsigned char*)kATSearchAllNames)
		{
			prefix[0] = 1;
			prefix[1] = '=';
		}
		else
		{
			doCopyPStr(thePrefix, prefix);
		}
		
		// Search
		if (theZone == (unsigned char*)kATSearchAllZones)
		{
			//	first do our zone
			err = fLookup->StartLookup(prefix, type, "\p*");

			// Network wide lookup
			err = fLookup->StartSearch(prefix, type);
		}
		else
		{
			// Specific zone lookup
			err = fLookup->StartLookup(prefix, type, theZone);
		}
	}
	else
	{
		err = kATEnumeratorBadZoneErr;
	}
	
	return err;
}

//----------------------------------------------------------------------------------------
// COTEntitiesEnumerator::GetCount
//----------------------------------------------------------------------------------------

NMErr
COTEntitiesEnumerator::GetCount(NMBoolean* allFound, NMUInt32* count)
{
NMErr	err = kNMNoError;
	
	err = fLookup->GetCount(count, allFound);
	
	return err;
}

//----------------------------------------------------------------------------------------
// COTEntitiesEnumerator::GetIndexedEntity
//----------------------------------------------------------------------------------------

NMErr
COTEntitiesEnumerator::GetIndexedEntity(
	OneBasedIndex	index,
	StringPtr		zone,
	StringPtr		type,
	StringPtr		name,
	ATAddress		*address)
{
NMErr	err = kNMNoError;
DDPAddress	ddpAddress;
	
	err = fLookup->GetIndexedItem(index, name, type, zone, &ddpAddress);
	
	if (err == kNMNoError)
	{
		// Return address
		address->fNetwork = ddpAddress.fNetwork;
		address->fNodeID  = ddpAddress.fNodeID;
		address->fSocket  = ddpAddress.fSocket;
	}
	else
	{
		err = kATEnumeratorBadIndexErr;
	}
	
	return err;
}

//----------------------------------------------------------------------------------------
// COTEntitiesEnumerator::Sort
//----------------------------------------------------------------------------------------

NMErr
COTEntitiesEnumerator::Sort()
{
NMErr err = kNMNoError;
	
	fLookup->Sort();
	
	return err;
}

//----------------------------------------------------------------------------------------
// COTEntitiesEnumerator::CancelLookup
//----------------------------------------------------------------------------------------

NMErr
COTEntitiesEnumerator::CancelLookup()
{
	NMErr err = kNMNoError;
	
	fLookup->CancelLookup();
	
	return err;
}
