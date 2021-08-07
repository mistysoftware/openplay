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

#ifndef __OPENPLAY__
#include "OpenPlay.h"
#endif
#include "OPUtils.h"
#include "machine_lock.h"
#include <time.h>
#include "NSpPrefix.h"

#include "CEndpoint_OP.h"
#include "NSpGame.h"
#include "ERObject.h"
#include "ByteSwapping.h"

//#include <assert.h>


struct OPData
{
		void*		fNext;
		void*		fData;
		NMUInt32	fLen;
};


//	------------------------------	Private Definitions
//	------------------------------	Private Types
//	------------------------------	Private Variables
//	------------------------------	Private Functions
//	------------------------------	Public Variables




//----------------------------------------------------------------------------------------
// CEndpoint::CEndpoint
//----------------------------------------------------------------------------------------
//		Creates a new endpoint

CEndpoint::CEndpoint(NSpGame *inGame)
{
	mGame = inGame;

	bSynchronous = gPolling;
	mOpenPlayEndpoint = kOPInvalidEndpointRef;	
	mEndpointCookie = NULL;

	machine_mem_zero(&mStreamSendInfo, sizeof (SendInfo));	
	machine_mem_zero(&mDatagramSendInfo, sizeof (SendInfo));

	mStreamSendInfo.sendQ = new NMList();
	op_assert(mStreamSendInfo.sendQ);
	
	mStreamSendInfo.sendQ->Init();
	
	mDatagramSendInfo.sendQ = new NMList();
	op_assert(mDatagramSendInfo.sendQ);
	mDatagramSendInfo.sendQ->Init();
		
		
	mConnectionReqCount = 0;
	mPendingJoinConnections = NULL;
	bAcceptConnections = false;
	bConnected = false;
	bClone = false;
	bInitiatedDisconnect = false;

	bReadingBody = false;
	bReadingHeader = false;
	mCurrentMessage = NULL;
	
	mRTT = 0;
	mMaxRTT = 0;
	mThruput = 0;
	mMinThruput = 0;
	mLastSentMessageTimeStamp = 0;
	bDisposing = false;	
}

//----------------------------------------------------------------------------------------
// CEndpoint::CEndpoint
//----------------------------------------------------------------------------------------
//	Creates a new endpoint being handed off

CEndpoint::CEndpoint(NSpGame *inGame, EPCookie *inUnreliableCookie, EPCookie *inCookie)
{
	UNUSED_PARAMETER(inUnreliableCookie);
	
	mGame = inGame;

	//	Initialize everything, just in case
	bSynchronous = gPolling;
	mOpenPlayEndpoint = kOPInvalidEndpointRef;	

	mEndpointCookie = NULL;

	machine_mem_zero(&mStreamSendInfo, sizeof (SendInfo));
	machine_mem_zero(&mDatagramSendInfo, sizeof (SendInfo));

	mStreamSendInfo.sendQ = new NMList();
	op_assert(mStreamSendInfo.sendQ);
	mStreamSendInfo.sendQ->Init();
	
	mDatagramSendInfo.sendQ = new NMList();
	op_assert(mDatagramSendInfo.sendQ);
	mDatagramSendInfo.sendQ->Init();
		
	mPendingJoinConnections = NULL;
	bAcceptConnections = false;
	bConnected = false;
	bClone = false;
	bInitiatedDisconnect = false;
	mConnectionReqCount = 0;
	bDisposing = false;	

	bReadingBody = false;
	bReadingHeader = false;
	mCurrentMessage = NULL;
	
	mRTT = 0;
	mMaxRTT = 0;
	mThruput = 0;
	mMinThruput = 0;


	//	Our reliable endpoint/cookie are set to the one passed in
	mEndpointCookie = inCookie;
	mEndpointCookie->endPointObjectPtr = this;	
	mOpenPlayEndpoint = inCookie->endpointRefOP;
		

	mPendingJoinConnections = NULL;
	bAcceptConnections = false;
	bConnected = true;
	bClone = true;
	bInitiatedDisconnect = false;
	mLastSentMessageTimeStamp = 0;
	bHosting = false;
	
	mRTT = 0;
	mMaxRTT = 0;
	mThruput = 0;
	mMinThruput = 0;
}

//----------------------------------------------------------------------------------------
// CEndpoint::Veto
//----------------------------------------------------------------------------------------

void
CEndpoint::Veto(void *inCookie, NSpMessageHeader *inMessage)
{
NMErr					result;
EPCookie					*theCookie = (EPCookie *) inCookie;
NSp_InterruptSafeListIterator	iter(*mPendingJoinConnections);
NSp_InterruptSafeListMember	*theItem;
NMBoolean					bFound = false;
NMSInt32					messageLength;

#if OTDEBUG
	op_vassert_return((NULL != inCookie),"Veto was passed a NULL cookie.  This should NEVER happen.",);
#endif

	while (iter.Next(&theItem))
	{
		if ((EPCookie *) (((UInt32ListMember *) theItem)->GetValue()) == theCookie)
		{
			mPendingJoinConnections->Remove(theItem);
			bFound = true;
			break;
		}
	}
	
	if (false == bFound)
	{
#if OTDEBUG
		op_vpause("In Veto: Didn't find the endpoint cookie in our list!");
#endif
		return;
	}

	messageLength = inMessage->messageLen;

#if !big_endian	
	SwapHeaderByteOrder(inMessage);		// A join denied message needs only the header byte-swapped.
#endif

	result = ::ProtocolSend(theCookie->endpointRefOP, (void *) inMessage, messageLength, 0);

#if OTDEBUG
	if (result < kNMNoErr)
		DEBUG_PRINT("Send Disconnect message returned %ld in CEndpoint::Veto", result);
#endif

	DEBUG_PRINT("Calling ProtocolCloseEndpt in CEndpoint::Veto");

	result = ::ProtocolCloseEndpoint(theCookie->endpointRefOP, true);
	theCookie->endpointRefOP = NULL;

#if OTDEBUG
	if (kNMNoErr != result)
		DEBUG_PRINT("ProtocolSendOrderlyDisconnect returned %ld in CEndpoint::Veto", result);
#endif

		
	InterruptSafe_free(inCookie);
}

