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

#ifndef __OTENDPOINT__
#define __OTENDPOINT__

//	------------------------------	Includes
	#ifndef __OPENPLAY__
	#include 			"OpenPlay.h"
	#endif
	#ifndef __NETMODULE__
	#include 			"NetModule.h"
	#endif
	#ifdef OP_API_NETWORK_OT
	#include <OpenTransport.h>
	#endif
	#include "Endpoint.h"
	#include "TPointerArray.h"
	//#include "OTUtils.h"

//	------------------------------	Public Definitions
//	------------------------------	Public Types

	class OTEndpoint;

	/* In order to install a notifier (in C++), you need to make sure you have a pointer to the C++
		object to which the OT endpoint belongs.  However, since we manage two OT endpoints
		per OpenPlay endpoint, you need to have some way of specifying in the cookie which
		OT endpoint is which, else you won't know when the callback comes.  We therefore include
		a pointer to "this" in each of our personal endpoint structs */
	class PrivateEndpoint
	{
	public:

		PrivateEndpoint(class OTEndpoint *inOTEP);
		

	public:

		EndpointRef				mEP;					// Open Transport's endpoint
		TNetbuf					mLocalAddress;				// must be alloced by subclass
		TNetbuf					mRemoteAddress;				// must be alloced by subclass
		NMUInt32				mInNotifier;	
		class OTEndpoint		*mOwnerEndpoint;				// There is a good reason for this (see above)

		//	For handling data reads
		OTBuffer				*mBuffer;			// only valid during data callback
		OTBufferInfo			mBufferInfo;			// helps keep track of partial reads
		NMBoolean				mFlowControlOn;
	};

	//	Used for handing off connections
	typedef struct HandoffInfo
	{
		OTEndpoint			*otherEP;			// The handee's ref to the hander, and vice verse
		NMBoolean			gotData;			// required in case a T_DATA comes before the T_PASSCON

	} HandoffInfo;


	// endpoint cache (for handing off connections)
	struct EPCache
	{
		NMSInt32		readyCount;
		NMSInt32		totalCount; //ECF 010927 keep two totals so we dont make excessive endpoints if some arent ready yet
		OTLIFO		Q;
		const char *otConfigStr;

#ifndef __cplusplus	
	};
#else

	public:

		inline EPCache() {readyCount = 0; totalCount = 0; otConfigStr = NULL;}
		
	};
#endif

	typedef struct EPCache EPCache;

// Handoff ep cache entry

