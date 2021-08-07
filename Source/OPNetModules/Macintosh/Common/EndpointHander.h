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

#ifndef __ENDPOINTHANDER__
#define __ENDPOINTHANDER__

//	------------------------------	Includes

	#include "OTEndpoint.h"

//	------------------------------	Public Definitions
//	------------------------------	Public Types

	class EndpointHander 
	{
	public:

		EndpointHander(OTEndpoint *inListener, OTEndpoint *inNewEP, TCall *inCall);
		~EndpointHander();

		NMErr		DoIt(void);
		static void		CleanupEndpoints(void);

	protected:

		NMUInt32		mState;
		OTEndpoint 	*mListenerEP;
		OTEndpoint 	*mNewEP;
		TCall		*mCall;
		TBind		mBindReq, mBindRet;
		OTLink		mLink;

		enum
		{
			kGotStreamEP = 1,
			kGotDatagramEP = 2,
			kStreamDone = 4,
			kDatagramDone = 8,
			kStreamRemoteAddr = 16,
			kStreamLocalAddr = 32,
			kDatagramRemoteAddr = 64,
			kDatagramLocalAddr = 128
		};
		
		NMErr	GetStreamEP(void);
		NMErr	GetDatagramEP(void);
		NMErr	Finish(void);
		NMBoolean	ScheduleDelete();
		
		static NetNotifierUPP	mNotifier;
		static pascal void		Notifier(void* contextPtr, OTEventCode code, OTResult result, void* cookie);

	};

//	------------------------------	Public Variables

	extern OTLIFO		*gDeadEndpoints;

#endif