//----------------------------------------------------------------------------------------
// CEndpoint::~CEndpoint
//----------------------------------------------------------------------------------------

CEndpoint::~CEndpoint()
{
	if (mStreamSendInfo.sendQ)
	{
		if (false == mStreamSendInfo.sendQ->IsEmpty())
		{
			SendQItem	*item;
			
			while ((item = (SendQItem *) mStreamSendInfo.sendQ->RemoveFirst()) != NULL)
			{
				delete item;	
			}
		}
		
		delete mStreamSendInfo.sendQ;
	}
	
	if (mDatagramSendInfo.sendQ)
	{
		if (false == mDatagramSendInfo.sendQ->IsEmpty())
		{
			SendQItem	*item;
			
			while ((item = (SendQItem *) mDatagramSendInfo.sendQ->RemoveFirst()) != NULL)
			{
				delete item;	
			}
		}

		delete mDatagramSendInfo.sendQ;
	}
		

	if (mEndpointCookie)
		InterruptSafe_free(mEndpointCookie);
	
	if (mPendingJoinConnections)
		delete mPendingJoinConnections;
	
	//	In case the unbind complete hasn't finished, we need to just close the provider
	while (kOPInvalidEndpointRef != mOpenPlayEndpoint)
	{
#if ASYNCDEBUGGING
	NMErr	status;
	
		status = ::OTGetEndpointState(mOpenPlayEndpoint);
		DEBUG_PRINT("CEndpoint::~CEndpoint: Waiting for unbind to complete.  state is %d", status);
#endif		
	}
	
	if (kOPInvalidEndpointRef != mOpenPlayEndpoint)
	{
		DEBUG_PRINT("Calling ProtocolCloseEndpt in CEndpoint::~CEndpoint");
		::ProtocolCloseEndpoint(mOpenPlayEndpoint, true);
		mOpenPlayEndpoint = NULL;

#if OTDEBUG
		op_vpause("CEndpoint::~CEndpoint: reliable endpoint should have been closed by now, but it's not");
#endif
	}

}

//----------------------------------------------------------------------------------------
// CEndpoint::Close
//----------------------------------------------------------------------------------------

void
CEndpoint::Close()
{
	NMErr		status;
		
	bDisposing = true;
	
	if (bHosting)
		Host(false);

	if (kOPInvalidEndpointRef != mOpenPlayEndpoint)
	{
		DEBUG_PRINT("Calling ProtocolCloseEndpt in CEndpoint::Close");
		status = ::ProtocolCloseEndpoint(mOpenPlayEndpoint, true);

		if (status != kNMNoError)
			status = ::ProtocolCloseEndpoint(mOpenPlayEndpoint, false);
		mOpenPlayEndpoint = NULL;

	}

}

//----------------------------------------------------------------------------------------
// CEndpoint::Init
//----------------------------------------------------------------------------------------

NMErr
CEndpoint::Init(NMUInt32		inConnectionReqCount,
				PConfigRef	inConfig)
{
	NMErr			status = kNMNoError;
	NMOpenFlags		flags;

	while (true)
	{
		if (inConnectionReqCount)
		{
			mConnectionReqCount = inConnectionReqCount;
			mPendingJoinConnections = new NSp_InterruptSafeList();
			//ThrowIfNULL_(mPendingJoinConnections);
			op_assert(mPendingJoinConnections);

			bAcceptConnections = true;
		}

		mEndpointCookie = (EPCookie *) InterruptSafe_alloc(sizeof (EPCookie));
		
		//ThrowIfNULL_(mEndpointCookie);
		op_assert(mEndpointCookie);

		//	Create (and open) the endpoint...

		if ( bAcceptConnections ) 
		{
			flags = kOpenNone;
		} 
		else
		{
			flags = kOpenActive;
		}
 
		//%%	Here's the hook awaiting the ability to set a timeout for connections
		//	in OpenPlay.  We'll uncomment it when this is ready...
// 		if (gEndpointConnectTimeout != 0)
// 			ProtocolSetConnectTimeout(inConfig);
 			
		status = ProtocolOpenEndpoint(	inConfig, 
										Notifier, 
										(void *) mEndpointCookie, 
										&mOpenPlayEndpoint, 
										flags );

		if (status != kNMNoError)
			DEBUG_PRINT("Error returned from ProtocolOpenEndpoint on reliable endpoint: %ld", status);
		if (status)
			break;
				
		//	Set the requested timeout for asynchronous operations on this endpoint...
		status = ProtocolSetTimeout(mOpenPlayEndpoint, gTimeout);
		if (status)
			break;

		//	Record that this is a hosting endpoint, if it is...
		if (inConnectionReqCount > 0)
			this->bHosting = true;

		//	Fill in our cookie info for the notifier
		mEndpointCookie->endPointObjectPtr = this;
		mEndpointCookie->endpointRefOP = mOpenPlayEndpoint;

		//	If we made it here with an active connection attempt, 
		//	then a connection was made...
		if (flags == kOpenActive)
			bConnected = true;
		
		break;
	}

	if (status)
	{
		if (mPendingJoinConnections)
		{
			delete mPendingJoinConnections;
			mPendingJoinConnections = NULL;
		}

		if (mEndpointCookie)
		{
			InterruptSafe_free(mEndpointCookie);
			mEndpointCookie = NULL;
		}

		if (mOpenPlayEndpoint != kOPInvalidEndpointRef)
		{
			DEBUG_PRINT("Calling ProtocolCloseEndpt in CEndpoint::Init");
			::ProtocolCloseEndpoint(mOpenPlayEndpoint, false);
			mOpenPlayEndpoint = kOPInvalidEndpointRef;
		}
	}

	return (status);
}

