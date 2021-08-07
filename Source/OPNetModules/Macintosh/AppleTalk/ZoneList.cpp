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

#include <OpenTransportProtocol.h>

#include "OTUtils.h"
#include "ZoneList.h"
#include "String_Utils.h"

//	------------------------------	Private Definitions
//	------------------------------	Private Types
//	------------------------------	Private Variables

static NetNotifierUPP	gZoneListNotifier(ZoneListNotifierProc);

//	------------------------------	Private Functions
//	------------------------------	Public Variables

//	--------------------	ZoneListNotifierProc


//----------------------------------------------------------------------------------------
// ZoneListNotifierProc
//----------------------------------------------------------------------------------------

pascal void
ZoneListNotifierProc(void* contextPtr, OTEventCode event, OTResult err, void* cookie)
{
TZoneList*	zoneList = (TZoneList*) contextPtr;
	
	zoneList->fErr = err;

	switch (event)
	{
		case T_OPENCOMPLETE:
			if ( err == kNMNoError )
			{
				// End-point open!
				// cookie is the EP
				zoneList->fEP = (ATSvcRef)cookie;

				zoneList->fErr = zoneList->fEP->GetZoneList(&zoneList->fZonesNB);

				if ( zoneList->fErr == kNMNoError )
				{
					zoneList->fErr = zoneList->fEP->GetMyZone(&zoneList->fMyZoneNameNB);

					if ( zoneList->fErr != kNMNoError )
					{
						//	We had an error so we won't be getting our zone name
						zoneList->fGotMyZoneName = true;
					}
				}
				else
				{
					//	We had an error so the list won't be built
					zoneList->fBuildingList = false;
					zoneList->fGotZones = true;
				}
			}
			break;
			
		case T_GETZONELISTCOMPLETE:
			if ( zoneList->fErr == kNMNoError )
			{
				zoneList->fBuildingList = false;	//	we've finished building the list
				zoneList->fGotZones = true;			//	we've finished building the list
			}
			break;
			
		case T_GETMYZONECOMPLETE:
			if ( zoneList->fErr == kNMNoError )
			{
				zoneList->fGotMyZoneName = true;
			}
			break;
			
		default:
			break;
	}


	// Notify client if we failed or got all the information we were expecting
	if ( zoneList->fNotifier != NULL )
	{
		if ( zoneList->fErr != kNMNoError || (zoneList->fGotZones && zoneList->fGotMyZoneName) )
		{
			// We are all done, notify client
			(*zoneList->fNotifier)(zoneList->fNotifierCookie, T_GETZONELISTCOMPLETE, zoneList->fErr, zoneList);
		}
	}
}

//----------------------------------------------------------------------------------------
// TZoneList::TZoneList
//----------------------------------------------------------------------------------------

TZoneList::TZoneList(NMBoolean sortOnScan)
{
	InitZoneList(kDefaultAppleTalkServicesPath, sortOnScan);
}

//----------------------------------------------------------------------------------------
// TZoneList::TZoneList
//----------------------------------------------------------------------------------------

TZoneList::TZoneList(OTConfiguration* config, NMBoolean sortOnScan)
{
	InitZoneList(config, sortOnScan);
}

//----------------------------------------------------------------------------------------
// TZoneList::InitZoneList
//----------------------------------------------------------------------------------------

void
TZoneList::InitZoneList(OTConfiguration* config, NMBoolean sortOnScan)
{
	fNumZones			= 0;
	fNamePtrList		= NULL;
	fZonesNB.buf		= NULL;
	fMyZoneNameNB.buf	= (NMUInt8*) fMyZoneName;
	fOTBuffer			= NULL;
	fEP					= NULL;
	fErr				= kNMNoError;
	fBuildingList		= false;
	fGotZones			= false;
	fGotMyZoneName		= false;
	fRefListBuilt		= false;
	fSorted				= false;
	fSortOnScan			= sortOnScan;
	fConfiguration		= config;
	fNotifier			= NULL;
	fNotifierCookie		= NULL;
}

//----------------------------------------------------------------------------------------
// TZoneList::~TZoneList
//----------------------------------------------------------------------------------------

TZoneList::~TZoneList()
{
	Reset();
}

//----------------------------------------------------------------------------------------
// TZoneList::SetNotifier
//----------------------------------------------------------------------------------------

void
TZoneList::SetNotifier(OTNotifyProcPtr notifier, void* cookie)
{
	fNotifier		= notifier;
	fNotifierCookie = cookie;
}

//----------------------------------------------------------------------------------------
// TZoneList::ScanForZones
//----------------------------------------------------------------------------------------

