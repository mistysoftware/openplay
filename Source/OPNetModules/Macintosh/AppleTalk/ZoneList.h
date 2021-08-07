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

#ifndef __ZONELIST__
#define __ZONELIST__

//	------------------------------	Includes

	#include <OpenTransport.h>
	#include <OpenTptAppletalk.h>

//	------------------------------	Public Definitions
//	------------------------------	Public Types

	enum
	{
		kMaxZoneNameSize		= sizeof (Str32),
		kMyZoneNameBufSize		= kMaxZoneNameSize + 4
	};


//#if GENERATINGPOWERPC
	#pragma options align=mac68k
//#endif

	class TZoneList
	{
		public:
					TZoneList(OTConfiguration* config, NMBoolean sortOnScan = true);
					TZoneList(NMBoolean sortOnScan = true);
					~TZoneList();
			
			void	SetNotifier(OTNotifyProcPtr notifier, void* cookie);
					// SetNotifier returns T_GETZONELISTCOMPLETE when scanning is complete
			
			NMBoolean	ScanForZones();
			NMBoolean	ScanComplete();
			NMBoolean	GetLocalZone(char* localZone);
			void		GetIndexedZone(NMSInt32 index, char* zoneName);
			NMBoolean	GetZoneIndex(char* zoneName, NMSInt32* index = NULL);
			void		Reset();
			void		SortList();
			
	inline	NMSInt16	Count() { return fNumZones; }
	inline	NMBoolean	IsSortedOnScan() { return fSortOnScan; }
	inline	void		SetSortOnScan(NMBoolean sortOnScan) { fSortOnScan = sortOnScan; }
			
		private:
			friend	pascal void	ZoneListNotifierProc(void* contextPtr, OTEventCode event, OTResult err, void* cookie);
			
			void	InitZoneList(OTConfiguration* config, NMBoolean sortOnScan);
			void	InitZonesNB();
			NMBoolean	FillZonesNB();
			void	InitMyZoneNameNB();
			void	CountZones();
			void	BuildRefList();
			void	Sort();
		
		private:

			OTConfiguration*	fConfiguration;
			OTNotifyProcPtr		fNotifier;
			void*				fNotifierCookie;
			
			NMSInt16	fNumZones;
			Ptr			fNamePtrList;
			
			char		fMyZoneName[kMyZoneNameBufSize];
			TNetbuf		fZonesNB;
			TNetbuf		fMyZoneNameNB;
			OTBuffer*	fOTBuffer;
			ATSvcRef	fEP;
			OTResult	fErr;
			
			NMBoolean	fBuildingList;
			NMBoolean	fGotZones;
			NMBoolean	fGotMyZoneName;
			NMBoolean	fRefListBuilt;
			NMBoolean	fSorted;
			NMBoolean	fSortOnScan;
	};

//#if GENERATINGPOWERPC
#pragma options align=reset
//#endif


#endif
