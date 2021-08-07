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

#include <OpenTptClient.h>

#include "EndpointDisposer.h"
#include "OTUtils.h"

//	------------------------------	Private Definitions
//	------------------------------	Private Types
//	------------------------------	Private Variables

NMBoolean EndpointDisposer::sLastChance;

//	------------------------------	Private Functions

extern "C" void CloseEPComplete(NMEndpointRef inEPRef);
NetNotifierUPP	EndpointDisposer::mNotifier(EndpointDisposer::Notifier);

//	------------------------------	Public Variables


//----------------------------------------------------------------------------------------
// EndpointDisposer::EndpointDisposer
//----------------------------------------------------------------------------------------

EndpointDisposer::EndpointDisposer(OTEndpoint *inEP, NMBoolean inNotifyUser)
{
	DEBUG_ENTRY_EXIT("EndpointDisposer::EndpointDisposer");
	op_assert(inEP != NULL);

	mEP = inEP;
	bNotifyUser = inNotifyUser;
	mState = 0;
	mLastDatagramState = mLastStreamState = 0;
	mTimerTask = 0;
	mLock = 0;

	mStreamContext.disposer = this;
	mStreamContext.ep = inEP->mStreamEndpoint;
	mDatagramContext.disposer = this;
	mDatagramContext.ep = inEP->mDatagramEndpoint;

	mTimerTaskUPP = NewOTProcessUPP(EndpointDisposer::TimerTask);
	op_assert(mTimerTaskUPP);
}

//----------------------------------------------------------------------------------------
// EndpointDisposer::~EndpointDisposer
//----------------------------------------------------------------------------------------

EndpointDisposer::~EndpointDisposer()
{
	DEBUG_ENTRY_EXIT("EndpointDisposer::~EndpointDisposer");

	if (mTimerTask)
	{
		OTCancelTimerTask(mTimerTask);
		OTDestroyTimerTask(mTimerTask);
		mTimerTask = 0;
	}

	//	remove us from the zombie list
	OTEndpoint::sZombieList.RemoveLink(&mLink);

	if (mTimerTaskUPP != NULL)
		DisposeOTProcessUPP(mTimerTaskUPP);
}

//----------------------------------------------------------------------------------------
// EndpointDisposer::DoIt
//----------------------------------------------------------------------------------------

NMErr
EndpointDisposer::DoIt(void)
{
	DEBUG_ENTRY_EXIT("EndpointDisposer::DoIt");

NMErr status;
NMBoolean entered;
	
	//	Add us to the zombie list in case things go to hell and they try to unload the module
	OTEndpoint::sZombieList.AddFirst(&mLink);

	//	We control the vertical.  We control the horizontal.
	if (mEP->mMode & kNMStreamMode)
	{
		op_assert(mEP->mStreamEndpoint);

		if (mEP->mStreamEndpoint->mEP == kOTInvalidEndpointRef)
		{
			mState |= kStreamDone;
		}
		else
		{
			entered = OTEnterNotifier(mEP->mStreamEndpoint->mEP);
			OTRemoveNotifier(mEP->mStreamEndpoint->mEP);

			OTInstallNotifier(mEP->mStreamEndpoint->mEP, mNotifier.fUPP, &mStreamContext);

			if (entered)
				OTLeaveNotifier(mEP->mStreamEndpoint->mEP);
		}
	}
	else
	{
		mState |= kStreamDone;
	}
	
	if (mEP->mMode & kNMDatagramMode)
	{	
		op_assert(mEP->mDatagramEndpoint);

		if (mEP->mDatagramEndpoint->mEP == kOTInvalidEndpointRef)
		{
			mState |= kDatagramDone;
		}
		else
		{
			OTRemoveNotifier(mEP->mDatagramEndpoint->mEP);
			OTInstallNotifier(mEP->mDatagramEndpoint->mEP, mNotifier.fUPP, &mDatagramContext);
		}
	}
	else
	{
		mState |= kDatagramDone;
	}
	 
	//	This endpoint's ass belongs to us now.  All notifications will be through this object
	if (! (mState & kDatagramDone))
		status = PrepForClose(mEP->mDatagramEndpoint);
		
	if (! (mState & kStreamDone))
		status = PrepForClose(mEP->mStreamEndpoint);
		
	//	Create the timer task that will process the ep
	OTGetTimeStamp(&mStartTime); 
	mTimerTask = OTCreateTimerTask(mTimerTaskUPP, this);

	op_vassert_return((mTimerTask != 0),"TimerInit: OTCreateTimerTask returned 0",-100);

	OTScheduleTimerTask(mTimerTask, 50 /*milliseconds */);

	return kNMNoError;
}

//----------------------------------------------------------------------------------------
// EndpointDisposer::Process
//----------------------------------------------------------------------------------------

