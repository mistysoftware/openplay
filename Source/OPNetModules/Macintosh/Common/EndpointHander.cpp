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

#include "EndpointDisposer.h"
#include "EndpointHander.h"
#include "OTUtils.h"
#include "Exceptions.h"
#include "OTEndpoint.h"

//	------------------------------	Private Definitions
//	------------------------------	Private Types
//	------------------------------	Private Variables

OTLIFO			gDeadEPLIFO;
OTLIFO			*gDeadEndpoints = &gDeadEPLIFO;
NetNotifierUPP	EndpointHander::mNotifier(EndpointHander::Notifier);

//	------------------------------	Private Functions
//	------------------------------	Public Variables

//----------------------------------------------------------------------------------------
// EndpointHander::CleanupEndpoints
//----------------------------------------------------------------------------------------

void
EndpointHander::CleanupEndpoints(void)
{
	OTLink			*list = OTLIFOStealList(gDeadEndpoints);
	OTLink			*link;
	EndpointHander	*endpoint;

	while ((link = list) != NULL) {
		list = link->fNext;
		endpoint = OTGetLinkObject(link, EndpointHander, mLink);
		delete endpoint;
	}
}

//----------------------------------------------------------------------------------------
// EndpointHander::ScheduleDelete
//----------------------------------------------------------------------------------------

NMBoolean
EndpointHander::ScheduleDelete()
{
	// add endpoint to cleanup list
	OTLIFOEnqueue(gDeadEndpoints, &mLink);
	return true;
}

//----------------------------------------------------------------------------------------
// EndpointHander::EndpointHander
//----------------------------------------------------------------------------------------

EndpointHander::EndpointHander(OTEndpoint *inListener, OTEndpoint *inNewEP, TCall *inCall)
{
	op_assert(inListener != NULL);
	op_assert(inNewEP != NULL);

	mListenerEP = inListener;
	mNewEP = inNewEP;
	mCall = inCall;
	mState = 0;
	mLink.Init();
}

//----------------------------------------------------------------------------------------
// EndpointHander::~EndpointHander
//----------------------------------------------------------------------------------------

EndpointHander::~EndpointHander()
{
}

//----------------------------------------------------------------------------------------
// EndpointHander::DoIt
//----------------------------------------------------------------------------------------

NMErr
EndpointHander::DoIt(void)
{
	DEBUG_ENTRY_EXIT("EndpointHander::DoIt");
	NMErr	status = kNMNoError;
	
	mNewEP->mState = OTEndpoint::kHandingOff;

	//	Get the remote addr for the new ep from the call structure
	NMBoolean success = OTUtils::CopyNetbuf(&mCall->addr, &mNewEP->mStreamEndpoint->mRemoteAddress);
	mState |= kStreamRemoteAddr;
	
	//Try_
	{
		status = GetStreamEP();
		//ThrowIfOSErr_(status);
		if (status)
			goto error;
		
		status = GetDatagramEP();	// will only get it if we're in uber mode
		//ThrowIfOSErr_(status);
		if (status)
			goto error;

	}
	//Catch_(code)
	error:
	if (status)
	{
		NMErr code = status;
		//	Handle a partial construction
	}
	
	return kNMNoError;
}

//----------------------------------------------------------------------------------------
// EndpointHander::GetStreamEP
//----------------------------------------------------------------------------------------

NMErr
EndpointHander::GetStreamEP(void)
{
	DEBUG_ENTRY_EXIT("EndpointHander::GetStreamEP");

	CachedEP		*streamEP = NULL;
	OTLink			*acceptorLink = NULL;
	NMErr			status = kNMNoError;

	//Try_
	{
		//	Get a cached stream ep
		acceptorLink = OTLIFODequeue(&OTEndpoint::sStreamEPCache.Q);
	
		//if there was an available one in the cache
		if (acceptorLink)
		{
			OTAtomicAdd32(-1, &OTEndpoint::sStreamEPCache.readyCount);
			OTAtomicAdd32(-1, &OTEndpoint::sStreamEPCache.totalCount);
			//jacked up and good to go
			mState |= kGotStreamEP;
			
			//get the cache reloading itself
			OTEndpoint::ServiceEPCaches();
		}
		//otherwise, make one now
		else
		{
			return OTAsyncOpenEndpoint(OTCreateConfiguration(mListenerEP->GetStreamProtocolName()), 0, NULL, mNotifier.fUPP, this);
		}

		//	Get the pointer to the cached ep object
		streamEP = OTGetLinkObject(acceptorLink, CachedEP, link);
		op_assert(streamEP != NULL);
		
		//	Remove the notifier that was being used for cacheing and install the "normal" one
		OTRemoveNotifier(streamEP->ep);
		OTInstallNotifier(streamEP->ep, mNotifier.fUPP, this);
		
		mNewEP->mStreamEndpoint->mEP = streamEP->ep;

		delete streamEP;

	}
	//Catch_(code)
	error:
	if (status)
	{
		NMErr code = status;
		return code;
	}
	
	mState |= kStreamDone;
	
	return kNMNoError;
}

