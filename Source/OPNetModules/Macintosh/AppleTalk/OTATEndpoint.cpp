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
#include "OTATEndpoint.h"
#include "Exceptions.h"
#include "OTUtils.h"
#include "String_Utils.h"

//	------------------------------	Private Definitions


//	------------------------------	Private Types
//	------------------------------	Private Variables

const NMUInt32 OTATEndpoint::kConnectionValidatedFlag = 'vald';
NetNotifierUPP	OTATEndpoint::mMapperNotifier(OTATEndpoint::MapperNotifier);

//	------------------------------	Private Functions
//	------------------------------	Public Variables


//----------------------------------------------------------------------------------------
// OTATEndpoint::OTATEndpoint
//----------------------------------------------------------------------------------------

OTATEndpoint::OTATEndpoint(
	NMEndpointRef	inRef,
	NMUInt32			inMode)
		: OTEndpoint(inRef, inMode)
{
NMBoolean	success;

	//	Each endpoint stores its local and its remote address
	success = AllocAddressBuffer(&mStreamEndpoint->mLocalAddress);
	op_assert(success);
	OTInitDDPAddress((DDPAddress*)mStreamEndpoint->mLocalAddress.buf,0,0,0,0);

	success = AllocAddressBuffer(&mStreamEndpoint->mRemoteAddress);
	op_assert(success);
	OTInitDDPAddress((DDPAddress*)mStreamEndpoint->mRemoteAddress.buf,0,0,0,0);

	success = AllocAddressBuffer(&mDatagramEndpoint->mLocalAddress);
	op_assert(success);
	OTInitDDPAddress((DDPAddress*)mDatagramEndpoint->mLocalAddress.buf,0,0,0,0);

	success = AllocAddressBuffer(&mDatagramEndpoint->mRemoteAddress);
	op_assert(success);
	OTInitDDPAddress((DDPAddress*)mDatagramEndpoint->mRemoteAddress.buf,0,0,0,0);

	//	Alloc the address buffer for the TCall structure
	//	We need to use the largest possible size, which is an nbp address
	//	success = AllocAddressBuffer(&mCall.addr);
	//	op_assert(success);

	mCall.addr.buf = new NMUInt8[kNBPAddressLength];
	mCall.addr.len = mCall.addr.maxlen = kNBPAddressLength;

	success = AllocAddressBuffer(&mRcvUData.addr);
	op_assert(success);

	//	AppleTalk-specific stuff
	mConfirmationBytesLeft = sizeof(DDPAddress);
	mMapper = kOTInvalidMapperRef;
	mNameID = 0;
	mDisposer = NULL;
	bNotifyOnClose = false;
}

//----------------------------------------------------------------------------------------
// OTATEndpoint::~OTATEndpoint
//----------------------------------------------------------------------------------------

OTATEndpoint::~OTATEndpoint()
{

	//	Even though these member variables belong to the parent class,
	//	only the child class knows how to free them in the desructor

	if (mStreamEndpoint)
	{
		FreeAddressBuffer(&mStreamEndpoint->mLocalAddress);
		FreeAddressBuffer(&mStreamEndpoint->mRemoteAddress);
	}


	if (mDatagramEndpoint)
	{
		FreeAddressBuffer(&mDatagramEndpoint->mLocalAddress);
		FreeAddressBuffer(&mDatagramEndpoint->mRemoteAddress);
	}

	// 19990125 sjb changed delete to delete[] since this is an array
	delete[] mCall.addr.buf;

	FreeAddressBuffer(&mRcvUData.addr);

	if (mMapper != kOTInvalidMapperRef)
		OTCloseProvider(mMapper);
}

//----------------------------------------------------------------------------------------
// OTATEndpoint::Initialize
//----------------------------------------------------------------------------------------

// Stores the config information and creates the OT endpoints
NMErr
OTATEndpoint::Initialize(NMConfigRef inConfig)
{

	SetConfig((NMATConfigPriv *) inConfig);
	return kNMNoError;
}

//----------------------------------------------------------------------------------------
// OTATEndpoint::SetConfig
//----------------------------------------------------------------------------------------

void
OTATEndpoint::SetConfig(NMATConfigPriv *inConfig)
{
	if (inConfig == NULL)
	{
		mConfig.type = kModuleID;
		mConfig.version = kVersion;
		mConfig.customEnumDataLen = 0;
		mConfig.nbpName[0] = 0;
		mConfig.nbpZone[0] = 0;
		mConfig.nbpType[0] = 0;
	}
	else
	{
		mConfig = *inConfig;

		char	nbpBuf[kNBPEntityBufferSize];

		doCopyPStr(mConfig.nbpName, (StringPtr)nbpBuf);
		doConcatPStr((StringPtr)nbpBuf, "\p:");
		doConcatPStr((StringPtr)nbpBuf, mConfig.nbpType);

		doConcatPStr((StringPtr)nbpBuf, "\p@");
		doConcatPStr((StringPtr)nbpBuf, mConfig.nbpZone);

		p2cstr((StringPtr)nbpBuf);
		mNBPAddrLen = mNBPAddr.Init(nbpBuf);

	}
}

