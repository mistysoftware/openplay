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

#ifndef __NETMODULE__
#include 			"NetModule.h"
#endif

#ifdef OP_API_NETWORK_OT
#include <OpenTptClient.h>
#endif

#include "EndpointDisposer.h"
#include "EndpointHander.h"
#include "NetModuleDefs.h"
#include "NetModuleIP.h"
#include "OTUtils.h"
#include "Exceptions.h"

//	------------------------------	Private Definitions
//	------------------------------	Private Types
//	------------------------------	Private Variables

EPCache			OTEndpoint::sStreamEPCache;
EPCache			OTEndpoint::sDatagramEPCache;
NMBoolean 		OTEndpoint::sWantStreams = false;
NMBoolean 		OTEndpoint::sWantDatagrams = false;
NMUInt32		OTEndpoint::sDesiredCacheSize = 0;
OTList			OTEndpoint::sZombieList;	
NetNotifierUPP	OTEndpoint::mNotifier(OTEndpoint::Notifier);
NetNotifierUPP	OTEndpoint::mCacheNotifier(OTEndpoint::CacheNotifier);

//	------------------------------	Private Functions

extern "C" void CloseEPComplete(NMEndpointRef inEPRef);

//	------------------------------	Public Variables

#pragma mark === CachedEP ===


//----------------------------------------------------------------------------------------
// CachedEP::operator new
//----------------------------------------------------------------------------------------

//void*	CachedEP::operator new(size_t size)
//{
//	return InterruptSafe_alloc(size);
//}

//----------------------------------------------------------------------------------------
// CachedEP::operator delete
//----------------------------------------------------------------------------------------

//void	CachedEP::operator delete(void *obj)
//{
//	InterruptSafe_free(obj);
//}

#pragma mark === PrivateEndpoint ===

//----------------------------------------------------------------------------------------
// PrivateEndpoint::PrivateEndpoint
//----------------------------------------------------------------------------------------

PrivateEndpoint::PrivateEndpoint(class OTEndpoint *inEP)
{
	op_assert(inEP);
	
	mFlowControlOn = false;
	mInNotifier = 0;
	mEP = kOTInvalidEndpointRef;
	mOwnerEndpoint = inEP;
	mLocalAddress.maxlen = mLocalAddress.len = 0;
	mLocalAddress.buf = NULL;
	mRemoteAddress = mLocalAddress;
	mBuffer = NULL;
}

#pragma mark === OTEndpoint ===

//----------------------------------------------------------------------------------------
// OTEndpoint::OTEndpoint
//----------------------------------------------------------------------------------------

OTEndpoint::OTEndpoint(NMEndpointRef inRef, NMUInt32 inMode) : Endpoint(inRef, inMode)
{	
	DEBUG_ENTRY_EXIT("OTEndpoint::OTEndpoint");
	
	if (inMode & kNMStreamMode)
		mStreamEndpoint = new PrivateEndpoint(this);
	else
		mStreamEndpoint = NULL;
		
	if (inMode & kNMDatagramMode)
		mDatagramEndpoint = new PrivateEndpoint(this);
	else
		mDatagramEndpoint = NULL;
		
	// These need to be alloced by the subclass, if used	
	mPreprocessBuffer = NULL;
	mEnumerationResponseData = NULL;		
	mEnumerationResponseLen = 0;

	mRcvUData.addr.buf = NULL;
	mRcvUData.addr.len = 0;
	mRcvUData.addr.maxlen = 0;

	mRcvUData.opt.len = 0;
	mRcvUData.opt.maxlen = 0;
	mRcvUData.opt.buf = NULL;
	
	mSndUData.opt = mRcvUData.opt;
	mSndUData.addr = mRcvUData.addr;
	
	if ( inMode & kNMDatagramMode )
	{
		mRcvUData.udata.buf = (NMUInt8 *) &mDatagramEndpoint->mBuffer;
		mRcvUData.udata.maxlen = kOTNetbufDataIsOTBufferStar;
    }
    else
    {
        mRcvUData.udata.buf = NULL;
        mRcvUData.udata.maxlen = 0;
    }
	bConnectionConfirmed = false;
	
	machine_mem_zero(&mCall, sizeof (TCall));
	machine_mem_zero(&mHandoffInfo, sizeof (HandoffInfo));
}

//----------------------------------------------------------------------------------------
// OTEndpoint::~OTEndpoint
//----------------------------------------------------------------------------------------

//	Close should be called on an endpoint before it is deleted
// 	This is our last chance to clean up everything
OTEndpoint::~OTEndpoint()
{
	DEBUG_ENTRY_EXIT("OTEndpoint::~OTEndpoint");
	
	delete mDatagramEndpoint;
	delete mStreamEndpoint;
}

//----------------------------------------------------------------------------------------
// OTEndpoint::Close
//----------------------------------------------------------------------------------------

//	This will asynchronously shut down the endpoint.
void
OTEndpoint::Close(void)
{
	DEBUG_ENTRY_EXIT("OTEndpoint::Close");

	NMErr status = kNMNoError;
	
	
	//	If we were already closing, there's nothing to do	
	if (mState == kClosing || mState == kDead)
	{
		DEBUG_PRINT("Tried to close an ep that was already closing: %x", this);
		return;
	}	

	if (mState != kAborting)
		mState = kClosing;

	//	create a new destructo-object and set it going
	EndpointDisposer *foo = new EndpointDisposer(this, mState != kAborting);
	if (foo == NULL){
		status = kNSpMemAllocationErr;
		goto error;
	}

	status = foo->DoIt();

error:
	if (status)
	{
		DEBUG_PRINT("Uh oh.  Got an error %d when creating the disposer", status);
	}	
}

//----------------------------------------------------------------------------------------
// OTEndpoint::BindDatagramEndpoint
//----------------------------------------------------------------------------------------