//----------------------------------------------------------------------------------------
// CEndpoint::Host
//----------------------------------------------------------------------------------------

NMBoolean
CEndpoint::Host(NMBoolean inAdvertise)
{
	NMBoolean prev = bHosting;

	bHosting = inAdvertise;

	return (prev);
}

//----------------------------------------------------------------------------------------
// CEndpoint::Advertise
//----------------------------------------------------------------------------------------

void
CEndpoint::Advertise(NMBoolean inAdvertise)
{
	if( inAdvertise )
		ProtocolStartAdvertising( mOpenPlayEndpoint );
	else
		ProtocolStopAdvertising( mOpenPlayEndpoint );
}

//----------------------------------------------------------------------------------------
// CEndpoint::HandleDisconnectComplete
//----------------------------------------------------------------------------------------

NMErr
CEndpoint::HandleDisconnectComplete(PEndpointRef inEP, EPCookie *inCookie)
{
UNUSED_PARAMETER(inCookie);
UNUSED_PARAMETER(inEP);

	return (kNMNoError);
}

//----------------------------------------------------------------------------------------
// CEndpoint::HandleDisconnectComplete
//----------------------------------------------------------------------------------------
// 	<WIP> - cjd - What happens if we're interrupted during this function?

CEndpoint *
CEndpoint::Clone(void *inCookie)
{
	CEndpoint 					*theEndpoint = NULL;
	EPCookie					*cookie = (EPCookie *) inCookie;
	NSp_InterruptSafeListIterator	iter(*mPendingJoinConnections);
	NSp_InterruptSafeListMember	*theItem;
	NMBoolean					bFound = false;
	NMErr 						status = kNMNoError;
	
	op_vassert_return((inCookie != NULL),"In clone: Someone passed a null cookie!",NULL);
	
	while (true)
	{
		while (iter.Next(&theItem))
		{
			if ((EPCookie *) (((UInt32ListMember *) theItem)->GetValue()) == cookie)
			{
				mPendingJoinConnections->Remove(theItem);
				bFound = true;
				break;
			}
		}
		
		if (false == bFound)
		{
#if OTDEBUG
			op_vpause("In clone: Didn't find the endpoint cookie in our list!");
#endif
			return (NULL);
		}

		theEndpoint =  MakeCopy(cookie);
		if (theEndpoint == NULL)
		{
			status = kNSpMemAllocationErr;
			break;
		}
		break;
	}

	if (status)
	{
#if INTERNALDEBUG
		DEBUG_PRINT("ERROR in Clone: %ld", status);
#endif

		DEBUG_PRINT("Calling ProtocolCloseEndpt in CEndpoint::Clone");
		::ProtocolCloseEndpoint(cookie->endpointRefOP, false);
		cookie->endpointRefOP = NULL;
		
		return (NULL);
	}
	
	return (theEndpoint);
}

//----------------------------------------------------------------------------------------
// CEndpoint::HandleDisconnectComplete
//----------------------------------------------------------------------------------------
//
// 	<WIP> - cjd - What happens if we're interrupted during this function?

NMBoolean
CEndpoint::FindPendingJoin(void *inCookie)
{
	EPCookie					*cookie = (EPCookie *) inCookie;
	NSp_InterruptSafeListIterator	iter(*mPendingJoinConnections);
	NSp_InterruptSafeListMember	*theItem;
	NMBoolean					bFound = false;
	
	op_vassert_return((inCookie != NULL),"In clone: Someone passed a null cookie!",0);
	
	while (iter.Next(&theItem))
	{
		if ((EPCookie *) (((UInt32ListMember *) theItem)->GetValue()) == cookie)
		{
			mPendingJoinConnections->Remove(theItem);
			bFound = true;
			break;
		}
	}
		
	if (false == bFound)
	{
#if OTDEBUG
		op_vpause("In clone: Didn't find the endpoint cookie in our list!");
#endif
	}

	return (bFound);
}

//----------------------------------------------------------------------------------------
// CEndpoint::GetBacklog
//----------------------------------------------------------------------------------------

NMUInt32
CEndpoint::GetBacklog( void )
{
	return( mStreamSendInfo.backlog );
}

//----------------------------------------------------------------------------------------
// CEndpoint::Disconnect
//----------------------------------------------------------------------------------------

