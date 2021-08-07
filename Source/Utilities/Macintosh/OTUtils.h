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

#ifndef __OTUTILS__
#define __OTUTILS__

//	------------------------------	Includes

	#ifndef __OPENPLAY__
	#include 			"OpenPlay.h"
	#endif
	#ifndef __OPUTILS__
	#include "OPUtils.h"
	#endif
	#include <OpenTransportProviders.h>


//	------------------------------	Public Types
// gg - 12/12/2000
// this wrapper class is insulates us from "future os changes" where
// creating upps might involve allocating memory.

	class NetNotifierUPP {
		public:
		inline	NetNotifierUPP(OTNotifyProcPtr notifyProc) { fUPP = NewOTNotifyUPP(notifyProc); op_assert(fUPP); }
		inline	~NetNotifierUPP() { if (fUPP != NULL) { DisposeOTNotifyUPP(fUPP); fUPP = NULL; }}
			
			OTNotifyUPP		fUPP;
	};

//-----

	class ScheduleTaskUPP {
		public:
			inline ScheduleTaskUPP(NMProcPtr taskProc) { fUPP = NewNMUPP(taskProc); op_assert(fUPP); }
			inline ~ScheduleTaskUPP() { if (fUPP != NULL) { DisposeNMUPP(fUPP); fUPP = NULL; } }
			
			NMUPP		fUPP;
	};
	
//-----
	
	
	class OTUtils
	{
	public:
		typedef enum {
			kOTDontCheckProtocol,
			kOTCheckForAppleTalk,
			kOTCheckForTCPIP
			
		} OTProtocolTest;
		
		static NMBoolean		HasOpenTransport(void);
		static NMBoolean		HasOpenTransportTCP(void);
		static NMBoolean		HasOpenTransportAppleTalk(void);
		static NMBoolean		OTInitialized(void) { return sOTInitialized; }
		
		static NMErr			StartOpenTransport( OTProtocolTest inProtocolTest, long reserveMemory, long reserveChunkSize);
		static void				CloseOpenTransport(void);
		static NMBoolean		Version11OrLater(void);
		static NMBoolean		Version111OrLater(void);

		static NMErr			DoNegotiateTCPNoDelayOption(EndpointRef ep, NMBoolean optValue);
		static NMErr			DoNegotiateIPReuseAddrOption(EndpointRef ep, NMBoolean reuseState);

		static OTResult			SetFourByteOption(EndpointRef inEndpoint, OTXTILevel inLevel, OTXTIName inName, NMUInt32 inValue);
		static NMErr			MakeInetAddressFromString(const char *addrString,InetAddress *outAddr);
		static DDPNBPAddress	*MakeDDPNBPAddressFromString(char *addrString);
		static NMErr			MakeInetNameFromAddress(const InetHost inHost, InetDomainName ioName);
		static NMBoolean		CopyNetbuf(TNetbuf *inSource, TNetbuf *ioDest);
	private:
		static void				GetOTGestalt();
		
		static NMBoolean		sOTGestaltTested;
		static NMSInt32			sOTGestaltResult;
		static NMSInt32			sOTVersion;
		static NMBoolean		sOTInitialized;
	};

#ifdef OP_PLATFORM_MAC_CARBON_FLAG

	// current ot context
	extern OTClientContextPtr	gOTClientContext;

	// handy carbon ot macros
	#define OTAsyncOpenEndpoint(config, oflag, info, proc, contextPtr)  OTAsyncOpenEndpointInContext(config, oflag, info, proc, contextPtr, gOTClientContext)
	#define OTAsyncOpenMapper(config, oflag, proc, contextPtr) OTAsyncOpenMapperInContext(config, oflag, proc, contextPtr, gOTClientContext)
	#define OTCreateTimerTask(upp, arg) OTCreateTimerTaskInContext(upp, arg, gOTClientContext)
	#define OTOpenEndpoint(config, oflag, info, err)  OTOpenEndpointInContext(config, oflag, info, err, gOTClientContext)
	#define NM_OTOpenMapper(config, oflag, err) OTOpenMapperInContext(config, oflag, err, gOTClientContext)

#else
	#define NM_OTOpenMapper(config, oflag, err) OTOpenMapper(config, oflag, err)

#endif


#endif	// __OTUTILS__