NMErr
OTEndpoint::BindDatagramEndpoint(NMBoolean inUseConfigAddr)
{
	DEBUG_ENTRY_EXIT("OTEndpoint::BindDatagramEndpoint");

	TBind			request, returned;			// Used to bind the endpoint to the protocol
	TEndpointInfo	info;
	NMBoolean		success = false;
	NMErr			status = kNMNoError;

	op_assert(mDatagramEndpoint);
	op_assert(mDatagramEndpoint->mEP == kOTInvalidEndpointRef);

	status = OTAsyncOpenEndpoint(OTCreateConfiguration(GetDatagramProtocolName()), 
				0, &info, mNotifier.fUPP, mDatagramEndpoint);
	DEBUG_NETWORK_API(0, "OTAsyncOpenEndpoint", status);					
	if (status)
		goto error;

	//	We have to block here, since we need to create the entire new endpoint
	//	before we return.  This doesn't happen very often, so I don't feel that bad.
	//	**sigh** the travails of trying to shield others from asynchronicity
	UnsignedWide	startTime;
	NMUInt32			elapsedMilliseconds;
	
	OTGetTimeStamp(&startTime); 
	while (mDatagramEndpoint->mEP == kOTInvalidEndpointRef) 
	{
		elapsedMilliseconds = OTElapsedMilliseconds(&startTime);
		if (elapsedMilliseconds > mTimeout)
		{
			status = kNMTimeoutErr;
			goto error;
		}
	}
			
	if ( mNetSprocketMode )	// In NetSprocket mode, let's always use the requested port.
	{
		success = AllocAddressBuffer(&request.addr);
		op_assert(success);
		
		OTUtils::CopyNetbuf(&mStreamEndpoint->mLocalAddress, &request.addr);
		
		ResetAddressForUnreliableTransport((OTAddress *) request.addr.buf);
	}
	else
	{
		//	Use the config address if requested
		if (inUseConfigAddr)	
		{
			GetConfigAddress(&request.addr, false);
		}
		else
		{
			request.addr.len = 0;
			request.addr.buf = NULL;
		}
	}

	request.qlen = 0;


	//	Make sure we store the address into our local address buffer
	returned.addr = mDatagramEndpoint->mLocalAddress;

	status = OTSetSynchronous(mDatagramEndpoint->mEP);
	DEBUG_NETWORK_API(mDatagramEndpoint->mEP, "OTSetSynchronous", status);					
	if (status)
		goto error;

	OTSetBlocking(mDatagramEndpoint->mEP);

	status = DoBind(mDatagramEndpoint, &request, &returned);
	if (status)
		goto error;

	status = OTSetAsynchronous(mDatagramEndpoint->mEP);
	DEBUG_NETWORK_API(mDatagramEndpoint->mEP, "OTSetAsynchronous", status);					

error:
	return status;
}

//----------------------------------------------------------------------------------------
// OTEndpoint::BindStreamEndpoint
//----------------------------------------------------------------------------------------

//	a QLen of 0 means don't accept connections.  > 0 means accept connections
NMErr
OTEndpoint::BindStreamEndpoint(OTQLen inQLen)
{
	DEBUG_ENTRY_EXIT("OTEndpoint::BindStreamEndpoint");

	TBind			request, returned;
	TEndpointInfo	info;
	const char		*OTConfigString;
	NMErr			status = kNMNoError;
		
	op_assert(mStreamEndpoint);
	op_assert(mStreamEndpoint->mEP == kOTInvalidEndpointRef);

	if (mStreamEndpoint->mEP == kOTInvalidEndpointRef)
	{
		//	get the correct config string for a listener or a connector
		OTConfigString = (inQLen > 0) ? GetStreamListenerProtocolName() : GetStreamProtocolName();

		//	Open the endpoint
		status = OTAsyncOpenEndpoint(OTCreateConfiguration(OTConfigString), 
					0, &info, mNotifier.fUPP, mStreamEndpoint);
		DEBUG_NETWORK_API(0, "OTAsyncOpenEndpoint", status);					
		if (status)
			goto error;


		UnsignedWide	startTime;
		NMUInt32			elapsedMilliseconds;
		
		OTGetTimeStamp(&startTime); 

		while (mStreamEndpoint->mEP == kOTInvalidEndpointRef) 
		{
			elapsedMilliseconds = OTElapsedMilliseconds(&startTime);

			if (elapsedMilliseconds > mTimeout)
			{
				status = kNMTimeoutErr;
				goto error;
			}
		}
		

		if (inQLen > 0)	//	We're listening, so we need the config address
		{
			GetConfigAddress(&request.addr, false);
		}
		else			// We're connecting.  We don't care what port we get
		{
			request.addr.buf = NULL;
			request.addr.len = 0;
		}
		
		request.qlen = inQLen;
	
		//	Fill in our local address with the address returned
		returned.addr = mStreamEndpoint->mLocalAddress;
		
		status = OTSetSynchronous(mStreamEndpoint->mEP);
		DEBUG_NETWORK_API(mStreamEndpoint->mEP, "OTSetSynchronous", status);					
		if (status)
			goto error;

		OTSetBlocking(mStreamEndpoint->mEP);

		//	Bind it
		status = DoBind(mStreamEndpoint, &request, &returned);
		if (status)
			goto error;

		status = OTSetAsynchronous(mStreamEndpoint->mEP);
		DEBUG_NETWORK_API(mStreamEndpoint->mEP, "OTSetAsynchronous", status);					
		if (status)
			goto error;
	}

	error:
	if (status)
	{
		NMErr code = status;
		return (code == kNMTimeoutErr) ? kNMTimeoutErr : kNMOpenFailedErr;
	}

	return status;
}

//----------------------------------------------------------------------------------------
// OTEndpoint::DoBind
//----------------------------------------------------------------------------------------

NMErr
OTEndpoint::DoBind(
	PrivateEndpoint	*inEP,
	TBind			*inRequest,
	TBind			*outReturned)
{
	NMErr	status = kNMNoError;
	
	//Try_
	{
	
		if (inRequest->addr.buf)
			status = OTBind(inEP->mEP, inRequest, outReturned);
		else
			status = OTBind(inEP->mEP, NULL, outReturned);

		DEBUG_NETWORK_API(inEP->mEP, "OTBind", status);					
		//ThrowIfOSErr_(status);
		if (status)
			goto error;

	}
	//Catch_(code)
	error:
	if (status)
	{
		NMErr code = status;
		return code;
	}
	
	return status;
}

//----------------------------------------------------------------------------------------
// OTEndpoint::Abort
//----------------------------------------------------------------------------------------

NMErr
OTEndpoint::Abort(NMBoolean inWaitForCompletion)
{
	UNUSED_PARAMETER(inWaitForCompletion);

	mState = kAborting;

	Close();

	return kNMNoError;
}

//----------------------------------------------------------------------------------------
// OTEndpoint::AcceptConnection
//----------------------------------------------------------------------------------------

NMErr
OTEndpoint::AcceptConnection(Endpoint *inNewEP, void *inCookie)
{
	DEBUG_ENTRY_EXIT("OTEndpoint::AcceptConnection");

	NMErr 		status = kNMNoError;	
	OTEndpoint		*theNewEP = (OTEndpoint *)inNewEP;

	mHandoffInfo.otherEP = theNewEP; 
	theNewEP->mHandoffInfo.otherEP = this;
	mHandoffInfo.gotData = false;
	theNewEP->mHandoffInfo.gotData = false;
					
	EndpointHander *foo = new EndpointHander(this, theNewEP, (TCall *) inCookie);
	if (foo == NULL){
		status = kNSpMemAllocationErr;
		goto error;
	}
	
	status = foo->DoIt();

error:
	return status;
}