NMErr
CEndpoint::Disconnect(NMBoolean orderly)
{
NMErr 	status = kNMNoError;
	
NSp_InterruptSafeListIterator	*iter = NULL;
NSp_InterruptSafeListMember		*theItem;
EPCookie						*theCookie = NULL;
	
	bInitiatedDisconnect = true;

	//	If we have any pending joiners, we disconnect them unceremoniously
	if (mPendingJoinConnections)
	{
		iter = new NSp_InterruptSafeListIterator(*mPendingJoinConnections);

		if (iter)
		{			
			while (iter->Next(&theItem))
			{
				theCookie = (EPCookie *)(((UInt32ListMember *)theItem)->GetValue());

				if (kOPInvalidEndpointRef != theCookie->endpointRefOP)
				{
					DEBUG_PRINT("Calling ProtocolCloseEndpt in CEndpoint::Disconnect (1)");
					status = ::ProtocolCloseEndpoint(theCookie->endpointRefOP, false);
					theCookie->endpointRefOP = NULL;
				}
#if OTDEBUG
					if (status != kNMNoError)
						DEBUG_PRINT("DoDisconnect returned %ld", status);
#endif
			}
		}
		
		delete iter;
		
		//	If this isn't a clone, there shouldn't be anyone currently connected to the reliable endpoint
		if (false == bClone)
			return (kNMNoError);
	}
	
	
	if (kOPInvalidEndpointRef != mOpenPlayEndpoint)
	{
		if (orderly)
		{
			DEBUG_PRINT("Calling ProtocolCloseEndpt in CEndpoint::Disconnect (2)");
			status = ::ProtocolCloseEndpoint(mOpenPlayEndpoint, true);
			mOpenPlayEndpoint = NULL;

#if OTDEBUG
			if (kNMNoError != status)
				DEBUG_PRINT("ProtocolCloseEndpoint returned %ld", status);
#endif
		}
		else
		{
			DEBUG_PRINT("Calling ProtocolCloseEndpt in CEndpoint::Disconnect (3)");
			status = ::ProtocolCloseEndpoint(mOpenPlayEndpoint, false);
			mOpenPlayEndpoint = NULL;

#if OTDEBUG
			if (kNMNoError != status)
				DEBUG_PRINT("ProtocolCloseEndpoint returned %ld", status);

#endif
		}
	}
	else
	{
#if OTDEBUG
		DEBUG_PRINT("mOpenPlayEndpoint is an invalid endpoint.");
#endif
	}

	return (status);
}

//----------------------------------------------------------------------------------------
// CEndpoint::WaitForDisconnect
//----------------------------------------------------------------------------------------

NMErr
CEndpoint::WaitForDisconnect(NMUInt32 inWaitSecs)
{	
NMUInt32	time, timesUp;
NMErr	status = kNSpTimeoutErr;
NMBoolean	state;
	
	time = machine_tick_count();

	timesUp = time + inWaitSecs * MACHINE_TICKS_PER_SECOND;

	state = ::ProtocolIsAlive(mOpenPlayEndpoint);
	
	while ((false != state) && (machine_tick_count() < timesUp))
	{
		state = ::ProtocolIsAlive(mOpenPlayEndpoint);
	}
	
	if (false == state)
		status = kNMNoError;

	
	return (status);
}

//----------------------------------------------------------------------------------------
// CEndpoint::SendMessage
//----------------------------------------------------------------------------------------
//		We assume that all of the header information has already been set
//		up by the time we get here

NMErr
CEndpoint::SendMessage(NSpMessageHeader *inHeader, NMUInt8 *inBody, NSpFlags inFlags, NMBoolean swapIt)
{
	NMErr	 	result = kNMNoError;
	NMUInt32		messageLength;
	
	UNUSED_PARAMETER(inBody);

	op_vassert_return((inHeader != NULL),"inHeader is NULL!",kNSpInvalidParameterErr);


	//	Set the timestamp
	mLastSentMessageTimeStamp = ::GetTimestampMilliseconds() + mGame->GetTimeStampDifferential();
	inHeader->when = mLastSentMessageTimeStamp;


#if !big_endian
	// Do some necessary byte-swapping on little-endian boxes...
	// Whether it had been swapped already or not, the inHeader->when field was just overwritten with a
	// new value (a few lines above).  It needs to be swapped.
	
	if (swapIt)
	{
		messageLength = inHeader->messageLen;
		inHeader->version |= inFlags;
		SwapBytesForSend(inHeader);	// This will byte-swap the header too, but we've already extracted what	
	}								// we need from it above.
	else
	{
		messageLength = SWAP4(inHeader->messageLen);	// Since this field was pre-swapped, must get right value.
		inHeader->when = SWAP4(inHeader->when);			// Since this field is now not swapped, must swap it.
	}	
#else

	UNUSED_PARAMETER(swapIt);

	//	OR the version with the send flags.  Perhaps this should be done outside of this function, since
	//	everyone platform does this, and we can therefore avoid some byteswapping subtleties.  CRT, Sep, 2000
	inHeader->version |= inFlags;

	//	Get the message length...
	messageLength = inHeader->messageLen;

#endif								

	if (mStreamSendInfo.ep == NULL)
		mStreamSendInfo.ep = mOpenPlayEndpoint;

	if (mDatagramSendInfo.ep == NULL)
		mDatagramSendInfo.ep = mOpenPlayEndpoint;

	//	Send it on the endpoint that is correct for the message's importance
	// Send big messages in the stream, too.  A quick and dirty way to get around using datagrams
	// That should probably be fixed at some point
	if (inFlags & kNSpSendFlag_Registered)
	{
		//	If there is a backlog, postpone or return an error
		//	We HAVE to postpone it if there is already a backlog, else things will be out of order!
		if (mStreamSendInfo.backlog > 0)
		{
			op_vpause("CEndpoint::SendMessage - There is a backlog... postponing send.");
			
			if (inFlags & kNSpSendFlag_FailIfPipeFull)
			{
				result = kNSpPipeFullErr;
			}
			else
			{
				result = PostponeSend(&mStreamSendInfo, inHeader);
			}
		}
		else
		{

			//	Do the send
			mStreamSendInfo.sendInProgress = true;

			if (inFlags & kNSpSendFlag_Blocking)
				result = ::ProtocolSend(mOpenPlayEndpoint, (void *) inHeader, messageLength, kNMBlocking);
			else
				result = ::ProtocolSend(mOpenPlayEndpoint, (void *) inHeader, messageLength, 0);
			
				
			if ((kNMFlowErr == result) || ((result > 0) && (result < messageLength)))
			{
				NMUInt32 bytesSent = (kNMFlowErr == result) ? 0 : result;
				
				op_vpause("CEndpoint::SendMessage - Flow Error");

				if (bytesSent)	//	if we sent any, we have to send it all
				{
					op_vpause("CEndpoint::SendMessage - Bytes sent.  Calling PostponeSend...");
					result = PostponeSend(&mStreamSendInfo, inHeader, bytesSent); 		
				}
				else
				{
					if (inFlags & kNSpSendFlag_FailIfPipeFull)
						result = kNSpPipeFullErr;
					else
					{
						op_vpause("CEndpoint::SendMessage - No bytes were sent.  Calling PostponeSend...");
						result = PostponeSend(&mStreamSendInfo, inHeader); 		
					}
				}
			}
			
			mStreamSendInfo.sendInProgress = false;
			
			if (mStreamSendInfo.goData)
				RunQ(&mStreamSendInfo);
		}
		
		if (result > 0)
			result = kNMNoError;
		else
		{
			goto error;
		}
	}
	else
	{			
		//	Do the send
		mDatagramSendInfo.sendInProgress = true;
		
		result = ProtocolSendPacket(mOpenPlayEndpoint, (void *) inHeader, messageLength, 0);

		if (kNMFlowErr == result)
		{
			if ((inFlags & kNSpSendFlag_Junk) || (inFlags & kNSpSendFlag_FailIfPipeFull))
			{
				result = kNSpPipeFullErr;
			}
			else
			{
#if OTDEBUG
				op_vpause("Flow Error");
#endif
				//	We got a flow error.  Q up the message to send later
				result = PostponeSend(&mDatagramSendInfo, inHeader);
			}
		}

		mDatagramSendInfo.sendInProgress = false;

		if (mDatagramSendInfo.goData)
			RunQ(&mDatagramSendInfo);
			
		if (result >= 0)	//?? (> or >= ???)
			result = kNMNoError;
		else
		{
			goto error;
		}
	}	

	if (result)
	{
error:
		DEBUG_PRINT("ERROR in CEndpoint::SendMessage, result = %ld", result);
	}
	
	return (result);
}