NMErr
EndpointDisposer::Process()
{
	DEBUG_ENTRY_EXIT("EndpointDisposer::Process");

NMErr	status;
	
	if (! OTAcquireLock(&mLock))	
		return kNMNoError;	// we're already in this funcion, don't reenter
		
	if (! (mState & kDatagramDone))
	{
		status = TransitionEP(mEP->mDatagramEndpoint);
		op_warn(status == kNMNoError);
	}

	if (! (mState & kStreamDone))
	{
		status = TransitionEP(mEP->mStreamEndpoint);
		op_warn(status == kNMNoError);
	}	
	
	if ((mState & kStreamDone) && (mState & kDatagramDone))
	{
		Finish();
	}
	else
	{
		op_warn(mEP->mStreamEndpoint->mInNotifier < 1);
		OTClearLock(&mLock);
	}
	
	return status;
}

//----------------------------------------------------------------------------------------
// EndpointDisposer::DoDisconnect
//----------------------------------------------------------------------------------------

NMErr
EndpointDisposer::DoDisconnect(NMBoolean inOrderly)
{
	DEBUG_ENTRY_EXIT("EndpointDisposer::DoDisconnect");

NMErr		status;
NMBoolean		didEnter = OTEnterNotifier(mEP->mStreamEndpoint->mEP);

	if (inOrderly)
	{
		status = OTSndOrderlyDisconnect(mEP->mStreamEndpoint->mEP);
		DEBUG_NETWORK_API(mEP->mStreamEndpoint->mEP, "OTSndOrderlyDisconnect", status);

		if (status == kOTNotSupportedErr)
		{
			status = OTSndDisconnect(mEP->mStreamEndpoint->mEP, NULL);
			DEBUG_NETWORK_API(mEP->mStreamEndpoint->mEP, "OTSndDisconnect", status);					
		}					
	}
	else
	{
		status = OTSndDisconnect(mEP->mStreamEndpoint->mEP, NULL);
		DEBUG_NETWORK_API(mEP->mStreamEndpoint->mEP, "OTSndDisconnect", status);					
	}

	if (status == kOTNoError)
		mState |= kSentDisconnect;
		
	if (didEnter)
		OTLeaveNotifier(mEP->mStreamEndpoint->mEP);

	return status;
}

//----------------------------------------------------------------------------------------
// EndpointDisposer::TransitionEP
//----------------------------------------------------------------------------------------

NMErr
EndpointDisposer::TransitionEP(PrivateEndpoint *inEP)
{
	DEBUG_ENTRY_EXIT("EndpointDisposer::TransitionEP");

NMErr	status;
NMUInt32		elapsedMilliseconds;
		
	//	if the ep is NULL, just set the flag
	if (inEP->mEP == kOTInvalidEndpointRef)
	{
		if (inEP == mEP->mStreamEndpoint)
			mState |= kStreamDone;
		else
			mState |= kDatagramDone;

		return kNMNoError;
	}
	
	status = OTGetEndpointState(inEP->mEP);

	if (inEP == mEP->mStreamEndpoint)	// we're still waiting for a previous state transition (callback)
	{
		DoLook(&mStreamContext);

		if (mLastStreamState != status)
			OTGetTimeStamp(&mStreamStateStartTime); 
	}

	if (inEP == mEP->mDatagramEndpoint)	// we're still waiting for a previous state transition (callback)
	{
		DoLook(&mDatagramContext);		
		if (mLastDatagramState != status)
			OTGetTimeStamp(&mDatagramStateStartTime); 
	}
	
	if (inEP == mEP->mStreamEndpoint)
		mLastStreamState = status;
	else
		mLastDatagramState = status;
	
	elapsedMilliseconds = OTElapsedMilliseconds(&mStreamStateStartTime);
	
	//	if the module is about to be unloaded, do a last ditch CloseProvider
	if (EndpointDisposer::sLastChance)
		status = T_UNBND;

theSwitch:		
	switch (status)
	{
		case T_DATAXFER:
			DoDisconnect(elapsedMilliseconds < 5000);
			break;	

		case T_OUTREL:
			if (elapsedMilliseconds > 5000)
			{
				op_vpause("in outrel for too long.  Obliterating endpoint");
				status = T_UNBND;
				goto theSwitch;
			}
			break;

		case T_INREL:
			mState |= kReceivedDisconnect;
			DoDisconnect(true);
			break;	

		case T_IDLE:
			status = OTUnbind(inEP->mEP);

			if (status == kNMNoError)	// try just closing the provider if we get an error
				break;

		case T_UNBND:
			status = OTCloseProvider(inEP->mEP);
			DEBUG_NETWORK_API(inEP->mEP, "OTCloseProvider", status);	

			if (status == kNMNoError)
			{
				inEP->mEP = kOTInvalidEndpointRef;
				
				if (inEP == mEP->mStreamEndpoint)
					mState |= kStreamDone;
				else
					mState |= kDatagramDone;
			}
			break;

		case T_OUTCON:
		case T_INCON:
			if (elapsedMilliseconds > 5000)
			{
				DEBUG_PRINT("in state %d for too long.  Obliterating endpoint", status);
				status = T_UNBND;
				goto theSwitch;
			}

			DoDisconnect(false);
			break;

		case T_UNINIT:
			break;

		default:
			DEBUG_PRINT("EP (%p) in unhandled state: %ld", inEP->mEP, status);
			break;
	}
	
	return kNMNoError;
}

//----------------------------------------------------------------------------------------
// EndpointDisposer::PrepForClose
//----------------------------------------------------------------------------------------