//----------------------------------------------------------------------------------------
// OTEndpoint::RejectConnection
//----------------------------------------------------------------------------------------

//	This will delete the cookie passed in
NMErr
OTEndpoint::RejectConnection(void *inCookie)
{
	DEBUG_ENTRY_EXIT("OTEndpoint::RejectConnection");

NMErr	err = kNMNoError;
TCall	*call = (TCall *) inCookie;
	
	err = OTSndDisconnect(mStreamEndpoint->mEP, call);
	DEBUG_NETWORK_API(mStreamEndpoint->mEP, "OTSndDisconnect", err);					
	
	return err;
}

//----------------------------------------------------------------------------------------
// OTEndpoint::Listen
//----------------------------------------------------------------------------------------

NMErr
OTEndpoint::Listen(void)
{
	DEBUG_ENTRY_EXIT("OTEndpoint::Listen");

	NMErr	status = kNMNoError;

	sDesiredCacheSize = kDefaultMaxEPCacheSize;

	//	specify which kinds of endpoints we need.  If ANYONE requests a kind, then
	//	they're available for all
	if (mMode & kNMStreamMode)
	{
		sWantStreams = true;

		if (sStreamEPCache.otConfigStr == NULL)
			sStreamEPCache.otConfigStr = GetStreamProtocolName();
	}

	if (mMode & kNMDatagramMode)
	{
		sWantDatagrams = true;

		if (sDatagramEPCache.otConfigStr == NULL)
			sDatagramEPCache.otConfigStr = GetDatagramProtocolName();
	}	

	//fill our cache for the first time
	ServiceEPCaches();
	
	mState = kOpening;

	if (mMode & kNMStreamMode)
	{
		status = BindStreamEndpoint(1);		// binds an endpoint that listens for connections
		if (status)
			goto error;
	}	

	if (mMode & kNMDatagramMode)
	{
		status = BindDatagramEndpoint(true);
		if (status)
			goto error;
	}

	if (mMode == kNMNormalMode)
	{
		//	We can only do this after we've opened an endpoint, since the interface info
		//	isn't set until then.
		MakeEnumerationResponse();	// Fill in the response packet
	}

	error:
	if (status)
	{
		NMErr code = status;
		NMErr err;
		err = Abort(true);
		
		return code;
	}	

	return status;
}

//----------------------------------------------------------------------------------------
// OTEndpoint::Connect
//----------------------------------------------------------------------------------------

NMErr
OTEndpoint::Connect(void)
{
	DEBUG_ENTRY_EXIT("OTEndpoint::Connect");

	NMErr		status = kNMNoError;
	TNetbuf		cruft;
	TNetbuf		addr;
	
	//Try_
	{
		mState = kOpening;

		//	If we're doing reliable (or normal), we need a stream.  Make that here
		if (mMode & kNMStreamMode)
		{
			//	bind the stream endpoint that doesn't accept connections
			status = BindStreamEndpoint(0);
			if (status)
				goto error;

			//	If we're connecting on the stream, the config address is the remote stream address
			GetConfigAddress(&addr, true);
		}

		//	Open the datagram ep, if requested
		if (mMode & kNMDatagramMode)
		{
			status = BindDatagramEndpoint(false);
				
			if (status)
				goto error;
			
			//	If we're ONLY doing a datagram ep, then the config addr refers to the datagram ep
			if (mMode != kNMNormalMode)
			{
				GetConfigAddress(&addr, true);
			}
		}		
		
		if (mMode == kNMNormalMode)
		{
			//	We can only do this after we've opened an endpoint, since the interface info
			//	isn't set until then.

				MakeEnumerationResponse();	// Fill in the response packet
		
//	The following block replaced by a CopyNetBuf() after receiving the T_CONNECT callback
//	somewhere below.  That works better for AppleTalk and just as well for TCP/IP.
//		
//				if (gInNetSprocketMode)
//				{
//					CopyNetbuf(&addr, &mDatagramEndpoint->mRemoteAddress);
//					ResetAddressForUnreliableTransport((OTAddress *) mDatagramEndpoint->mRemoteAddress.buf);
//				}
		}
		//	if we're not doing streams, set the remote address and leave
		else if (mMode == kNMDatagramMode)
		{
			OTUtils::CopyNetbuf(&addr, &mDatagramEndpoint->mRemoteAddress);
			return kNMNoError;
		}

		//	We don't really care about most parameters to the call, so set up a bland TNetbuf
		cruft.buf = 0;
		cruft.len = 0;
		cruft.maxlen = 0;

		//	Set up our call structure to connect to the host
		OTUtils::CopyNetbuf(&addr, &mCall.addr);
		mCall.opt = cruft;
		mCall.udata = cruft;

		//	Set up the rcvCall to know what we connected to
		mRcvCall.addr = mStreamEndpoint->mRemoteAddress;
		mRcvCall.opt = cruft;
		mRcvCall.udata = cruft;
		
		status = OTConnect(mStreamEndpoint->mEP, &mCall, &mRcvCall);

		if (status == kOTNoDataErr)
			status = kNMNoError;

		DEBUG_NETWORK_API(mStreamEndpoint->mEP, "OTConnect", status);					
		
		if (status != kNMNoError)
		{
			DEBUG_PRINT("Got an error in Connect: %ld", status);

			if (status == kOTLookErr)
			{
				status = DoLook(mStreamEndpoint);

				if (status != kNMNoError)
				{
					DEBUG_PRINT("DoLook returned error: %ld", status);
				}
			}
		}
		if (status)
			goto error;

		//	Wait for things to complete
		OTTimeStamp			startTime;
		NMUInt32			elapsedMilliseconds = 0;
		NMBoolean			keepWaiting = true;
		OTResult			state;
		
		OTGetTimeStamp(&startTime); 

		while ((OTGetEndpointState(mStreamEndpoint->mEP) != T_DATAXFER) && mState == kOpening)
		{
			elapsedMilliseconds = OTElapsedMilliseconds(&startTime);

			if (elapsedMilliseconds > mTimeout)
				break;
		}

		state = OTGetEndpointState(mStreamEndpoint->mEP);
		if (state != T_DATAXFER){
			status = kNMBadStateErr;
			goto error;
		}
		
		if (mState != kOpening){
			status = kNMBadStateErr;
			goto error;
		}
		
		if (mMode == kNMNormalMode)
		{	
			//	Now block, waiting for the listener to send us the open confirmation
			//	or for our stream to finish opening if we're not using confirmations 
			//	(nspmode or stream mode)

			status = WaitForOpenConfirmation();
			if (status)
				goto error;

			if (!mNetSprocketMode) 
			{				
				
				//	Send the confirmation down the stream endpoint.  This differs from protocol to protocol
				//	This is the very first thing expected for normal mode, non NetSprocketMode connections
				status = SendOpenConfirmation();
				op_warn(status == kNMNoError);
				if (status)
					goto error;
			}	
		}

		return status;
	}
	error:
	if (status)
	{
		NMErr code = status;
		Abort(true);
		
		return kNMOpenFailedErr;
	}
	return status;
}