//----------------------------------------------------------------------------------------
// CEndpoint::PostponeSend
//----------------------------------------------------------------------------------------

NMErr
CEndpoint::PostponeSend(SendInfo *inInfo, NSpMessageHeader *inData, NMUInt32 inBytesSent, NMBoolean inAddToTail)
{

	SendQItem	*qItem = NULL;
	NMErr	status = kNMNoError;
	NMEndpointMode	endpointMode = kNMModeNone;
	NMErr notifierErr = kNMNoError;

	//	Enter the notifier so we don't get called back when adding to the Q

	if (inInfo == &mStreamSendInfo)
		endpointMode = kNMStreamMode;
	else if (inInfo == &mDatagramSendInfo)
		endpointMode = kNMDatagramMode;
		
	notifierErr = ProtocolEnterNotifier(mOpenPlayEndpoint, endpointMode);
	if (notifierErr)
	{
		op_vpause("CEndpoint::PostponeSend - Got error from ProtocolEnterNotifier.");
		status = notifierErr;
		goto error;
	}

	if (inInfo->backlog > kMaxSendBacklog)
	{
		op_vpause("CEndpoint::PostponeSend - Pipe is full.");
		status = kNSpPipeFullErr;
		goto error;
	}

	op_vpause("CEndpoint::PostponeSend - About to create new SendQItem...");
	
	qItem = new SendQItem( (void *) inData, inBytesSent);
	if (qItem == NULL){
		status = kNSpMemAllocationErr;
		goto error;
	}

	if (inAddToTail)
		inInfo->sendQ->AddLast(qItem);
	else
		inInfo->sendQ->AddFirst(qItem);
		
	inInfo->backlog++;

	if (status)
	{
error:
		DEBUG_PRINT("ERROR in CEndpoint::PostponeSend, result = %ld", status);

		if (notifierErr == kNMNoError)
			ProtocolLeaveNotifier(mOpenPlayEndpoint, endpointMode);

		return( status );
	}
	
	ProtocolLeaveNotifier(mOpenPlayEndpoint, endpointMode);

	return (kNMNoError);
}

//----------------------------------------------------------------------------------------
// CEndpoint::RunQ
//----------------------------------------------------------------------------------------