//----------------------------------------------------------------------------------------
// EndpointHander::GetDatagramEP
//----------------------------------------------------------------------------------------

NMErr
EndpointHander::GetDatagramEP(void)
{
	DEBUG_ENTRY_EXIT("EndpointHander::GetDatagramEP");

	CachedEP		*datagramEP = NULL;
	OTLink			*acceptorLink = NULL;
	NMErr		status = kNMNoError;

	//	if we're not in uber mode, there should be no datagram ep
	//	just jump to the notifier
	if (mNewEP->mMode != kNMNormalMode)
	{
		Notifier(this, T_BINDCOMPLETE, kNMNoError, NULL);
		return kNMNoError;
	}
		
	//Try_
	{
		//	Get a datagram endpoint from the cache
		acceptorLink = OTLIFODequeue(&OTEndpoint::sDatagramEPCache.Q);
		if (acceptorLink)
		{
			OTAtomicAdd32(-1, &OTEndpoint::sDatagramEPCache.readyCount);
			OTAtomicAdd32(-1, &OTEndpoint::sDatagramEPCache.totalCount);
			
			//jacked up and good to go
			mState |= kGotDatagramEP;
			
			//get the cache reloading itself
			OTEndpoint::ServiceEPCaches();
		}
		//if we couldn't get one off the cache, we'll make one after we have a stream endpoint
		// (one at a time for simplicity)
		else
		{
			if (mState & kGotStreamEP)
			{
				return OTAsyncOpenEndpoint(OTCreateConfiguration(mListenerEP->GetDatagramProtocolName()), 0, NULL, mNotifier.fUPP, this);
			}
			return kNMNoError; //we just wait for the stream endpoint to get done
		}

		//	Get the pointer to the cached ep object
		datagramEP = OTGetLinkObject(acceptorLink, CachedEP, link);
		op_assert(datagramEP != NULL);
		
		//	Remove the notifier that was being used for cacheing and install the "normal" one
		OTRemoveNotifier(datagramEP->ep);
		OTInstallNotifier(datagramEP->ep, mNotifier.fUPP, this);

		mNewEP->mDatagramEndpoint->mEP = datagramEP->ep;
		
		delete datagramEP;
		
		//if we got both endpoints, go ahead and bind here
		if (mState & kGotStreamEP)
		{
			//	Bind the endpoint.  Execution continues in the notifier for T_BINDCOMPLETE
			mBindReq.addr.len = 0;
			mBindReq.addr.buf = NULL;
			mBindRet.addr = mNewEP->mDatagramEndpoint->mLocalAddress;

			status = OTBind(mNewEP->mDatagramEndpoint->mEP,NULL, &mBindRet);
			//status = OTBind(mNewEP->mDatagramEndpoint->mEP,&mBindReq, &mBindRet);

			DEBUG_NETWORK_API(mNewEP->mDatagramEndpoint->mEP, "OTBind", status);					
			//ThrowIfOSErr_(status);
			if (status)
				goto error;
		}
	}
	//Catch_(code)
	error:
	if (status)
	{
		NMErr code = status;
		return code;
	}
	
	return kNMNoError;
}

//----------------------------------------------------------------------------------------
// EndpointHander::Finish
//----------------------------------------------------------------------------------------