//----------------------------------------------------------------------------------------
// OTEndpoint::FreeAddress
//----------------------------------------------------------------------------------------

NMBoolean
OTEndpoint::FreeAddress(void **outAddress)
{
	DEBUG_ENTRY_EXIT("OTEndpoint:FreeAddress");

	/* sanity check */
	if( !outAddress || !*outAddress )
		return( kNMParameterErr );

	free( *outAddress );	/* free memory, then mark as NULL for safety */
	*outAddress = NULL;

	return( kNMNoError );
}

//----------------------------------------------------------------------------------------
// OTEndpoint::GetAddress
//----------------------------------------------------------------------------------------

NMBoolean
OTEndpoint::GetAddress(NMAddressType addressType, void **outAddress)
{
	DEBUG_ENTRY_EXIT("OTEndpoint::GetAddress");

	NMErr status = kNMNoError;

	//Way down low in the bowels of the C++ layers is the real code.

	switch( addressType )
	{
		case kNMIPAddressType:	//¥ IP address (string of format "127.0.0.1:80")
			OTAddress *addr;
			addr = (OTAddress *)(mStreamEndpoint->mRemoteAddress.buf);
			if( AF_INET == addr->fAddressType )
			{
				InetAddress *iaddr = (InetAddress *)addr;
				char addrStr[256];
				int size;

				/* Create IP String */
				sprintf( addrStr, "%d.%d.%d.%d:%d",
									(int)((UInt8)(iaddr->fHost >> 24)), (int)((UInt8)(iaddr->fHost >> 16)),
									(int)((UInt8)(iaddr->fHost >> 8)), (int)((UInt8)(iaddr->fHost)),			iaddr->fPort);
				size = strlen( addrStr );

				/* Create output buffer for string */
				*outAddress = malloc( size + 1 );
				if (NULL == *outAddress)
					return (kNMOutOfMemoryErr);

				/* Send string to user */
				strcpy( (char *)*outAddress, addrStr );
			}
			else if( AF_DNS == addr->fAddressType )
			{
				int size = mStreamEndpoint->mRemoteAddress.len - sizeof(OTAddressType);

				/* Create output buffer for string */
				*outAddress = malloc( size );
				if (NULL == *outAddress)
					return (kNMOutOfMemoryErr);

				/* Send string to user */
				BlockMoveData( ((DNSAddress *)addr)->fName, *outAddress, size );
			}
			else
				status = kNMParameterErr;
			break;

		case kNMOTAddressType:	//¥	OT address (of unknown type)
			*outAddress = malloc(mStreamEndpoint->mRemoteAddress.len);
			if (NULL == *outAddress)
				return (kNMOutOfMemoryErr);

			/* copy the InetAddress (or other format of OTAddress) to user's buffer */
			BlockMoveData(mStreamEndpoint->mRemoteAddress.buf, *outAddress, mStreamEndpoint->mRemoteAddress.len);
			break;

		default:
			status = kNMParameterErr;
			break;
	}
	return( status );	
}

//----------------------------------------------------------------------------------------
// OTEndpoint::IsAlive
//----------------------------------------------------------------------------------------

NMBoolean
OTEndpoint::IsAlive(void)	
{
	DEBUG_ENTRY_EXIT("OTEndpoint::IsAlive");

	//ECF011114 not sure if this is exactly correct, but its better than always returning false
	NMBoolean result;
	if (mState == kRunning)
		result = true;
	else
		result = false;
	
	return result;
}

//----------------------------------------------------------------------------------------
// OTEndpoint::Idle
//----------------------------------------------------------------------------------------

NMErr
OTEndpoint::Idle(void)	
{
	NMErr	err = kNMNoError;
	
	ServiceEPCaches();
	
	return err;
}

//----------------------------------------------------------------------------------------
// OTEndpoint::FunctionPassThrough
//----------------------------------------------------------------------------------------

NMErr
OTEndpoint::FunctionPassThrough(NMType inSelector, void *inParamBlock)	
{
	DEBUG_ENTRY_EXIT("OTEndpoint::FunctionPassThrough");

NMErr	err = kNMNoError;
		
	UNUSED_PARAMETER(inSelector);
	UNUSED_PARAMETER(inParamBlock);
		
	return err;
}

//----------------------------------------------------------------------------------------
// OTEndpoint::SendDatagram
//----------------------------------------------------------------------------------------

NMErr
OTEndpoint::SendDatagram(
	const NMUInt8		*inData,
	NMUInt32	inSize,
	NMFlags		inFlags)	
{
	DEBUG_ENTRY_EXIT("OTEndpoint::SendDatagram");

OTResult	status;
	
	//	Make sure we've got a datagram endpoint
	if (! (mMode & kNMDatagramMode))
		return kNMWrongModeErr;
	
	if (mState != kRunning)
	{
		DEBUG_PRINT("called SendDatagram when we were in state %d", mState);
		return kNMBadStateErr;
	}
	
	//	Set up the unit data structure
	mSndUData.addr = mDatagramEndpoint->mRemoteAddress;
	mSndUData.udata.len = inSize;
	mSndUData.udata.buf = (NMUInt8 *) inData;
	
	//	if the user wants us to block, try blocking
	if (inFlags & kNMBlocking)
	{
		if (mDatagramEndpoint->mInNotifier)
		{
			DEBUG_PRINT("Tried to go sync from within notifier.  Can't do that");
			return kNMBadStateErr;
		}

		//	The EP is already blocking, so going sync will cause us to send everything
		//	before returning
		status = OTSetSynchronous(mDatagramEndpoint->mEP);
		DEBUG_NETWORK_API(mDatagramEndpoint->mEP, "OTSetSynchronous", status);
	}

	do
	{
		status = OTSndUData(mDatagramEndpoint->mEP, &mSndUData);

		if (status != kNMNoError)
		{
			DEBUG_NETWORK_API(mDatagramEndpoint->mEP, "OTSndUData", status);					

			//	if its a flow error, see if we can wait till flow is clear
			if (status == kOTLookErr)
			{
				DoLook(mDatagramEndpoint);
			}
		}
			
	} while (status == kOTLookErr);

	if (status == kOTFlowErr)
		status = kNMFlowErr;
	else if (status < 0)
		status = kNMProtocolErr;
		
	if (inFlags & kNMBlocking && OTIsSynchronous(mDatagramEndpoint->mEP))
		OTSetAsynchronous(mDatagramEndpoint->mEP);
		
	return status;
}

