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
 */


#ifndef __CENDPOINT__
#define __CENDPOINT__

//	------------------------------	Includes


	#ifndef __OPENPLAY__
	#include 			"OpenPlay.h"
	#endif
	#ifndef __NETMODULE__
	#include 			"NetModule.h"
	#endif
	#include "OPUtils.h"
	#include "machine_lock.h"

#ifndef OP_API_NETWORK_OT
	#include "LinkedList.h"
#endif

	#include "NSp_InterruptSafeList.h"

	#include "NSpProtocolRef.h"

//	------------------------------	Public Definitions


	#define kOPInvalidEndpointRef	((PEndpointRef)0)

	//currently disabled, as exceptions arent working on linux
	/*#define ThrowIfNMErr_(err)											\
		do {															\
			long	__theErr = err;										\
			if (__theErr != kNMNoError) {									\
				Throw_(__theErr);										\
			}															\
		} while (false)
	*/

//	------------------------------	Public Types


	class NSpGame;
	class ERObject;

	typedef enum
	{
		kNormalMode = 1,
		kBuildingMessageMode
	} TEndpointMode;

	typedef struct 
	{
		NMUInt32	messageID;
		NMUInt32	sendersStamp;
		NMUInt32	receiversStamp;
	} RTTStruct;

	typedef struct 
	{
		NMUInt32	messageID;
		NMUInt32	count;
		NMUInt32 	data[120];
	} ThruputStruct;

	class CEndpoint;

	typedef struct
	{
		CEndpoint		*endPointObjectPtr;
		PEndpointRef	endpointRefOP;
	//	TEndpointInfo	info;
	//	OTAddress		*address;
	//	NMUInt32		addrLen;
	//	OTSequence		sequence;
	} EPCookie;

	typedef struct
	{
		PEndpointRef	ep;
		NMUInt32		backlog;
		NMBoolean		sendInProgress;
		NMBoolean		goData;
		NMList			*sendQ;
		machine_lock	QLock;
	} SendInfo;

	class CEndpoint
	{
	public:
		//enum { kReceiveBufferSize = 4096};
		enum { kVersion10Continuation = 0x10100000 };
		enum { kRTTQuery = 0xF0F0F0F0};
		enum { kRTTResponse = 0xF0F0F0F1};
		enum { kThruputQuery = 0xF0F0F000};
		enum { kThruputResponse = 0xF0F0F001};
		enum { kMaxSendBacklog = 5};
		
		CEndpoint(NSpGame *inGame);
		CEndpoint(NSpGame *inGame, EPCookie *inUnreliableCookie, EPCookie *inCookie);
		virtual ~CEndpoint();

				NMErr 	Init(NMUInt32 inConnectionReqCount, PConfigRef inConfig);		
		virtual	NMErr	InitNonAdvertiser(NSpProtocolPriv *inProt) = 0;
				
				NMErr	WaitForDisconnect(NMUInt32 inWaitTicks);
				NMErr	Disconnect(NMBoolean orderly = true);
		virtual	NMBoolean	Host(NMBoolean inAdvertise);
				void		Advertise(NMBoolean inAdvertise);

				NMUInt32	GetRTT(void *inCookie = NULL);
				NMSInt32	GetNormalizedTimeDifferential();

				NMUInt32	GetThruput(void *inCookie = NULL);

				void		HandleRTTReply(RTTStruct *inMessage);
				void		HandleRTTQuery(PEndpointRef inEndpoint, RTTStruct *inMessage);

				void		HandleThruputReply();
				void		HandleThruputQuery(PEndpointRef inEndpoint, ThruputStruct *inMessage);
				
				NMUInt32 GetBacklog( void );
				NMBoolean	FindPendingJoin(void *inCookie);
				CEndpoint	*Clone(void *inCookie);
				void		Veto(void *inCookie, NSpMessageHeader *inMessage);
				
				NMErr	SendMessage(NSpMessageHeader *inHeader, NMUInt8 *inBody, NSpFlags inFlags = 0, NMBoolean swapIt = true);
				NMErr	DoReceive(PEndpointRef inEndpoint, EPCookie *inCookie);
				void		Close(void);
		inline	NMBoolean	IsAlive(void)	{return bConnected;}
		
				void		Idle();
	// Accessors

		inline	PEndpointRef	GetReliableEndpoint(void) {return mOpenPlayEndpoint;}
		
		inline 	NMUInt32	GetMaxRTT(void) {return mMaxRTT;}
		inline 	NMUInt32	GetMinThruput(void) {return mMinThruput;}
		inline	NMBoolean	IsSynchronous(void) { return bSynchronous;}
		inline 	NMUInt32	GetLastMessageSentTimeStamp(void) {return mLastSentMessageTimeStamp;}

		inline	EPCookie *	GetReliableCookie(void) { return (mEndpointCookie); };
		
		static  void 		Notifier(PEndpointRef inEndpoint, void* contextPtr, NMCallbackCode code, NMErr result, void* cookie);

	protected:

	//			Notifier Handlers
				NMErr	HandleNewConnection(PEndpointRef inEndpoint, void *inCookie);
				NMErr 	HandOffConnection(PEndpointRef inEndpoint, void *inCookie);
				NMErr	HandleOpenEndpointComplete(EPCookie *inCookie, PEndpointRef inEP);
				NMErr 	HandleHandoffComplete(EPCookie *inCookie, PEndpointRef inEP);
				NMErr 	HandleAcceptComplete(PEndpointRef inEP, EPCookie *inCookie);
				NMErr	HandlePassconComplete(EPCookie *inCookie, PEndpointRef inEP);
				NMErr	HandleDisconnect(EPCookie *inCookie, PEndpointRef inEndpoint, NMBoolean orderly = true);
				NMErr	HandleDisconnectComplete(PEndpointRef inEP, EPCookie *inCookie);
				NMErr	HandleUnbindComplete(EPCookie *inCookie);
				NMErr	HandleConnectComplete(EPCookie *inCookie);		
				NMErr	HandleGoData(PEndpointRef inEP);
				NMErr	PostponeSend(SendInfo *inInfo, NSpMessageHeader *inData, NMUInt32 inBytesSent = 0, NMBoolean inAddToTail = true);
				NMErr	RunQ(SendInfo *inInfo);

		
				NMErr	DoReceiveStream(PEndpointRef inEndpoint, EPCookie *inCookie);
				NMErr	DoReceiveDatagram(PEndpointRef inEndpoint, EPCookie *inCookie);
				ERObject	*ExchangeForBiggerER(ERObject *inERObject);
		virtual CEndpoint	*MakeCopy(EPCookie *inReliableCookie) = 0;
		
				
		static	void		ReceiveTask(void *inEP);
		
		NSpGame				*mGame;

	//	OT Stuff
		NMBoolean			bSynchronous;
		NMBoolean			bDisposing;
		PEndpointRef		mOpenPlayEndpoint;
		
		EPCookie			*mEndpointCookie;
		
		SendInfo			mStreamSendInfo;
		SendInfo			mDatagramSendInfo;
		
//LR -- Randy's message buffer fix		NMBoolean			bReadingMessage;
		NMBoolean			bReadingHeader;
		NMBoolean			bReadingBody;

		NMUInt8				*mCurrentBufPtr;
		NMUInt32			mLeftToRead;
		ERObject			*mCurrentMessage;
		
	//	TNetbuf				*mAddress;
		NMUInt32			mConnectionReqCount;
		
		NSp_InterruptSafeList	*mPendingJoinConnections;
		NMBoolean			bAcceptConnections;
		NMBoolean			bConnected;
		NMBoolean			bClone;
		NMBoolean			bInitiatedDisconnect;

		NMUInt32			mRTT;
		NMUInt32			mThruput;
		
		NMUInt32			mMaxRTT;
		NMUInt32			mMinThruput;

		NMUInt32			mReceiversStamp;
		NMUInt32			mLastSentMessageTimeStamp;
		//UnsignedWide		mReceiveWait; // It is not used...
		NMSInt32			mReceiveTask;
		
		NMBoolean 			bHosting;
	};

	/*	
	typedef struct TMessageContinuation
	{
		NMUInt32	version;
		NMUInt32	from;
		NMUInt32	id;
		NMUInt32	part;
		NMUInt8	data[kVariableLengthArray];
	//	NMUInt8	data[];
	} TMessageContinuation;
	*/

//	------------------------------	Public Variables


	extern NMUInt32	gEndpointConnectTimeout;
	extern NMUInt32	gTimeout;


#endif // __CENDPOINT__