NMErr
CEndpoint::RunQ(SendInfo *inInfo)
{

	NMErr	result = kNMNoError;
	SendQItem	*theItem;
	NMUInt32	leftToSend;
	char		*dataPointer;
	
	
	if (machine_acquire_lock(&inInfo->QLock))
	{
		if (inInfo->sendQ->IsEmpty())
		{
			machine_clear_lock(&inInfo->QLock);
			return (kNMNoError);
		}
	
		do
		{
			theItem = (SendQItem *) inInfo->sendQ->RemoveFirst();

			if (NULL == theItem)
				continue;
				
			leftToSend = theItem->BytesLeft();
				
			dataPointer = (char *)theItem->mData + theItem->mTotalSent;
			
			if (inInfo == &mStreamSendInfo)
			{
				result = ::ProtocolSend(mOpenPlayEndpoint, (void *) dataPointer, leftToSend, 0);
			
				if (result > 0)
				{
					if (result < leftToSend)
					{
						theItem->AdvanceBuffPtr(result);
						inInfo->sendQ->AddFirst(theItem);	// put it back on the front
						result = kNMFlowErr;
					}
					else
					{
						inInfo->backlog--;
						delete theItem;
					}
				}
				else
				{
					inInfo->sendQ->AddFirst(theItem);	// put it back on the front
				}
			}
			else
			{
				result = ::ProtocolSendPacket(mOpenPlayEndpoint, (void *) dataPointer, leftToSend, 0);

				if (kNMNoError == result)
				{
					inInfo->backlog--;
					delete theItem;
				}
				else
				{
					inInfo->sendQ->AddFirst(theItem);	// put it back on the front
				}
			}
		
		
		} while ((false == inInfo->sendQ->IsEmpty()) && (result >= kNMNoError));
			
		machine_clear_lock(&inInfo->QLock);
	}
	
	if (result > 0)
		result = kNMNoError;

	return (kNMNoError);
}

//----------------------------------------------------------------------------------------
// CEndpoint::HandleGoData
//----------------------------------------------------------------------------------------

NMErr
CEndpoint::HandleGoData(PEndpointRef inEP)
{
NMErr	status = kNMNoError;
	
	if (inEP == mOpenPlayEndpoint)
	{
		if (mStreamSendInfo.sendInProgress)
			mStreamSendInfo.goData = true;
		else
			RunQ(&mStreamSendInfo);
	}
	else 
	{
		if (mDatagramSendInfo.sendInProgress)
			mDatagramSendInfo.goData = true;
		else
			RunQ(&mDatagramSendInfo);
	}
	
	return (status);
}

//----------------------------------------------------------------------------------------
// CEndpoint::Idle
//----------------------------------------------------------------------------------------
void CEndpoint::Idle(void)
{
	//let openplay idle the endpoint - we could also check to see if this is even necessry
	if (mOpenPlayEndpoint)
		ProtocolIdle(mOpenPlayEndpoint);
}

//----------------------------------------------------------------------------------------
// CEndpoint::Notifier
//----------------------------------------------------------------------------------------

void
CEndpoint::Notifier(PEndpointRef		inEndpoint,
					void				*contextPtr,
					NMCallbackCode		code,
					NMErr 				inError,
					void				*inCookie)
{
	NMErr	status;
	
	if (inError < kNMNoError)
		return;
	
	EPCookie *epCookie = (EPCookie *) contextPtr;
	CEndpoint *theEndpoint = epCookie->endPointObjectPtr;

	switch (code)
	{
	
/*
	kNMConnectRequest	= 1,
	kNMDatagramData		= 2,
	kNMStreamData		= 3,
	kNMFlowClear		= 4,
	kNMAcceptComplete	= 5,
	kNMHandoffComplete	= 6,
	kNMEndpointDied		= 7,
	kNMCloseComplete	= 8	*/


		case kNMStreamData:
//			DEBUG_PRINT("Received message on endpoint %ld", inEndpoint);
			status = theEndpoint->DoReceiveStream(inEndpoint, epCookie);
		break;

		case kNMDatagramData:
			status = theEndpoint->DoReceiveDatagram(inEndpoint, epCookie);
		break;

		case kNMConnectRequest:
			status = theEndpoint->HandleNewConnection(inEndpoint, inCookie);
		break;
		
		case kNMHandoffComplete:	
			// Received by listener endpoint.  There is nothing to do.
		break;

		case kNMEndpointDied:
			status = theEndpoint->mGame->HandleEndpointDisconnected(theEndpoint);
		break;
		
		case kNMFlowClear:
			status = theEndpoint->HandleGoData(inEndpoint);
		break;
			
		case kNMAcceptComplete:	// Received by new handed-off endpoint.
			status = theEndpoint->HandleAcceptComplete(inEndpoint, epCookie);
		break;

		case kNMCloseComplete:
			status = theEndpoint->HandleDisconnectComplete(inEndpoint, epCookie);
		break;
									
		default:
		break;

	}
}

#if defined(__MWERKS__)
#pragma mark === Async Notifier Handlers ===
#endif

//----------------------------------------------------------------------------------------
// CEndpoint::HandleNewConnection
//----------------------------------------------------------------------------------------

NMErr
CEndpoint::HandleNewConnection(PEndpointRef inEndpoint, void *inCookie)
{
	NMErr	status;
		
	//	If we're not hosting, reject the connection
	if (bHosting == true)
	{		
		status = HandOffConnection(inEndpoint, inCookie);
		
		if (status != kNMNoError)
		{
			// SysBeep(10);
			// OpenPlayStub
		}

	}
	else
	{
		status = ProtocolRejectConnection(inEndpoint, inCookie);
	}
		
	return (status);
}

//----------------------------------------------------------------------------------------
// CEndpoint::HandOffConnection
//----------------------------------------------------------------------------------------