//----------------------------------------------------------------------------------------
// OTEndpoint::ReceiveDatagram
//----------------------------------------------------------------------------------------

NMErr
OTEndpoint::ReceiveDatagram(
	NMUInt8		*outData,
	NMUInt32	*ioSize,
	NMFlags		*outFlags)	
{
	DEBUG_ENTRY_EXIT("OTEndpoint::ReceiveDatagram");

NMBoolean	finished;
	
	//	Make sure we've got a datagram endpoint
	if (! (mMode & kNMDatagramMode))
		return kNMWrongModeErr;
		
	//	If they didn't call us back right away, we probably discarded our buffer, or never had any to begin with
	if (mDatagramEndpoint->mBuffer == NULL)
		return kNMNoDataErr;
		
	finished = OTReadBuffer(&mDatagramEndpoint->mBufferInfo, outData, ioSize);	
	*outFlags = 0;
	
	//	clean up so that our notifier will know the user read all the data
	if (finished)
	{
		OTReleaseBuffer(mDatagramEndpoint->mBuffer);
		mDatagramEndpoint->mBuffer = NULL;
	}
	
	return (*ioSize > 0) ? kNMNoError : kNMNoDataErr;
}

//----------------------------------------------------------------------------------------
// OTEndpoint::Send
//----------------------------------------------------------------------------------------

// returns number of bytes actually sent, or an error
long
OTEndpoint::Send(
	const void		*inData,
	NMUInt32	inSize,
	NMFlags		inFlags)	
{
	DEBUG_ENTRY_EXIT("OTEndpoint::Send");

OTResult	result;
OTFlags		flags = 0;
NMUInt8		*data = (NMUInt8 *) inData;
NMUInt32		size = inSize;
	
	//	Make sure we've got a datagram endpoint
	if (!(mMode & kNMStreamMode))
		return kNMWrongModeErr;
		
	if (mState != kRunning)
	{
		DEBUG_PRINT("called Send when we were in state %d", mState);
		return kNMBadStateErr;
	}
	
	//	if the user wants us to block, try blocking
	if (inFlags & kNMBlocking)
	{
		if (mStreamEndpoint->mInNotifier)
		{
			DEBUG_PRINT("Tried to go sync from within notifier.  Can't do that");
			return kNMBadStateErr;
		}

		//	The EP is already blocking, so going sync will cause us to send everything
		//	before returning
		result = OTSetSynchronous(mStreamEndpoint->mEP);
		if (result != kNMNoError) 
			DEBUG_NETWORK_API(mStreamEndpoint->mEP, "OTSetSynchronous", result);
	}
	
	do
	{
		result = OTSnd(mStreamEndpoint->mEP, data, size, flags);

		if (result < 0)
		{
			//flow control shouldnt cause a halt, should it?... ECF
			//DEBUG_NETWORK_API(mStreamEndpoint->mEP, "OTSnd", result);

			if (result == kOTLookErr)
			{
				DoLook(mStreamEndpoint);
			}
		}
	} while (result == kOTLookErr);
	
	if (result == kOTFlowErr)
		result = kNMFlowErr;
	else if (result < 0)
		result = kNMProtocolErr;
		
	if (inFlags & kNMBlocking && OTIsSynchronous(mStreamEndpoint->mEP))
		OTSetAsynchronous(mStreamEndpoint->mEP);
		
	return result;
}

//----------------------------------------------------------------------------------------
// OTEndpoint::Receive
//----------------------------------------------------------------------------------------

NMErr
OTEndpoint::Receive(
	void		*outData,
	NMUInt32	*ioSize,
	NMFlags		*outFlags)	
{
	DEBUG_ENTRY_EXIT("OTEndpoint::Receive");

NMBoolean	finished;
	
	//	Make sure we've got a datagram endpoint
	if (!(mMode & kNMStreamMode))
		return kNMWrongModeErr;
		
	//	If they didn't call us back right away, we probably discarded our buffer, or never had any to begin with
	if (mStreamEndpoint->mBuffer == NULL)
		return kNMNoDataErr;
		
	finished = OTReadBuffer(&mStreamEndpoint->mBufferInfo, outData, ioSize);	
	*outFlags = 0;
	
	//	clean up so that our notifier will know the user read all the data
	if (finished)
	{
		OTReleaseBuffer(mStreamEndpoint->mBuffer);
		mStreamEndpoint->mBuffer = NULL;
	}
	
	return (*ioSize > 0) ? kNMNoError : kNMNoDataErr;
}

//----------------------------------------------------------------------------------------
// OTEndpoint::DoLook
//----------------------------------------------------------------------------------------

NMErr
OTEndpoint::DoLook(PrivateEndpoint *inEP)
{
	DEBUG_ENTRY_EXIT("OTEndpoint::DoLook");

OTResult status;
OTEndpoint *ep = inEP->mOwnerEndpoint;
			

	status = OTLook(inEP->mEP);
	Notifier(inEP, status, 0, NULL);	// call our notifier to handle the error
	
	return status;
}

//----------------------------------------------------------------------------------------
// OTEndpoint::DoCloseComplete
//----------------------------------------------------------------------------------------

//	can be overriden by subclasses to do something before being deleted
void
OTEndpoint::DoCloseComplete(EndpointDisposer *inEPDisposer, NMBoolean inNotifyUser)
{
	DEBUG_ENTRY_EXIT("OTEndpoint::DoCloseComplete");

	if (inNotifyUser)
		(*mCallback)(mRef, mContext, kNMCloseComplete, 0, NULL);

	CloseEPComplete(mRef);	

	//	the last thing is to dispose of the disposer.  If we're closing the module,
	//	this keeps us from closing until we're all done
	delete inEPDisposer;                     
}

//----------------------------------------------------------------------------------------
// OTEndpoint::HandleQuery
//----------------------------------------------------------------------------------------

//	This assumes the query came in on the datagram ep
void
OTEndpoint::HandleQuery(void)
{
	DEBUG_ENTRY_EXIT("OTEndpoint::HandleQuery");

TUnitData	udata;
NMErr	status;

	udata.addr = mRcvUData.addr;
	udata.udata.buf = mEnumerationResponseData;
	udata.udata.len = mEnumerationResponseLen;
	udata.opt.len = 0;

	// dair, send kNMUpdateResponse to allow the response to be updated
	NMUInt32	dataSize = mEnumerationResponseLen                - sizeof(IPEnumerationResponsePacket);
	NMUInt8		*dataPtr = ((NMUInt8 *) mEnumerationResponseData) + sizeof(IPEnumerationResponsePacket);

	if (mCallback != NULL && dataSize != 0)
		mCallback(mRef, mContext, kNMUpdateResponse, dataSize, dataPtr);

	do
	{
		status = OTSndUData(mDatagramEndpoint->mEP, &udata);		
		DEBUG_NETWORK_API(mDatagramEndpoint->mEP, "OTSndUData", status);	
						
		if (status == kOTLookErr)
			DoLook(mDatagramEndpoint);
			
	} while (status != kNMNoError && status == kOTLookErr);
}