NMBoolean
TZoneList::ScanForZones()
{
NMBoolean	scanning = true;	// %%% yan - please review
	
	do
	{
		Reset();
		
		fBuildingList = true;
		
		// We have to be super careful with async open endpoint. The notifier may return even before
		// we exit OTAsyncOpen, and set fErr to an error. Therefore we can't set the result of 
		// OTAsyncOpen to fErr directly or we may miss error conditions
#ifndef OP_PLATFORM_MAC_CARBON_FLAG
		NMErr err = OTAsyncOpenAppleTalkServices(fConfiguration, 0, ZoneListNotifierProc, this);
#else
		NMErr err = OTAsyncOpenAppleTalkServicesInContext(fConfiguration, 0, gZoneListNotifier.fUPP, this, gOTClientContext);
#endif

		if ( err != kNMNoError )
		{
			scanning = false;	// %%% yan - please review (should we set fBuildingList to false?)
			fErr = err;
		}
	} while (false);
	
	
	return scanning;	// %%% yan - please review
}

//----------------------------------------------------------------------------------------
// TZoneList::ScanComplete
//----------------------------------------------------------------------------------------

NMBoolean
TZoneList::ScanComplete()
{
NMBoolean	result = false;

	do
	{
		// See if we failed
		if ( fErr != kNMNoError )
		{
			result = true;
				
			if ( fEP != NULL )
			{
				OTCloseProvider(fEP);
				fEP = NULL;
			}
			break;
		}
		
		// see if we completed
		if (fBuildingList == false
		&&	fGotZones == true
		&&	fGotMyZoneName == true)
		{
			result = true;

			// Extract the zones form OT
			if ( FillZonesNB() )
			{
				// Count the zones
				this->CountZones();	
				
				if ( fRefListBuilt == false )
				{
					BuildRefList();
		
					if ( fSortOnScan == true )
					{
						Sort();
					}
				}
			}
			else
			{
				// Couldn't fill the list
				fErr = memFullErr;
			}

			if ( fEP != NULL )
			{
				OTCloseProvider(fEP);
				fEP = NULL;
			}
		}
	} while (false);
	
	return result;
}

//----------------------------------------------------------------------------------------
// TZoneList::GetLocalZone
//----------------------------------------------------------------------------------------

NMBoolean
TZoneList::GetLocalZone(char* localZone)
{
	machine_move_data(fMyZoneName, localZone, fMyZoneNameNB.len);
	localZone[0] = fMyZoneNameNB.len - 1;
	
	return fGotMyZoneName;
}

//----------------------------------------------------------------------------------------
// TZoneList::GetIndexedZone
//----------------------------------------------------------------------------------------

void
TZoneList::GetIndexedZone(NMSInt32 index, char* zoneName)
{
	zoneName[0] = '\0';

	do
	{
		if ( index < 0 || index >= fNumZones )
		{
			break;
		}
			
		if ( fNamePtrList == NULL )
		{
			break;
		}
		
		char*	zoneNamePtr = ((Ptr*)(fNamePtrList))[index];
		doCopyPStr((StringPtr)zoneNamePtr, (StringPtr)zoneName);
	} while (false);
}

//----------------------------------------------------------------------------------------
// TZoneList::GetZoneIndex
//----------------------------------------------------------------------------------------

NMBoolean
TZoneList::GetZoneIndex(char* zoneName, NMSInt32* index)
{
NMBoolean	found = false;

	do
	{
		if ( fNamePtrList == NULL )
		{
			break;
		}

		NMSInt32 i;
		Str255	tempName;
		
		for (i = 0; i < fNumZones; i++)
		{
			GetIndexedZone(i, (char*) tempName);
			if ( CompareString((ConstStr255Param) tempName, (ConstStr255Param) zoneName, NULL) == 0 )
//			if ( IUCompString((ConstStr255Param) tempName, (ConstStr255Param) zoneName) == 0 )
			{
				found = true;

				if ( index != NULL )
					*index = i;
				
				break;
			}
		}
	} while (false);
	
	return found;
}

//----------------------------------------------------------------------------------------
// TZoneList::Reset
//----------------------------------------------------------------------------------------