NMErr
CEndpoint::HandOffConnection(PEndpointRef inEndpoint, void *inCookie)
{
	NMErr	status = kNMNoError;
	EPCookie	*newCookie = NULL;
	
	
	//	Create a new cookie for our endpoint
	newCookie = (EPCookie *) InterruptSafe_alloc(sizeof (EPCookie));
	if (newCookie == NULL){
		status = kNSpMemAllocationErr;
		goto error;
	}

	newCookie->endpointRefOP = (PEndpointRef) inCookie;
	newCookie->endPointObjectPtr = this;
	
	//	Open the endpoint
	status = ::ProtocolAcceptConnection(inEndpoint, inCookie, Notifier, (void *) newCookie);

	if (status)
	{
error:
		//	We need to send a disconnect to the active peer
		DEBUG_PRINT("ERROR in HandOffConnection: %ld", status);
		status = ::ProtocolCloseEndpoint(mOpenPlayEndpoint, false);
		mOpenPlayEndpoint = NULL;

		if (newCookie)
			InterruptSafe_free(newCookie);
			
		return (status);
	}
	
	return (status);
}

//----------------------------------------------------------------------------------------
// CEndpoint::DoReceiveStream
//----------------------------------------------------------------------------------------

NMErr
CEndpoint::DoReceiveStream(PEndpointRef inEndpoint, EPCookie *inCookie)
{
	NMErr				result = kNMNoError;
	NMUInt8				*receiveBuffer;
	unsigned long		bytesToRead;
	NSpMessageHeader	*theHeader;
	NMFlags				flags;
	NMUInt32			time;
	NMErr 				status = kNMNoError;


	while (result == kNMNoError)
	{
		//	Get our buffer for receiving
		if( (bReadingHeader) || (bReadingBody) )
		{					
			receiveBuffer = mCurrentBufPtr;
			bytesToRead = mLeftToRead;
		}
		else	// We are at the beginning of a message...
		{
			mCurrentMessage = mGame->GetFreeERObject();

			if( NULL == mCurrentMessage )
			{
				DEBUG_PRINT("Couldn't receive the message because we ran out of free q elements.");

				status = kNSpFreeQExhaustedErr;	// not best solution, see comments below
				goto error;
			}

			receiveBuffer = (NMUInt8 *) mCurrentMessage->PeekNetMessage();
			bytesToRead = sizeof (NSpMessageHeader);
			
			bReadingHeader = true;
			mCurrentBufPtr = receiveBuffer;
			mLeftToRead = bytesToRead;
			
		}
		
		// Get some data...
		
		result = ::ProtocolReceive(inEndpoint, receiveBuffer, &bytesToRead, &flags);
		
		// If we got some data, then update the buffer pointer and check whether we
		// have reached the end of the header or message...
		
		if (result == kNMNoError) 
		{
			mLeftToRead -= bytesToRead;
			mCurrentBufPtr += bytesToRead;

			if( 0 == mLeftToRead )
			{
				if( bReadingHeader )
				{
					bReadingHeader = false;
					
					// We are on the header/body boundary.  Parse the header
					// to see how many bytes are in the message...
					
					theHeader = mCurrentMessage->PeekNetMessage();					
#if (little_endian)
					SwapHeaderByteOrder(theHeader);
#endif
					if (kVersion10Message == (theHeader->version & kVersionMask))
					{
						//	Set our timestamp for when the message was received
						time = ::GetTimestampMilliseconds();
						mCurrentMessage->SetTimeReceived(time);
					
						// Examine the message length.  If exactly the length of the
						// header itself, then pass the message off to be handled and
						// we are done...
						
						if (theHeader->messageLen == sizeof(NSpMessageHeader))
							mGame->HandleNewEvent(mCurrentMessage, this, inCookie);
						else
						{
							// There is more to read.  Setup for reading the body of
							// the message...
							
							// If the message is bigger than our pre-allocated ER
							// objects, we must get a bigger ER...
							
							if (theHeader->messageLen > mCurrentMessage->GetMaxMessageLen())
							{
								mCurrentMessage = ExchangeForBiggerER(mCurrentMessage);

								if( NULL == mCurrentMessage )
								{
									DEBUG_PRINT("Got a NULL ERObject!");

									status = kNSpMemAllocationErr;
									goto error;

									// Screwed!  We should do something better
									// than returning an error here.
									// Need graceful recovery (with 1 lost message) or
									// to forcibly end the connection so that the game
									// knows that something pathological has happened.
								}	
							
								//	Get a pointer to the message buffer
								receiveBuffer = (NMUInt8 *) mCurrentMessage->PeekNetMessage();

								//	Reset our pointer to the current buffer position
								mCurrentBufPtr = receiveBuffer + sizeof(NSpMessageHeader);

								//	Reset our pointer to the header
								theHeader = (NSpMessageHeader *) receiveBuffer;	

								//	Reset the timestamp
								mCurrentMessage->SetTimeReceived(time);
							}
												
							mLeftToRead = theHeader->messageLen - sizeof(NSpMessageHeader);
							bReadingBody = true;
						}
					}
					else
					{
						status = -1;
						goto error;
					}
				}
				else
				{
					bReadingBody = false;
					mGame->HandleNewEvent(mCurrentMessage, this, inCookie);
				}
	
			}

		}

	}
	
	if (status)
	{
	error:
		// The connection with the remote endpoint is not good.  If this is not
		// a listening endpoint, then just close it.  If it is a listening endpoint,
		// then disconnect the remote peer... not best solution, but better than previous.
		
		if (bHosting)
		{
			// Remove pending join request if there is one...

			NSp_InterruptSafeListIterator	iter(*mPendingJoinConnections);
			NSp_InterruptSafeListMember		*theItem;
			
			while (iter.Next(&theItem))
			{
				if ( ( (EPCookie *) (((UInt32ListMember *) theItem)->GetValue())) -> endpointRefOP == inEndpoint)
				{
					DEBUG_PRINT("Removing pending join connection in CEndpoint::DoReceiveStream");
					mPendingJoinConnections->Remove(theItem);
					break;
				}
			} 
			
			// Close endpoint that received the bogus data...
			if (kOPInvalidEndpointRef != inEndpoint)
			{
				DEBUG_PRINT("Calling ProtocolCloseEndpt in CEndpoint::DoReceiveStream");
				status = ::ProtocolCloseEndpoint(inEndpoint, true);

				if (status != kNMNoError)
				status = ::ProtocolCloseEndpoint(inEndpoint, false);
			}
		}
		else
		{
			DEBUG_PRINT("Calling Close() in CEndpoint::DoReceiveStream");
			Close();
		}
		return (status);
	}

	if (kNMNoDataErr == result)
		result = kNMNoError;

	return (result);
}