#pragma mark === Enter/Leave Notifier ===

//----------------------------------------------------------------------------------------

NMErr
OTEndpoint::EnterNotifier(
	NMUInt32	endpointMode)	
{
	DEBUG_ENTRY_EXIT("OTEndpoint::EnterNotifier");

NMErr		status;
NMBoolean	entered = true;
	
	if (endpointMode & kNMDatagramMode)
		entered = OTEnterNotifier(mDatagramEndpoint->mEP);

	if (!entered)
	{
		status = -1;
		return status;
	}
	
	if ( (entered) && (endpointMode & kNMStreamMode) )
		entered = OTEnterNotifier(mStreamEndpoint->mEP);
	
	if (entered)
	{
		status = noErr;
	}
	else
		status = -1;	
			
	return status;
}

//----------------------------------------------------------------------------------------

NMErr
OTEndpoint::LeaveNotifier(
	NMUInt32	endpointMode)	
{
	DEBUG_ENTRY_EXIT("OTEndpoint::EnterNotifier");
	
	if (endpointMode & kNMDatagramMode)
		OTLeaveNotifier(mDatagramEndpoint->mEP);

	if (endpointMode & kNMStreamMode)
		OTLeaveNotifier(mStreamEndpoint->mEP);
				
	return noErr;
}

#pragma mark === Async Notifier Handlers ===

//----------------------------------------------------------------------------------------
// OTEndpoint::Notifier
//----------------------------------------------------------------------------------------

pascal void
OTEndpoint::Notifier(
	void		*contextPtr,
	OTEventCode	code,
	OTResult	result,
	void		*cookie)
{
	DEBUG_ENTRY_EXIT("OTEndpoint::Notifier");
	
PrivateEndpoint 	*epStuff = (PrivateEndpoint *) contextPtr;
OTEndpoint			*ep = epStuff->mOwnerEndpoint;
NMErr				status;

	UNUSED_PARAMETER(result);

	epStuff->mInNotifier++;
	
	DEBUG_NETWORK_API(epStuff->mEP, "Notifier", result);	

	switch (code)
	{
		case T_DATA:
			// Normal data has arrived.  Call OTRcvUData or OTRcv.  Continue reading until
			// the function returns with kOTNoDataErr.
			ep->HandleData(epStuff);
			break;

		case T_CONNECT:
			// The passive peer has accepted a connection that you requested using the 
			// OTConnect function.  Call the OTRcvConnect function to retrieve any data
			// or option information that the passive peer has specified when accepting the
			// function or to retrieve the address to which you are actually connected.
			// The cookie parameter to the notifier function is the sndCall parameter that
			//  you specified when calling the OTConnect function
			status = OTRcvConnect(epStuff->mEP, &ep->mRcvCall);
			DEBUG_NETWORK_API(epStuff->mEP, "OTRcvConnect", status);	

			// For NetSprocket mode, this is a good place to set the remote datagram address...
			if (status == kNMNoError && ep->mNetSprocketMode)
			{
				OTUtils::CopyNetbuf(&ep->mRcvCall.addr, &ep->mDatagramEndpoint->mRemoteAddress);
				ep->ResetAddressForUnreliableTransport((OTAddress *) ep->mDatagramEndpoint->mRemoteAddress.buf);
			}
			
			//ecf in NSpMode, we need to wait for the stream to get up and running.
			if (status == kNMNoError && ((ep->mNetSprocketMode) || (ep->mMode == kNMStreamMode)))
				ep->bConnectionConfirmed = true;				
			break;

		case T_DISCONNECT:
			// A connection has been torn down or rejected.  Use the OTRcvDisconnect 
			//  function to clear the event.
			// If the event is used to signify that the connection has been terminated,
			//  cookie is NULL
			// Else, cookie is the same as the sndCall parameter passed to OTConnect
			ep->HandleDisconnect(epStuff);
			break;

		case T_ORDREL:
			// The remote client has called the OTSndOrderlyDisconnect function to initiate
			//  an orderly disconnect.  You must call the OTRcvOrderlyDisconnect function to
			//  acknowledge receiving the event and to retrieve and data that may have been
			//  sent with the disconnection request.
			ep->HandleOrdRel(epStuff);
			break;

		case T_DISCONNECTCOMPLETE:
			if (ep->mState == kOpening)
				ep->mState = kDead;
			break;

		case T_LISTEN:
			ep->HandleNewConnection();
			break;

		case T_OPENCOMPLETE:
			ep->HandleOpenEndpointComplete(epStuff, (EndpointRef) cookie);
			break;

		case T_ACCEPTCOMPLETE:
			// The OTAccept function has completed.  The cookie parameter contains the 
			// endpoint reference of the endpoint to which you passed off the connection.
			ep->HandleAcceptComplete();
			break;

		case T_PASSCON:
			// When the OTAccept function completes, the endpoint provider passes this event
			// to the endpoint receiving the connection. (Whether that endpoint is the same as
			// or different from the endpoint that calls the OTAccept function).  The cookie
			// parameter contains the endpoint reference of the endpoint that called the
			// OTAccept function.
			ep->HandlePassConnComplete(epStuff);
			break;

		case T_GODATA:
			DEBUG_PRINT("Got flow clear for ep %x", ep);
			(*ep->mCallback)(ep->mRef, ep->mContext, kNMFlowClear, 0, NULL);
			break;

		case T_UDERR:
			OTRcvUDErr(epStuff->mEP, NULL);

		default:
			break;
	}

	epStuff->mInNotifier--;	
}

//----------------------------------------------------------------------------------------
// OTEndpoint::PreprocessPacket
//----------------------------------------------------------------------------------------

NMErr
OTEndpoint::PreprocessPacket(void)
{
	return kNMNoError;
}

//----------------------------------------------------------------------------------------
// OTEndpoint::HandleData
//----------------------------------------------------------------------------------------

