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

#ifndef __OTIPENUMERATOR__
#define __OTIPENUMERATOR__

//	------------------------------	Includes

	#include <OpenTptInternet.h>

	#include "IPEnumerator.h"
	#include "TPointerArray.h"
	#include "OTUtils.h"

//	------------------------------	Public Definitions
//	------------------------------	Public Types

	class OTIPEnumerator : public IPEnumerator
	{
	public:
				 OTIPEnumerator(NMIPConfigPriv *inConfig, NMEnumerationCallbackPtr inCallback, void *inContext, NMBoolean inActive);
				~OTIPEnumerator();
				
		virtual	NMErr	StartEnumeration(void);
		virtual NMErr	IdleEnumeration(void);
		virtual NMErr	EndEnumeration(void);
		
		virtual	NMErr	SendQuery(void);
		
		NMBoolean		bDataWaiting;

	protected:
		static pascal void	Notifier(void* contextPtr, OTEventCode code, OTResult result, void* cookie);
				NMErr	ReceiveDatagram(void);
				
						void HandleLookErr();
	protected:
		EndpointRef				mEP;
		TPointerArray			mEntityList;
		
		InetAddress				mAddress;
		InetInterfaceInfo		mInterfaceInfo;
		NMBoolean				bGotInterfaceInfo;
		TUnitData				mIncomingData;	
		UnsignedWide			mLastQueryTimeStamp;	// an OT timestamp
		UnsignedWide			mLastIdleTimeStamp;	// an OT timestamp
		NMBoolean				bFirstIdle;
		NMUInt32				mEnumPeriod;	// in milliseconds
		static NetNotifierUPP	mNotifier;

	};

//	------------------------------	Public Variables
//	------------------------------	Public Functions

#endif // __OTIPENUMERATOR__