//----------------------------------------------------------------------------------------
// OTATEndpoint::DoRegisterName
//----------------------------------------------------------------------------------------

NMErr
OTATEndpoint::DoRegisterName(PrivateEndpoint *inEP)
{
	DEBUG_ENTRY_EXIT("OTATEndpoint::DoRegisterName");

	NMErr	 			status = kNMNoError;
	TRegisterRequest	req;
	TRegisterReply		rep;

	//Try_
	{
		mMapper = NM_OTOpenMapper(OTCreateConfiguration(kNBPName), 0, &status);
		DEBUG_NETWORK_API(inEP->mEP, "NM_OTOpenMapper", status);
		//ThrowIfOSErr_(status);
		if (status)
			goto error;

		req.name.buf = (NMUInt8 *) &mNBPAddr.fNBPNameBuffer;
		req.name.len = mNBPAddrLen - sizeof(OTAddressType);
		req.addr = inEP->mLocalAddress;
		req.flags = 0;

		rep.addr.buf = NULL;
		rep.addr.maxlen = 0;

		status = OTRegisterName(mMapper, &req, &rep);
		DEBUG_NETWORK_API(inEP->mEP, "OTRegisterName", status);

		//ThrowIfOSErr_(status);
		if (status)
			goto error;

		mNameID = rep.nameid;

	}
	//Catch_(code)
	error:
	if (status)
	{
		NMErr code = status;
		mNameID = 0;
		if (mMapper != kOTInvalidMapperRef)
		{
			OTCloseProvider(mMapper);
			mMapper = kOTInvalidMapperRef;
		}

		return code;
	}

	return status;
}

//----------------------------------------------------------------------------------------
// OTATEndpoint::DoUnregisterName
//----------------------------------------------------------------------------------------

NMErr
OTATEndpoint::DoUnregisterName(void)
{
	DEBUG_ENTRY_EXIT("OTATEndpoint::DoUnregisterName");

NMErr 			status = kOTNoError;

	if (mNameID == 0)
		return kOTNoError;

	//	This will start the ball rolling
	if (mMapper == kOTInvalidMapperRef)
	{
		status = OTAsyncOpenMapper(OTCreateConfiguration(kNBPName), 
			0, mMapperNotifier.fUPP, this);
		DEBUG_NETWORK_API(NULL, "OTAsyncOpenMapper", status);

		//	if we got an error, we can't remove the mapper, but we can at least try to kill the ep
		if (status != kNMNoError && mDisposer)
			OTEndpoint::DoCloseComplete(mDisposer, bNotifyOnClose);
	}
	else
	{
		OTInstallNotifier(mMapper, mMapperNotifier.fUPP, this);
		OTSetAsynchronous(mMapper);

		status = OTDeleteNameByID(mMapper, mNameID);
		DEBUG_NETWORK_API(NULL, "OTDeleteNameByID", status);

		//	if we got an error, we can't remove the mapper, but we can at least try to kill the ep
		if (status != kNMNoError && mDisposer)
			OTEndpoint::DoCloseComplete(mDisposer, bNotifyOnClose);
	}

	return status;
}

//----------------------------------------------------------------------------------------
// OTATEndpoint::DoBind
//----------------------------------------------------------------------------------------