//	This is a static function because inEPStuff will be NULL for the datagram endpoint.
//	We need to figure out who this is destined for
void
OTEndpoint::HandleData(PrivateEndpoint *inEPStuff)
{
	DEBUG_ENTRY_EXIT("OTEndpoint::HandleData");

NMCallbackCode 	nmCode;
OTResult  		result;
OTFlags			flags = 0;
NMBoolean		flowCount = 0;
	
	//	Which endpoint is it for?
	nmCode = (inEPStuff == mDatagramEndpoint) ? kNMDatagramData : kNMStreamData;

	//	This will do a no-copy receive.  Better not hold onto it too long!
	if (nmCode == kNMStreamData)
	{
		do
		{
			//	Do a no-copy receive.  The client had better deal with this quickly!
			result = OTRcv(inEPStuff->mEP, &inEPStuff->mBuffer, kOTNetbufDataIsOTBufferStar, &flags);

			if (result < 0 && result != kOTNoDataErr)
				DEBUG_NETWORK_API(inEPStuff->mEP, "OTRcv", result);	

			if (result == kOTLookErr)
			{
				result = DoLook(inEPStuff);
				continue;
			}

			//	Handle the special case of getting data before the PASSCON has happened
			if (result == kOTStateChangeErr && mState == kHandingOff)
			{
				inEPStuff->mOwnerEndpoint->mHandoffInfo.gotData = true;
			}

			if (result > 0)
			{
				OTInitBufferInfo(&inEPStuff->mBufferInfo, inEPStuff->mBuffer);

				if ((mMode == kNMNormalMode && !mNetSprocketMode) && (mState == kHandingOff || !bConnectionConfirmed) )
				{				
					result = HandleAsyncOpenConfirmation();
					
					if (mState == kHandingOff)
					{
						mState = kRunning;
			
						(*mCallback)(mRef, mContext, kNMAcceptComplete, 0, mHandoffInfo.otherEP->mRef);
						
						if (result == kNMMoreDataErr)
							(*mCallback)(mRef, mContext, kNMStreamData, OTBufferDataSize(mStreamEndpoint->mBuffer), NULL);
					}
				}
				else if (mState == kRunning)
				{
					//	Make the callback to the user to copy the buffer
					(*mCallback)(mRef, mContext, nmCode, OTBufferDataSize(inEPStuff->mBuffer), NULL);
				}
				
				if (inEPStuff->mBuffer != NULL)	// it's set to NULL in the Receive routine
				{
					//	If the user didn't get it, tough luck.  We're discarding it
					OTReleaseBuffer(inEPStuff->mBuffer);
				}

				inEPStuff->mBuffer = NULL;
			}
			else if (result != kOTNoDataErr)
			{
				DEBUG_NETWORK_API(inEPStuff->mEP, "OTRcv", result);
				break;	
			}
			
		} while (result != kOTNoDataErr);	// while we have data, do it
	}
	else	// It's a datagram
	{
		do	// Loop until there's no more data
		{
			//	This will do a no-copy receive
			result = OTRcvUData(inEPStuff->mEP, &mRcvUData, &flags);

			if (result != kNMNoError && result != kOTNoDataErr)
			{
				DEBUG_PRINT("OTRcvUData returned %ld", result);
			}

			if (result == kOTLookErr)
			{
				DoLook(mDatagramEndpoint);
			}
			else if (result == kNMNoError)
			{	
				//	Check if it's a special packet for us (enumerations)
				if (mMode == kNMNormalMode &&	DoingQueryForwarding() && (PreprocessPacket() == kWasQuery))
				{	
					HandleQuery();
				}
				else	// otherwise, send it to the user
				{
					//	if we're datagram only, set the endpoint address to the datagram sender's
					if (mMode == kNMDatagramMode)
					{
						OTUtils::CopyNetbuf(&mRcvUData.addr, &inEPStuff->mRemoteAddress);
					}
					 
					OTInitBufferInfo(&inEPStuff->mBufferInfo, inEPStuff->mBuffer);

					//	Make the callback to the user to copy the buffer.
					//	Note:  In NetSprocket, the listening datagram endpoint also receives
					//  messages which may be routed by the host to other players.
//					if ( (mState == kRunning) || ( (mState == kListening) && (gInNetSprocketMode) ) )
					if ( (mState == kRunning) || ( (mState == kListening) && (mNetSprocketMode) ) )
					{
						(*mCallback)(mRef, mContext, nmCode, OTBufferDataSize(inEPStuff->mBuffer), NULL);
					}
				}

				//	The user had a chance to read it, now release the buffer
				if (inEPStuff->mBuffer != NULL)
				{
					OTReleaseBuffer(inEPStuff->mBuffer);
				}

				inEPStuff->mBuffer = NULL;
			}
			else if (result != kOTNoDataErr)
			{
				DEBUG_NETWORK_API(inEPStuff->mEP, "OTRcvUData", result);	
			}
		} while (result != kOTNoDataErr);	
	}
}

//----------------------------------------------------------------------------------------
// OTEndpoint::HandleNewConnection
//----------------------------------------------------------------------------------------

void
OTEndpoint::HandleNewConnection(void)
{
	DEBUG_ENTRY_EXIT("OTEndpoint::HandleNewConnection");

	NMErr status = kNMNoError;

	//Try_
	{	
		if (mState != kListening)
		{
			DEBUG_PRINT("Hey.  We got a connect request even though we're not listening!");

			status = OTSndDisconnect(mStreamEndpoint->mEP, NULL);
			DEBUG_NETWORK_API(mStreamEndpoint->mEP, "OTSndDisconnect", status);	

			return;
		}
		
		status = OTListen(mStreamEndpoint->mEP, &mCall);
		DEBUG_NETWORK_API(mStreamEndpoint->mEP, "OTListen", status);	
		
		if (status == kOTLookErr)
		{
		OTResult	lookErr = OTLook(mStreamEndpoint->mEP);

			if (lookErr == T_DISCONNECT)			
				OTRcvDisconnect(mStreamEndpoint->mEP, NULL);
			else
				DEBUG_PRINT("Got an unknown look err: %d", lookErr);
			
			return;
		}
			
		//ThrowIfOSErr_(status);
		if (status)
			goto error;

		//	Make the callback to the user.  he should call accept/reject
		(*mCallback)(mRef, mContext, kNMConnectRequest, 0, &mCall);
	}
	//Catch_(code)
	error:
	if (status)
	{
		NMErr code = status;
		status = OTSndDisconnect(mStreamEndpoint->mEP, NULL);
		DEBUG_NETWORK_API(mStreamEndpoint->mEP, "OTSndDisconnect", status);	
	}
}

//----------------------------------------------------------------------------------------
// OTEndpoint::HandleDisconnect
//----------------------------------------------------------------------------------------

void
OTEndpoint::HandleDisconnect(PrivateEndpoint *inEPStuff)
{
	DEBUG_ENTRY_EXIT("OTEndpoint::HandleDisconnect");

NMErr	status;
	
	status = OTRcvDisconnect(inEPStuff->mEP, NULL);
	DEBUG_NETWORK_API(inEPStuff->mEP, "OTRcvDisconnect", status);	
		
	if (inEPStuff->mOwnerEndpoint->mState == kRunning)		// Tell the user we were disconnected
		(*mCallback)(mRef, mContext, kNMEndpointDied, 0, 0);
	else
		mState = kAborting;
}

//----------------------------------------------------------------------------------------
// OTEndpoint::HandleOrdRel
//----------------------------------------------------------------------------------------