//ECF 010927 overrode the allocators to make this interrupt-safe
	class CachedEP
	{
		public:
			//void*		operator new(size_t size);
			//void		operator delete(void* obj);
			EPCache		*destCache;
			EndpointRef	ep;
			OTLink		link;
	};

	class EndpointDisposer;

	class OTEndpoint : public Endpoint
	{
	public:
								OTEndpoint(NMEndpointRef inRef, NMUInt32 inMode = kNMNormalMode);
		virtual 				~OTEndpoint();

		virtual 	NMErr		Initialize(NMConfigRef inConfig) = 0;
		
		virtual		NMErr		Listen(void);
		virtual		NMErr		AcceptConnection(Endpoint *inNewEP, void *inCookie);
		virtual 	NMErr		RejectConnection(void *inCookie);
		virtual 	NMErr		Connect(void);
		virtual		void		Close(void);

		virtual		NMBoolean	FreeAddress(void **outAddress);
		virtual		NMBoolean	GetAddress(NMAddressType addressType, void **outAddress);
		virtual 	NMBoolean 	IsAlive(void);

		virtual 	NMErr		Idle(void);

		virtual		NMErr		FunctionPassThrough(NMType inSelector, void *inParamBlock);

		virtual 	NMErr		SendDatagram(const NMUInt8 * inData, NMUInt32 inSize, NMFlags inFlags);
		virtual 	NMErr		ReceiveDatagram(NMUInt8 * outData, NMUInt32 *outSize, NMFlags *outFlags);
		virtual 	NMErr		Send(const void *inData, NMUInt32 inSize, NMFlags inFlags);
		virtual		NMErr		Receive(void *outData, NMUInt32 *ioSize, NMFlags *outFlags);

		virtual const char 		*GetStreamProtocolName(void) = 0;
		virtual const char		*GetStreamListenerProtocolName(void) = 0;	// because we can use the "tilisten" modifier
		virtual const char 		*GetDatagramProtocolName(void) = 0;
		
		virtual 	NMErr		EnterNotifier(NMUInt32	endpointMode);
		virtual 	NMErr		LeaveNotifier(NMUInt32	endpointMode);
		
		static pascal void 		Notifier(void* contextPtr, OTEventCode code, OTResult result, void* cookie);

		virtual 	NMErr		PreprocessPacket(void);
		
		virtual		void		GetConfigAddress(TNetbuf *outBuf, NMBoolean inActive) = 0;
		virtual		void 		SetConfigAddress(TNetbuf *inBuf) = 0;

	protected:

		virtual		NMErr		BindDatagramEndpoint(NMBoolean inUseConfigAddr);
		virtual		NMErr		BindStreamEndpoint(OTQLen inQLen);
		virtual		NMErr		DoBind(PrivateEndpoint *inEP, TBind *inRequest, TBind *outReturned);
		virtual 	NMErr		Abort(NMBoolean inWaitForCompletion);
		virtual		void		MakeEnumerationResponse(void) = 0;
					void		HandleQuery(void);
		virtual		NMErr		SendOpenConfirmation(void) = 0;
		virtual		NMErr		WaitForOpenConfirmation(void) = 0;
		virtual 	NMErr		HandleAsyncOpenConfirmation(void) = 0;
				
		virtual NMBoolean		AllocAddressBuffer(TNetbuf *ioBuf) = 0;
		virtual void			FreeAddressBuffer(TNetbuf *inBuf) = 0;
		virtual NMBoolean		AddressesEqual(TNetbuf *inAddr1, TNetbuf *inAddr2) = 0;
		virtual	void	 		ResetAddressForUnreliableTransport(OTAddress *inAddress) = 0;
		
		static	NMErr DoLook(PrivateEndpoint *inEP);
		
		//	These functions all deal with OT events
		virtual	void	DoCloseComplete(EndpointDisposer *inEPDisposer, NMBoolean inNotifyUser);
				void	HandleData(PrivateEndpoint *inEPStuff);
				void 	HandleNewConnection(void);
				void 	HandleDisconnect(PrivateEndpoint *inEPStuff);
				void 	HandleOrdRel(PrivateEndpoint *inEPStuff);
				void	HandleOpenEndpointComplete(PrivateEndpoint *inEPStuff, EndpointRef inEP);
				void 	HandleAcceptComplete(void);
				void 	HandlePassConnComplete(PrivateEndpoint *inEPStuff);

	protected:

		PrivateEndpoint		*mStreamEndpoint;			
		PrivateEndpoint		*mDatagramEndpoint;
		TCall				mCall;						// storage for the call data structure
		TCall				mRcvCall;
		NMUInt8				*mEnumerationResponseData;	// Info for enumeration queries
		NMUInt32			mEnumerationResponseLen;

		TUnitData			mRcvUData;					// For receiving datagrams
		TUnitData			mSndUData;					// For receiving datagrams
		
		NMUInt8				*mPreprocessBuffer;
		
		enum {kAsyncCallbackMaxWait = 0x7FFFFFFF};
		enum {kWasUserData = -9000, kWasQuery = -9001, kWasHandled = -9002};	// different types of non-user data
		enum {kWaitingForAsyncCompletion = -10000};
		enum {kPassive = 1, kActive, kClone};
		
	public:

		NMBoolean				bConnectionConfirmed;
		HandoffInfo				mHandoffInfo;

		//	Static q's of endpoints for handing off connections.  This always remains empty 
		//	when there are no lister endpoints
		static	EPCache			sStreamEPCache;
		static	EPCache			sDatagramEPCache;
		
		static	NMBoolean		sWantStreams;
		static	NMBoolean		sWantDatagrams;
		
		static 	NMUInt32		sDesiredCacheSize;

		static OTList			sZombieList;
		static NetNotifierUPP	mNotifier;
		static NetNotifierUPP	mCacheNotifier;
		static ScheduleTaskUPP	mServiceCachesTask;

		static pascal void 	CacheNotifier(void* contextPtr, OTEventCode code, OTResult result, void* cookie);
		static void			ServiceEPCaches(void);
		static NMErr		FillEPCache(EPCache *inCache);
		static void			CacheHandoffEP(CachedEP *inEP);
		
		friend class EndpointHander;
		friend class EndpointDisposer;

	};

//	------------------------------	Public Variables
//	------------------------------	Public Functions

#endif // __OTENDPOINT__