NMErr
EndpointDisposer::PrepForClose(PrivateEndpoint *inPrivEP)
{
	DEBUG_ENTRY_EXIT("EndpointDisposer::PrepForClose");

NMErr status;
	
	op_assert(inPrivEP);
	
	status = OTSetNonBlocking(inPrivEP->mEP);
	DEBUG_NETWORK_API(inPrivEP->mEP, "OTSetNonBlocking", status);					

	status = OTSetAsynchronous(inPrivEP->mEP);
	DEBUG_NETWORK_API(inPrivEP->mEP, "OTSetAsynchronous", status);					

	return status;
}

//----------------------------------------------------------------------------------------
// EndpointDisposer::Finish
//----------------------------------------------------------------------------------------

NMErr
EndpointDisposer::Finish(void)
{
	DEBUG_ENTRY_EXIT("EndpointDisposer::Finish");

	if (mTimerTask)
	{
		OTCancelTimerTask(mTimerTask);
		OTDestroyTimerTask(mTimerTask);
		mTimerTask = 0;
	}

	//	allow the endpoint to "finish him!"
	mEP->DoCloseComplete(this, bNotifyUser);
	
	return kNMNoError;
}

//----------------------------------------------------------------------------------------
// EndpointDisposer::DoLook
//----------------------------------------------------------------------------------------

void
EndpointDisposer::DoLook(NotifierContext *inContext)
{
OTResult	result = OTLook(inContext->ep->mEP);
	
	if (result)
		Notifier(inContext, result, 0, 0);
}

//----------------------------------------------------------------------------------------
// EndpointDisposer::Notifier
//----------------------------------------------------------------------------------------

pascal void
EndpointDisposer::Notifier(
	void		*contextPtr,
	OTEventCode code,
	OTResult	result,
	void		*cookie)
{
	DEBUG_ENTRY_EXIT("EndpointDisposer::Notifier");

NMErr			status;
NotifierContext		*context = (NotifierContext *) contextPtr;
EndpointDisposer	*epDisposer = context->disposer;
PrivateEndpoint		*epStuff = context->ep;
	
	op_assert(epDisposer);
	UNUSED_PARAMETER(result);
	UNUSED_PARAMETER(cookie);
	

	switch (code)
	{
		case T_ORDREL:
			op_assert(epStuff == epDisposer->mEP->mStreamEndpoint);
			status = OTRcvOrderlyDisconnect(epStuff->mEP);
			DEBUG_NETWORK_API(epStuff->mEP, "OTRcvOrderlyDisconnect", status);					

		case T_UNBINDCOMPLETE:
			break;

		case T_DATA:
			// Normal data has arrived.  Call OTRcvUData or OTRcv.  Continue reading until
			// the function returns with kOTNoDataErr.
			epDisposer->mEP->HandleData(epStuff);
			break;

		case T_CONNECT:
			// The passive peer has accepted a connection that you requested using the 
			// OTConnect function.  Call the OTRcvConnect function to retrieve any data
			// or option information taht the passive peer has specified when accepting the
			// function or to retrieve the address to which you are actually connected.
			// The cookie parameter to the notifier function is the sndCall parameter that
			//  you specified when calling the OTConnect function
			status = OTRcvConnect(epStuff->mEP, &epDisposer->mEP->mRcvCall);
			DEBUG_NETWORK_API(epStuff->mEP, "OTRcvConnect", status);	
			break;

		case T_DISCONNECT:
			DEBUG_PRINT("got T_DISCONNECT %x", epStuff);
			// A connection has been torn down or rejected.  Use the OTRcvDisconnect 
			//  function to clear the event.
			// If the event is used to signify that the connection has been terminated,
			//  cookie is NULL
			// Else, cookie is the same as the sndCall parameter passed to OTConnect
			status = OTRcvDisconnect(epStuff->mEP, NULL);
			DEBUG_NETWORK_API(epStuff->mEP, "OTRcvDisconnect", status);	
			break;

		case T_DISCONNECTCOMPLETE:
			DEBUG_PRINT("got T_DISCONNECTCOMPLETE %x", epStuff);
			break;

		case T_LISTEN:
			epDisposer->DoDisconnect(false);
			break;

		case T_OPENCOMPLETE:
		case T_UDERR:
			OTRcvUDErr(epStuff->mEP, NULL);

		default:
			break;
	}
}

//----------------------------------------------------------------------------------------
// EndpointDisposer::TimerTask
//----------------------------------------------------------------------------------------

pascal void
EndpointDisposer::TimerTask(void *inTerminator)
{
	DEBUG_ENTRY_EXIT("EndpointDisposer::TimerTask");

EndpointDisposer	*terminator = (EndpointDisposer *) inTerminator;
	
//	if we've been runnning too long, try resetting the states
NMUInt32	elapsedMilliseconds = OTElapsedMilliseconds(&terminator->mStartTime);

	if (elapsedMilliseconds > 5000)
		OTGetTimeStamp(&terminator->mStartTime); 

	terminator->Process();
	
	OTScheduleTimerTask(terminator->mTimerTask, 100);
}