void
OTEndpoint::HandleOrdRel(PrivateEndpoint *inEPStuff)
{
	DEBUG_ENTRY_EXIT("OTEndpoint::HandleOrdRel");

NMErr	status;
	
	status = OTRcvOrderlyDisconnect(inEPStuff->mEP);

	if (status != kNMNoError)
	{
		DEBUG_NETWORK_API(inEPStuff->mEP, "OTRcvOrderlyDisconnect", status);	

		if (status == kOTLookErr)
		{
			status = DoLook(inEPStuff);

			if (status == T_DISCONNECT || status == T_ORDREL)
				status = kNMNoError;		// abort
		}
	}

	if (mState == kRunning)
		(*mCallback)(mRef, mContext, kNMEndpointDied, 0, 0);
}

//----------------------------------------------------------------------------------------
// OTEndpoint::HandleOpenEndpointComplete
//----------------------------------------------------------------------------------------

void
OTEndpoint::HandleOpenEndpointComplete(PrivateEndpoint *inEPStuff, EndpointRef inEP)
{
	DEBUG_ENTRY_EXIT("OTEndpoint::HandleOpenEndpointComplete");

	//	Set the ep so that anyone waiting on us will be able to continue	
	inEPStuff->mEP = inEP;
}

//----------------------------------------------------------------------------------------
// OTEndpoint::HandleAcceptComplete
//----------------------------------------------------------------------------------------

//	Called to the endpoint handing off the connection
void
OTEndpoint::HandleAcceptComplete(void)
{
	DEBUG_ENTRY_EXIT("OTEndpoint::HandleAcceptComplete");

	mState = kListening;
	
	(*mCallback)(mRef, mContext, kNMHandoffComplete, 0, mHandoffInfo.otherEP->mRef);
}

//----------------------------------------------------------------------------------------
// OTEndpoint::HandlePassConnComplete
//----------------------------------------------------------------------------------------

//	Called to the new endpoint for a handoff
void
OTEndpoint::HandlePassConnComplete(PrivateEndpoint *inEPStuff)
{
	DEBUG_ENTRY_EXIT("OTEndpoint::HandlePassConnComplete");

NMErr	status;
	
//	if ( (mMode == kNMNormalMode) && (!gInNetSprocketMode) )
	if ( (mMode == kNMNormalMode) && (!mNetSprocketMode) )
	{	
		//	Send the confirmation down the stream endpoint.  This differs from protocol to protocol
		//	This is the very first thing expected ONLY for normal mode connections	
			status = SendOpenConfirmation();
			op_assert(status == kNMNoError);
	}
	else
	{
		// For NetSprocket mode, this is a good place to set the remote datagram address...
//		if (gInNetSprocketMode)
		if (mNetSprocketMode)
		{			
			OTUtils::CopyNetbuf(&mStreamEndpoint->mRemoteAddress, &mDatagramEndpoint->mRemoteAddress);
								
			ResetAddressForUnreliableTransport((OTAddress *) mDatagramEndpoint->mRemoteAddress.buf);
		}
	
		mState = kRunning;
			
		(*mCallback)(mRef, mContext, kNMAcceptComplete, 0, mHandoffInfo.otherEP->mRef);
	
		//	If we got a T_DATA callback before the PASSCON, then handle the data now	
		if (inEPStuff->mOwnerEndpoint->mHandoffInfo.gotData == true)
			HandleData(inEPStuff);		
	}
}

#pragma mark ==== EP Cache Functions ===

//----------------------------------------------------------------------------------------
// OTEndpoint::ServiceEPCaches
//----------------------------------------------------------------------------------------

void
OTEndpoint::ServiceEPCaches()
{
	NMErr	status;

	if (sStreamEPCache.otConfigStr != NULL && sStreamEPCache.totalCount < sDesiredCacheSize)
	{
		status = FillEPCache(&sStreamEPCache);
	}

	if (sDatagramEPCache.otConfigStr != NULL && sDatagramEPCache.totalCount < sDesiredCacheSize)
	{
		status = FillEPCache(&sDatagramEPCache);
	}
}

//----------------------------------------------------------------------------------------
// OTEndpoint::FillEPCache
//----------------------------------------------------------------------------------------

NMErr
OTEndpoint::FillEPCache(EPCache *inCache)
{
NMErr	status = kNMNoError;
CachedEP	*ep;
	
	if (gTerminating)
		return status;
		
	for (int i = inCache->totalCount; i < sDesiredCacheSize; i++)
	{
		ep = new CachedEP;

		if (ep == NULL)
		{
			DEBUG_PRINT("Ran out of memory filling the ep cache");
			return kNMOutOfMemoryErr;
		}
		OTAtomicAdd32(1, &inCache->totalCount);
		
		ep->destCache = inCache;
		ep->ep = NULL;

		status = OTAsyncOpenEndpoint(OTCreateConfiguration(inCache->otConfigStr), 0, NULL, mCacheNotifier.fUPP, ep);

		DEBUG_NETWORK_API(NULL, "OTAsyncOpenEndpoint", status);

		if (status != kNMNoError)
			delete ep;	
	}
	
	return status;
}

//----------------------------------------------------------------------------------------
// OTEndpoint::CacheHandoffEP
//----------------------------------------------------------------------------------------

void
OTEndpoint::CacheHandoffEP(CachedEP *inEP)
{
	OTLIFOEnqueue(&inEP->destCache->Q, &inEP->link);
	OTAtomicAdd32(1, &inEP->destCache->readyCount);
	
}

//----------------------------------------------------------------------------------------
// OTEndpoint::CacheNotifier
//----------------------------------------------------------------------------------------

//	th only purpose of this notifier is to create endpoints for the handoff caches
pascal void
OTEndpoint::CacheNotifier(
	void		*contextPtr,
	OTEventCode	code,
	OTResult	result,
	void		*cookie)
{
	DEBUG_ENTRY_EXIT("OTEndpoint::CacheNotifier");

NMErr	status;
CachedEP 	*cachedEP = (CachedEP *) contextPtr;
	
	op_warn(cachedEP != NULL);
	
	if (cachedEP == NULL)
		return;
		
	if (result != kNMNoError)
	{
		DEBUG_PRINT("EP cache notifier got an error %d", result);
		return;
	}
	
	switch (code)
	{
		case T_OPENCOMPLETE:
			cachedEP->ep = (EndpointRef) cookie;
			status = OTSetBlocking(cachedEP->ep);
			DEBUG_NETWORK_API(NULL, "OTSetBlocking", status);
			
			CacheHandoffEP(cachedEP);
			break;

		case T_BINDCOMPLETE:
			break;
			
		default:
			DEBUG_PRINT("An unexepected event (%d) happened for a cached EP: %p", code, contextPtr);
			break;
	}
}