NMErr
EndpointHander::Finish(void)
{
	DEBUG_ENTRY_EXIT("EndpointHander::Finish");

NMErr	status;
		
	OTRemoveNotifier(mNewEP->mStreamEndpoint->mEP);
	OTInstallNotifier(mNewEP->mStreamEndpoint->mEP, mNewEP->mNotifier.fUPP, mNewEP->mStreamEndpoint);

	if (mNewEP->mMode == kNMNormalMode)
	{
		OTRemoveNotifier(mNewEP->mDatagramEndpoint->mEP);
		OTInstallNotifier(mNewEP->mDatagramEndpoint->mEP, mNewEP->mNotifier.fUPP, mNewEP->mDatagramEndpoint);
	}
	
	status = OTAccept(mListenerEP->mStreamEndpoint->mEP, mNewEP->mStreamEndpoint->mEP, mCall);
	DEBUG_NETWORK_API(mListenerEP->mStreamEndpoint->mEP, "OTAccept", status);					

	if (status == kOTLookErr)
	{
		OTResult lookResult = OTLook(mListenerEP->mStreamEndpoint->mEP);

		if (lookResult == T_DISCONNECT)		// the active peer disconnected
			OTRcvDisconnect(mListenerEP->mStreamEndpoint->mEP, NULL);
			
		//	fake the accept complete
		mListenerEP->HandleAcceptComplete();
		
		//	kill the new endpoint
		EndpointDisposer *killer = new EndpointDisposer(mNewEP, true);
	}
	
	if (! ScheduleDelete())
		delete this;		// this is pretty damn likely to crach [gg]
	
	return kNMNoError;
}

//----------------------------------------------------------------------------------------
// EndpointHander::Notifier
//----------------------------------------------------------------------------------------

pascal void
EndpointHander::Notifier(
	void		*contextPtr,
	OTEventCode code,
	OTResult	result,
	void		*cookie)
{
	NMErr status;
	DEBUG_ENTRY_EXIT("EndpointHander::Notifier");
	EndpointHander *epHander = (EndpointHander *) contextPtr;

	UNUSED_PARAMETER(result);

	switch (code)
	{
		case T_OPENCOMPLETE:		// one of our endpoints finished opening
		
			//if we were waiting on the stream ep, move on to the datagram ep if necessary
			if (!(epHander->mState & kGotStreamEP))
			{
				epHander->mNewEP->mStreamEndpoint->mEP = (EndpointRef)cookie;	
				epHander->mState |= kGotStreamEP;
				if (!(epHander->mState & kGotDatagramEP))
				{
					OTAsyncOpenEndpoint(OTCreateConfiguration(epHander->mListenerEP->GetDatagramProtocolName()), 0, NULL, epHander->mNotifier.fUPP, contextPtr);
				}
			}
			//it must be our datagram ep that's done
			else
			{
				epHander->mNewEP->mDatagramEndpoint->mEP = (EndpointRef)cookie;		
				epHander->mState |= kGotDatagramEP;				
			}
			
			//once both eps are done, we go ahead and bind
			if ((epHander->mState & kGotStreamEP) && (epHander->mState & kGotDatagramEP))
			{
				//	Bind the endpoint.  Execution continues in the notifier for T_BINDCOMPLETE
				epHander->mBindReq.addr.len = 0;
				epHander->mBindReq.addr.buf = NULL;
				epHander->mBindRet.addr = epHander->mNewEP->mDatagramEndpoint->mLocalAddress;

				//ECF011114 caused problems under MacOSX...
				status = OTBind(epHander->mNewEP->mDatagramEndpoint->mEP, NULL, &epHander->mBindRet);			
				//status = OTBind(epHander->mNewEP->mDatagramEndpoint->mEP, &epHander->mBindReq, &epHander->mBindRet);
				DEBUG_NETWORK_API(epHander->mNewEP->mDatagramEndpoint->mEP, "OTBind", status);								
			}
			break;
			
		case T_BINDCOMPLETE:		// The datagram endpoint has finished
			epHander->mState |= kDatagramDone;
			
			//	by this time, we should have both endpoints ready to go
			op_assert(epHander->mState & (kStreamDone | kDatagramDone) == (kStreamDone | kDatagramDone));
			
			//	we got the local address from the bind
			epHander->mState |= kDatagramLocalAddr;
			
			//	Now that everything is ready, accept the connection
			epHander->Finish();
			break;

		default:
			break;
	}
}