NMErr
OTATEndpoint::DoBind(
	PrivateEndpoint	*inEP, 
	TBind			*inRequest, 
	TBind			*outReturned)
{
	NMErr			status = kNMNoError;
	NMBoolean		active;

	//Try_
	{
		status = OTBind(inEP->mEP, inRequest, outReturned);
		DEBUG_NETWORK_API(inEP->mEP, "OTBind", status);

		//ThrowIfOSErr_(status);
		if (status)
			goto error;

		active = (inRequest->qlen == 0);

		//	If we're passive, we should register the name as appropriate
		if (! active && (mMode == kNMDatagramMode || (inEP == mStreamEndpoint)))
		{
			// We've just opened a passive AppleTalk endpoint.  Let's make it visible
			// on the local machine...
			
			::OTIoctl(inEP->mEP, ATALK_IOC_FULLSELFSEND, (void *)1);

			status = DoRegisterName(inEP);
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
		DoUnregisterName();
		return code;
	}

	return status;
}

//----------------------------------------------------------------------------------------
// OTATEndpoint::AllocAddressBuffer
//----------------------------------------------------------------------------------------

NMBoolean
OTATEndpoint::AllocAddressBuffer(TNetbuf *ioBuf)
{
	ioBuf->buf = (NMUInt8 *) new DDPAddress;

	if (ioBuf->buf == NULL)
		return false;

	ioBuf->len = ioBuf->maxlen = sizeof(DDPAddress);

	return true;
}

//----------------------------------------------------------------------------------------
// OTATEndpoint::FreeAddressBuffer
//----------------------------------------------------------------------------------------

void
OTATEndpoint::FreeAddressBuffer(TNetbuf *inBuf)
{
	op_warn(inBuf != NULL && inBuf->buf != NULL);
	delete (DDPAddress *)inBuf->buf;
}

//----------------------------------------------------------------------------------------
// OTATEndpoint::FreeAddressBuffer
//----------------------------------------------------------------------------------------

NMBoolean	
OTATEndpoint::AddressesEqual(TNetbuf *inAddr1, TNetbuf *inAddr2)
{
	DEBUG_ENTRY_EXIT("OTATEndpoint::AddressesEqual");

	return OTCompareDDPAddresses((DDPAddress *) inAddr1->buf,(DDPAddress *)inAddr2->buf);
}

//----------------------------------------------------------------------------------------
// OTATEndpoint::GetConfigAddress
//----------------------------------------------------------------------------------------

//	Since we do nbp name registration separately, we don't care what address is used to bind
void
OTATEndpoint::GetConfigAddress(TNetbuf *outBuf, NMBoolean inActive)
{
	if (inActive)
	{
		outBuf->buf = (NMUInt8 *) &mNBPAddr;
		outBuf->len = mNBPAddrLen;
		outBuf->maxlen = kNBPEntityBufferSize;
	}
	else
	{
		outBuf->buf = 0;
		outBuf->len = 0;
	}
}

//----------------------------------------------------------------------------------------
// OTATEndpoint::SetConfigAddress
//----------------------------------------------------------------------------------------

//	For AppleTalk, this is an NBP address
void
OTATEndpoint::SetConfigAddress(TNetbuf *inBuf)
{
	machine_move_data(inBuf->buf, &mNBPAddr, inBuf->len);
	mNBPAddrLen = inBuf->len;
}

//----------------------------------------------------------------------------------------
// OTATEndpoint::GetIdentifier
//----------------------------------------------------------------------------------------

NMErr
OTATEndpoint::GetIdentifier(char* outIdStr, NMSInt16 inMaxSize)
{
	char result[256];
    TBind peerAddr;
    
	op_vassert_return((mStreamEndpoint != NULL),"Stream Endpoint is NIL!",  kNMBadStateErr);

    OSStatus err = OTGetProtAddress(mStreamEndpoint->mEP, NULL, &peerAddr);

    if (err != kNMNoError) {
        return err;
    }
    
	NBPAddress *addr = (NBPAddress *) peerAddr.addr.buf;
	op_vassert_return((addr->fAddressType == AF_ATALK_NBP),"Bad Endpoint Address Type!",  kNMInternalErr);

	sprintf(result, "%s", addr->fNBPNameBuffer);
	
	strncpy(outIdStr, result, inMaxSize - 1);
   outIdStr[inMaxSize - 1] = 0;

	return (kNMNoError);    
}


//----------------------------------------------------------------------------------------
// OTATEndpoint::MakeEnumerationResponse
//----------------------------------------------------------------------------------------

void
OTATEndpoint::MakeEnumerationResponse(void)
{
}

//----------------------------------------------------------------------------------------
// OTATEndpoint::SendOpenConfirmation
//----------------------------------------------------------------------------------------

// Confirm the connection is in place by sending the address of the datagram endpoint
// This is only used in uber mode
NMErr
OTATEndpoint::SendOpenConfirmation(void)
{
	DEBUG_ENTRY_EXIT("OTATEndpoint::SendOpenConfirmation");

TNetbuf	addrBuf;
OTResult result;


	//	Get the address of the datagram endpoint.
	addrBuf = mDatagramEndpoint->mLocalAddress;

	//	Just send the entire address buffer.  Makes it easy for the recipient
	result = OTSnd(mStreamEndpoint->mEP, addrBuf.buf, addrBuf.len, 0);

	op_warn(result == addrBuf.len);

	if (result < 0)		// it's only an error if it's negative
		DEBUG_NETWORK_API(0, "OTSnd", result);

	return (result  <0) ? result : kNMNoError;
}

//----------------------------------------------------------------------------------------
// OTATEndpoint::WaitForOpenConfirmation
//----------------------------------------------------------------------------------------

// wait for the other side to send us an open confirmation
NMErr
OTATEndpoint::WaitForOpenConfirmation(void)
{
	DEBUG_ENTRY_EXIT("OTATEndpoint::WaitForOpenConfirmation");

UnsignedWide	startTime;
NMUInt32			elapsedMilliseconds;
NMBoolean			keepWaiting = true;

	OTGetTimeStamp(&startTime);

	while (!bConnectionConfirmed &&
			(elapsedMilliseconds = OTElapsedMilliseconds(&startTime)) < mTimeout) {};

	if (! bConnectionConfirmed)
		return kNMTimeoutErr;

	return kNMNoError;
}

//----------------------------------------------------------------------------------------
// OTATEndpoint::HandleAsyncOpenConfirmation
//----------------------------------------------------------------------------------------

// Handle a confirmation message (we hope).  If it's not a confirmation
// message, then there's a bug in the module code
NMErr
OTATEndpoint::HandleAsyncOpenConfirmation(void)
{
	DEBUG_ENTRY_EXIT("OTATEndpoint::HandleAsyncOpenConfirmation");

NMUInt8 			*data;
TNetbuf			addrBuf;
DDPAddress		*addr;
OTFlags			flags = 0;
NMBoolean 		finished;
NMUInt32			ioSize;
NMErr		status = kNMNoError;

	//	it's possible that we will receive the confirmation in multiple packets
	//	we therefore need to keep track of how many we expect, and how many we get
	ioSize = mConfirmationBytesLeft;

	addrBuf = mDatagramEndpoint->mRemoteAddress;
	data = addrBuf.buf + (sizeof(DDPAddress) - mConfirmationBytesLeft);

	finished = OTReadBuffer(&mStreamEndpoint->mBufferInfo, data, &ioSize);

	mConfirmationBytesLeft -= ioSize;

	//	clean up so that our notifier will know the user read all the data
	if (finished)
	{
		OTReleaseBuffer(mStreamEndpoint->mBuffer);
		mStreamEndpoint->mBuffer = NULL;
	}
	else
	{
		status = kNMMoreDataErr;
	}

	op_warn(mConfirmationBytesLeft >= 0);

	if (mConfirmationBytesLeft == 0)
	{
		addr = (DDPAddress *) addrBuf.buf;
		bConnectionConfirmed = true;

	}

	return status;
}

//----------------------------------------------------------------------------------------
// OTATEndpoint::DoCloseComplete
//----------------------------------------------------------------------------------------

void
OTATEndpoint::DoCloseComplete(
	EndpointDisposer	*inEPDisposer,
	NMBoolean			inNotifyUser)
{
	DEBUG_ENTRY_EXIT("OTATEndpoint::DoCloseComplete");

	//	If we need to unregister the name, this'll run async
	if (mNameID != 0)
	{
		//	We have to remember these because we're doing an async unregister name
		bNotifyOnClose = inNotifyUser;
		mDisposer = inEPDisposer;

		DoUnregisterName();
	}
	else	// no name registered, just do the normal action
	{
		OTEndpoint::DoCloseComplete(inEPDisposer, inNotifyUser);
	}
}

//----------------------------------------------------------------------------------------
// OTATEndpoint::MapperNotifier
//----------------------------------------------------------------------------------------

pascal void
OTATEndpoint::MapperNotifier(
	void		*contextPtr, 
	OTEventCode	code,
	OTResult	result, 
	void		*cookie)
{
	DEBUG_ENTRY_EXIT("OTATEndpoint::MapperNotifier");

OTATEndpoint	*ep = (OTATEndpoint *) contextPtr;

	if (result != kNMNoError)
	{
		DEBUG_PRINT("Mapper Notifier got result %d", result);
		return;
	}

	switch (code)
	{
		case T_OPENCOMPLETE:
			ep->mMapper = (MapperRef) cookie;

			if (ep->mState == kClosing || ep->mState == kAborting)
			{
				OTDeleteNameByID(ep->mMapper, ep->mNameID);
			}
			break;

		case T_DELNAMECOMPLETE:
			EndpointDisposer *disposer = ep->mDisposer;
			op_warn(disposer != NULL);
			ep->mNameID = 0;

			OTCloseProvider(ep->mMapper);
			ep->mMapper = kOTInvalidMapperRef;

			if (ep->bNotifyOnClose)	// if we're aborting, the user never knew we were open anyway
				(*ep->mCallback)(ep->mRef, ep->mContext, kNMCloseComplete, 0, NULL);

			ep->mState = kDead;		// must happen before the call to CloseEPComplete
			CloseEPComplete(ep->mRef);

			delete disposer;
			break;

		default:
			break;
	}
}

//----------------------------------------------------------------------------------------
// OTATEndpoint::ResetAddressForUnreliableTransport
//----------------------------------------------------------------------------------------

//  This function changes the address type to the appropriate value for unreliable transport.
void
OTATEndpoint::ResetAddressForUnreliableTransport(OTAddress *inAddress)
{
	((DDPAddress *) inAddress)->fDDPType = 11;
}

