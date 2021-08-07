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

#ifndef __ENDPOINTDISPOSER__
#define __ENDPOINTDISPOSER__

//	------------------------------	Includes

	#include "OTEndpoint.h"

//	------------------------------	Public Definitions
//	------------------------------	Public Types

	typedef struct NotifierContext
	{
		EndpointDisposer	*disposer;
		PrivateEndpoint		*ep;
	} NotifierContext;

	class EndpointDisposer 
	{
	public:

		EndpointDisposer(OTEndpoint *inEP, NMBoolean inNotifyUser);
		~EndpointDisposer();

		NMErr	DoIt(void);

		static		NMBoolean sLastChance;	// flag telling us the module is about to be unloaded

	protected:

		NMUInt32		mState;
		OTEndpoint 		*mEP;
		NMBoolean		bNotifyUser;
		OTLink			mLink;
		NMSInt32		mTimerTask;
		OTResult		mLastStreamState;
		OTResult 		mLastDatagramState;
		UnsignedWide	mStreamStateStartTime;
		UnsignedWide	mDatagramStateStartTime;
		UnsignedWide	mStartTime;
		NMUInt8			mLock;
		NotifierContext	mStreamContext;
		NotifierContext	mDatagramContext;
		OTProcessUPP	mTimerTaskUPP;

		enum
		{
			kStreamDone = 1,
			kDatagramDone = 2,
			kSentDisconnect = 4,
			kReceivedDisconnect = 8
		};
		
		NMErr	Finish(void);
		NMErr	PrepForClose(PrivateEndpoint *inPrivEP);
		NMErr	Process();
		NMErr	TransitionEP(PrivateEndpoint *inEP);
		NMErr	DoDisconnect(NMBoolean inOrderly);

		static NetNotifierUPP	mNotifier;
		static pascal void		Notifier(void* contextPtr, OTEventCode code, OTResult result, void* cookie);
		static pascal void		TimerTask(void*);
		static void				DoLook(NotifierContext *inDisposer);

	};

#endif // __ENDPOINTDISPOSER__