//----------------------------------------------------------------------------------------
// CEndpoint::ExchangeForBiggerER
//----------------------------------------------------------------------------------------

ERObject *
CEndpoint::ExchangeForBiggerER(ERObject *inERObject)
{
ERObject	*backupER;
	
	//	First see if we can get a bigger container
	backupER = mGame->GetFreeERObject(inERObject->PeekNetMessage()->messageLen);

	if (NULL == backupER)
		return (NULL);

	//	Now move the header that we've already retrieved into the new buffer				
	machine_move_data((NMUInt8 *) inERObject->PeekNetMessage(), (NMUInt8 *) backupER->PeekNetMessage(), sizeof (NSpMessageHeader));
	
	//	Release the old one, which takes care of its own buffer
	mGame->ReleaseERObject(inERObject);
	
	return (backupER);
}

//----------------------------------------------------------------------------------------
// CEndpoint::DoReceiveDatagram
//----------------------------------------------------------------------------------------

NMErr
CEndpoint::DoReceiveDatagram(PEndpointRef inEndpoint, EPCookie *inCookie)
{
	NMErr			status = kNMNoError;
	NMUInt8				*receiveBuffer;
	NMUInt32			bufSize;
	NMUInt32			bytesToRead;
	ERObject			*theERObject = NULL;
	NSpMessageHeader	*theHeader;
	NMFlags				flags;
	NMUInt32			time;
	NMUInt8				tempBuf[64];
	NMUInt32			leftToRead;
	NMUInt32			dataLength = 64;

	while (status == kNMNoError)
	{
		theERObject = mGame->GetFreeERObject();

		if (NULL == theERObject)
		{
#if OTDEBUG
			op_vpause("Couldn't receive the message because we ran out of free q elements.");
#endif
			
			//	If we're out of memory, we still need to read and discard the incoming data, since we won't be notified again
			while (kNMNoError == status)
			{
				status = ::ProtocolReceivePacket(inEndpoint, tempBuf, &dataLength, &flags);
			}
			
			status = kNSpFreeQExhaustedErr;
			goto error;
		}

		receiveBuffer = (NMUInt8 *) theERObject->PeekNetMessage();
		bufSize = theERObject->GetMaxMessageLen();
		bytesToRead = bufSize; // MVO: for windows the bytestoread must be maximum buffer size before was sizeof (NSpMessageHeader); 

		status = ::ProtocolReceivePacket(inEndpoint, receiveBuffer, &bytesToRead, &flags);
		if (kNMNoError == status)
		{
			theHeader = (NSpMessageHeader *) receiveBuffer;

#if !big_endian
			SwapHeaderByteOrder(theHeader);
#endif
			
			if (kVersion10Message == (theHeader->version & kVersionMask))
			{
				if (theHeader->messageLen > bufSize)
				{
					theERObject = ExchangeForBiggerER(theERObject);
					
					//	Get a pointer to the message buffer
					receiveBuffer = (NMUInt8 *) theERObject->PeekNetMessage();

					//	Find out what's the max size for this buffer
					bufSize = theERObject->GetMaxMessageLen();		

					//	Reset our pointer to the header
					theHeader = (NSpMessageHeader *) receiveBuffer;	
				}
				
				//	Set our timestamp for when the message was received
				time = ::GetTimestampMilliseconds();
				theERObject->SetTimeReceived(time);
				
				if (theHeader->messageLen > bytesToRead)
				{
					leftToRead = theHeader->messageLen - bytesToRead;
					status = ::ProtocolReceivePacket(inEndpoint, receiveBuffer + bytesToRead, &leftToRead, &flags);
				}
									
				if (status == kNMNoError)
				{
					mGame->HandleNewEvent(theERObject, this, inCookie);
				}
				else
				{
					goto error;
				}
			}
			else
			{
				mGame->ReleaseERObject(theERObject);
#if OTDEBUG
				op_vpause("got a bogus datagram");
#endif
			}
		}
		else
		{
			mGame->ReleaseERObject(theERObject);	
		}
	}
	
	if (status)
	{
error:
		return (status);
	}

	if (kNMNoDataErr == status)
		status = kNMNoError;
	
	return (status);
}

//----------------------------------------------------------------------------------------
// CEndpoint::HandleAcceptComplete
//----------------------------------------------------------------------------------------

NMErr
CEndpoint::HandleAcceptComplete(PEndpointRef inEP, EPCookie *inCookie)
{
	UInt32ListMember	*theMember = NULL;
	NMErr			status = kNMNoError;
	
	theMember = new UInt32ListMember((NMUInt32) inCookie);
	if (theMember == NULL)
	{
		status = kNSpMemAllocationErr;
	}
	else
	{
		inCookie->endpointRefOP = inEP;
		mPendingJoinConnections->Append(theMember);	
	}

#if OTDEBUG
	if (status)
		DEBUG_PRINT("ERROR in HandleHandoffComplete: %ld", status);
#endif

	return (status);
}