void
TZoneList::Reset()
{
OTResult	err;
	
	if ( fEP != NULL )
	{
		err = OTCloseProvider(fEP);
		fEP = NULL;
	}
	
	// Release OT buffer
	if ( fOTBuffer != NULL )
		OTReleaseBuffer(fOTBuffer);
		
	// Dispose sorting array
	if ( fNamePtrList != NULL )
	{
		dispose_pointer(fNamePtrList);
		fNamePtrList = NULL;
	}

	// Dispose zones buffer is realy guy
	if ( fZonesNB.buf != NULL && fZonesNB.maxlen != kOTNetbufDataIsOTBufferStar )
		dispose_pointer(fZonesNB.buf);

	InitZonesNB();
	InitMyZoneNameNB();
	
	fNumZones		= 0;
	fErr			= kNMNoError;
	fBuildingList	= false;
	fGotZones		= false;
	fGotMyZoneName	= false;
	fRefListBuilt	= false;
	fSorted			= false;
}

//----------------------------------------------------------------------------------------
// TZoneList::SortList
//----------------------------------------------------------------------------------------

void
TZoneList::SortList()
{
	if ( fSorted == false )
		Sort();
}

//----------------------------------------------------------------------------------------
// TZoneList::InitZonesNB
//----------------------------------------------------------------------------------------

void
TZoneList::InitZonesNB()
{
	fZonesNB.len	= 0;
	fZonesNB.maxlen	= kOTNetbufDataIsOTBufferStar;
	fZonesNB.buf	= (NMUInt8*) &fOTBuffer;
}

//----------------------------------------------------------------------------------------
// TZoneList::FillZonesNB
//----------------------------------------------------------------------------------------

NMBoolean
TZoneList::FillZonesNB()
{
// Extract the zones form OT
size_t		 dataSize = OTBufferDataSize(fOTBuffer);

	fZonesNB.buf	= (NMUInt8 *) new_pointer(dataSize);
	
	if (( fZonesNB.buf != NULL ) && (dataSize != 0))
	{
		// Copy data from OT
		OTBufferInfo info;
		fZonesNB.maxlen = dataSize;
		fZonesNB.len	= dataSize;
		OTInitBufferInfo(&info, fOTBuffer);
		OTReadBuffer(&info, fZonesNB.buf, &fZonesNB.len);
		OTReleaseBuffer(fOTBuffer);
		fOTBuffer = NULL;
	}
	
	return fZonesNB.buf != NULL;
}

//----------------------------------------------------------------------------------------
// TZoneList::InitMyZoneNameNB
//----------------------------------------------------------------------------------------

void
TZoneList::InitMyZoneNameNB()
{
	machine_mem_zero(fMyZoneName, kMyZoneNameBufSize);
	fMyZoneNameNB.maxlen = kMyZoneNameBufSize;
	fMyZoneNameNB.len = 0;
}

//----------------------------------------------------------------------------------------
// TZoneList::CountZones
//----------------------------------------------------------------------------------------

void
TZoneList::CountZones()
{
NMUInt8	*curZoneNamePtr = (NMUInt8 *) fZonesNB.buf;
NMUInt8	*endPtr = curZoneNamePtr + fZonesNB.len;
	
	fNumZones = 0;
	
	while (curZoneNamePtr < endPtr)
	{
		fNumZones++;
		curZoneNamePtr += *curZoneNamePtr + 1;
	}
}

//----------------------------------------------------------------------------------------
// TZoneList::BuildRefList
//----------------------------------------------------------------------------------------

void
TZoneList::BuildRefList()
{
NMErr	err;
	
	do
	{
		if ( fNamePtrList != NULL )
		{
			break;
		}
			
		NMSInt32	namePtrListSize = fNumZones * sizeof(Ptr);
		
		fNamePtrList = new_pointer(namePtrListSize);
		if ( fNamePtrList == NULL || (err = MemError()) != kNMNoError )
		{
			break;
		}
		
		//	Build up our list of pointers to zone names
		NMUInt8	*curZoneNamePtr = (NMUInt8 *) fZonesNB.buf;
		NMUInt8	*endPtr = curZoneNamePtr + fZonesNB.len;
		NMUInt8	**elemPtr = (NMUInt8 **) fNamePtrList;
		
		while (curZoneNamePtr < endPtr)
		{
			*elemPtr = curZoneNamePtr;
			elemPtr++;
			curZoneNamePtr += *curZoneNamePtr + 1;
		}
		
		fRefListBuilt = true;
	} while (false);
}

//----------------------------------------------------------------------------------------
// TZoneList::Sort
//----------------------------------------------------------------------------------------

void
TZoneList::Sort()
{
	do
	{
		if ( fNamePtrList == NULL )
		{
			break;
		}

		// Sort the names
		RadixSort((NMUInt8 **)fNamePtrList, fNumZones, 32);

		fSorted = true;
	} while (false);
}
