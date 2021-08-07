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

//	------------------------------	Includes

#include <OpenTptClient.h>

#include "OTLRdzEndpoint.h" // PAB - This needs to use the Rdz version
#include "OTUtils.h"
#include "Exceptions.h"


//	------------------------------	Private Definitions
//	------------------------------	Private Types
//	------------------------------	Private Variables
//	------------------------------	Private Functions
//	------------------------------	Public Variables


//----------------------------------------------------------------------------------------
// OTIPEndpoint::OTIPEndpoint
//----------------------------------------------------------------------------------------

OTIPEndpoint::OTIPEndpoint(
	NMEndpointRef	inRef,
	NMUInt32			inMode)
		: OTEndpoint(inRef, inMode)
{	
NMBoolean	success;
	
	//	Each endpoint stores its local and its remote address
	success = AllocAddressBuffer(&mStreamEndpoint->mLocalAddress);
	op_assert(success);
	OTInitInetAddress((InetAddress *)mStreamEndpoint->mLocalAddress.buf,0,0);
	
	success = AllocAddressBuffer(&mStreamEndpoint->mRemoteAddress);
	op_assert(success);
	OTInitInetAddress((InetAddress *)mStreamEndpoint->mRemoteAddress.buf,0,0);

	success = AllocAddressBuffer(&mDatagramEndpoint->mLocalAddress);
	op_assert(success);
	OTInitInetAddress((InetAddress *)mDatagramEndpoint->mLocalAddress.buf,0,0);
	
	success = AllocAddressBuffer(&mDatagramEndpoint->mRemoteAddress);
	op_assert(success);
	OTInitInetAddress((InetAddress *)mDatagramEndpoint->mRemoteAddress.buf,0,0);
	
	//	Alloc the address buffer for the TCall structure
	success = AllocAddressBuffer(&mCall.addr);
	op_assert(success);
		
	success = AllocAddressBuffer(&mRcvUData.addr);
	op_assert(success);

	mEnumerationResponseData = NULL;
	mEnumerationResponseLen = 0;

	mPreprocessBuffer = new NMUInt8[kQuerySize];
}

//----------------------------------------------------------------------------------------
// OTIPEndpoint::~OTIPEndpoint
//----------------------------------------------------------------------------------------

OTIPEndpoint::~OTIPEndpoint()
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
	if (mCall.addr.buf != NULL)
		FreeAddressBuffer( &mCall.addr);

	FreeAddressBuffer(&mRcvUData.addr);

	// sjb 19990201 use array delete operator for arrays!
	delete[] mEnumerationResponseData;
	delete[] mPreprocessBuffer;
}

//----------------------------------------------------------------------------------------
// OTIPEndpoint::Initialize
//----------------------------------------------------------------------------------------

NMErr
OTIPEndpoint::Initialize(NMConfigRef inConfig)
{
		
	SetConfig((NMIPConfigPriv *) inConfig);

	return (kNMNoError);
}

//----------------------------------------------------------------------------------------
// OTIPEndpoint::SetConfig
//----------------------------------------------------------------------------------------

void
OTIPEndpoint::SetConfig(NMIPConfigPriv *inConfig)
{
	if (inConfig == NULL)
	{
		mConfig.type = kModuleID;
		mConfig.version = kVersion;
		mConfig.customEnumDataLen = 0;
		OTInitInetAddress((InetAddress *) &mConfig.address, 0, 0);
	}
	else
	{
		mConfig = *inConfig;
	}
}

//----------------------------------------------------------------------------------------
// OTIPEndpoint::DoBind
//----------------------------------------------------------------------------------------

NMErr
OTIPEndpoint::DoBind(
	PrivateEndpoint	*inEP,
	TBind			*inRequest,
	TBind			*outReturned)
{
	NMErr	status = kNMNoError;
	NMBoolean	active;
	
	//Try_
	{
		active = (inRequest->qlen == 0);
		
		if (inEP == mStreamEndpoint  && !active)
		{
			status = OTUtils::DoNegotiateIPReuseAddrOption(inEP->mEP, true);
			DEBUG_NETWORK_API(inEP->mEP, "Negotiate Reuse Option", status);	
			status = OTUtils::DoNegotiateTCPNoDelayOption(inEP->mEP, true);
			DEBUG_NETWORK_API(inEP->mEP, "Negotiate No Delay Option", status);
		}			
		
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
// OTIPEndpoint::AllocAddressBuffer
//----------------------------------------------------------------------------------------

NMBoolean
OTIPEndpoint::AllocAddressBuffer(TNetbuf *ioBuf)
{
	ioBuf->buf = (NMUInt8 *) new InetAddress;

	if (ioBuf->buf == NULL)
		return false;

	ioBuf->len = ioBuf->maxlen = sizeof(InetAddress);
	
	return true;
}

//----------------------------------------------------------------------------------------
// OTIPEndpoint::FreeAddressBuffer
//----------------------------------------------------------------------------------------

void
OTIPEndpoint::FreeAddressBuffer(TNetbuf *inBuf)
{
	op_warn(inBuf != NULL && inBuf->buf != NULL);
	delete (InetAddress *)inBuf->buf;
}

//----------------------------------------------------------------------------------------
// OTIPEndpoint::AddressesEqual
//----------------------------------------------------------------------------------------

NMBoolean
OTIPEndpoint::AddressesEqual(TNetbuf *inAddr1, TNetbuf *inAddr2)
{
	DEBUG_ENTRY_EXIT("OTIPEndpoint::AddressesEqual");

	InetAddress *addr1 = (InetAddress *) inAddr1->buf;
	InetAddress *addr2 = (InetAddress *) inAddr2->buf;
	
	return (addr1->fPort == addr2->fPort &&	addr1->fHost == addr2->fHost);
}

//----------------------------------------------------------------------------------------
// OTIPEndpoint::GetConfigAddress
//----------------------------------------------------------------------------------------

//	For a passive connect, this contains the port that we want to listen on
void
OTIPEndpoint::GetConfigAddress(TNetbuf *outBuf, NMBoolean inActive)
{
	outBuf->buf = (NMUInt8 *) &mConfig.address;
	outBuf->len = outBuf->maxlen = sizeof(InetAddress);
}

//----------------------------------------------------------------------------------------
// OTIPEndpoint::SetConfigAddress
//----------------------------------------------------------------------------------------

void
OTIPEndpoint::SetConfigAddress(TNetbuf *inBuf)
{
	InetAddress *addr = (InetAddress *) inBuf->buf;
	OTInitInetAddress((InetAddress *)&mConfig.address, addr->fPort,addr->fHost);
}

//----------------------------------------------------------------------------------------
// OTIPEndpoint::GetIdentifier
//----------------------------------------------------------------------------------------

NMErr
OTIPEndpoint::GetIdentifier(char* outIdStr, NMSInt16 inMaxSize)
{
	char result[256];
    TBind peerAddr;
    
	op_vassert_return((mStreamEndpoint != NULL),"Stream Endpoint is NIL!",  kNMBadStateErr);

    OSStatus err = OTGetProtAddress(mStreamEndpoint->mEP, NULL, &peerAddr);

    if (err != kNMNoError) {
        return err;
    }
    
	InetAddress *addr = (InetAddress *) peerAddr.addr.buf;
	op_vassert_return((addr->fAddressType == AF_INET),"Bad Endpoint Address Type!",  kNMInternalErr);

   unsigned char *addrp = (unsigned char*)&addr->fHost;

	sprintf(result, "%u.%u.%u.%u", addrp[0], addrp[1], addrp[2], addrp[3]);
	
	strncpy(outIdStr, result, inMaxSize - 1);
    outIdStr[inMaxSize - 1] = 0;

	return (kNMNoError);    
}
        
//----------------------------------------------------------------------------------------
// OTIPEndpoint::MakeEnumerationResponse
//----------------------------------------------------------------------------------------

void
OTIPEndpoint::MakeEnumerationResponse(void)
{
	DEBUG_ENTRY_EXIT("OTIPEndpoint::MakeEnumerationResponse");

	mEnumerationResponseLen = sizeof(IPEnumerationResponsePacket) + mConfig.customEnumDataLen;

	//	in case we created one from a previous listen...
	if (mEnumerationResponseData)
		delete[] mEnumerationResponseData;
		
	mEnumerationResponseData = (NMUInt8 *)new char[mEnumerationResponseLen];

	IPEnumerationResponsePacket *theResponse = (IPEnumerationResponsePacket *) mEnumerationResponseData;
	InetInterfaceInfo info;
	OSStatus		status;
	TNetbuf			addr;
	NMSInt16		len;
	
	addr = mStreamEndpoint->mLocalAddress;
	status = OTInetGetInterfaceInfo(&info, kDefaultInetInterface);
	if (status == kNMNoError)
		((InetAddress *)addr.buf)->fHost = info.fAddress;
	
	len = build_ip_enumeration_response_packet((char *) theResponse, mConfig.gameID, kVersion,
			info.fAddress, ((InetAddress *)addr.buf)->fPort, mConfig.name, mConfig.customEnumDataLen,
			mConfig.customEnumData);
			
	op_assert(len <= mEnumerationResponseLen);
}

//----------------------------------------------------------------------------------------
// OTIPEndpoint::SendOpenConfirmation
//----------------------------------------------------------------------------------------

NMErr
OTIPEndpoint::SendOpenConfirmation(void)
{
	DEBUG_ENTRY_EXIT("OTIPEndpoint::SendOpenConfirmation");

NMUInt8 		*data;
TNetbuf		addrBuf;
InetAddress	*addr;
OTResult 	result;
	
	addrBuf = mDatagramEndpoint->mLocalAddress;
	addr = (InetAddress *) addrBuf.buf;
	data = (NMUInt8 *) &addr->fPort;
		
	result = OTSnd(mStreamEndpoint->mEP, data, sizeof (InetPort), 0);
	op_warn(result == sizeof (InetPort));
	
	return (result  < 0) ? result : kNMNoError;
}

//----------------------------------------------------------------------------------------
// OTIPEndpoint::WaitForOpenConfirmation
//----------------------------------------------------------------------------------------

NMErr
OTIPEndpoint::WaitForOpenConfirmation(void)
{
	DEBUG_ENTRY_EXIT("OTIPEndpoint::WaitForOpenConfirmation");

UnsignedWide		startTime;
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
// OTIPEndpoint::HandleAsyncOpenConfirmation
//----------------------------------------------------------------------------------------

NMErr
OTIPEndpoint::HandleAsyncOpenConfirmation(void)
{
	DEBUG_ENTRY_EXIT("OTIPEndpoint::HandleAsyncOpenConfirmation");

NMUInt8 	*data;
InetAddress	*addr;
InetPort	port;
OTFlags		flags = 0;
NMBoolean 	finished;
NMUInt32	ioSize = sizeof (InetPort);
OSStatus	status = kNMNoError;

	data = (NMUInt8 *) &port;
	
	finished = OTReadBuffer(&mStreamEndpoint->mBufferInfo, data, &ioSize);	
	op_warn(ioSize == sizeof (InetPort));
	
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
	
	bConnectionConfirmed = true;

	//	Now initialize our datagram remote address
	OTUtils::CopyNetbuf(&mStreamEndpoint->mRemoteAddress, &mDatagramEndpoint->mRemoteAddress);
	addr = (InetAddress *) mDatagramEndpoint->mRemoteAddress.buf;
	addr->fPort = port;
	//DEBUG_PRINT("remote port is %d",port);
		
	return status;
}

//----------------------------------------------------------------------------------------
// OTIPEndpoint::PreprocessPacket
//----------------------------------------------------------------------------------------

// This function is called on every datagram packet, so do it quickly!
NMErr
OTIPEndpoint::PreprocessPacket(void)
{
	DEBUG_ENTRY_EXIT("OTIPEndpoint::PreprocessPacket");

NMSInt32 	i;
NMBoolean	isQuery = true;
NMUInt8 	*data = mPreprocessBuffer;
NMUInt32 	len, bufLen = OTBufferDataSize(mDatagramEndpoint->mBuffer);
NMUInt32	*block;
	
	//	Do the quick check
	if (bufLen != kQuerySize)
		return kWasUserData; 


	len = sizeof (NMUInt32);
	OTInitBufferInfo(&mDatagramEndpoint->mBufferInfo, mDatagramEndpoint->mBuffer);

	//	Read only the first 4 bytes to check for a query flag
	OTReadBuffer(&mDatagramEndpoint->mBufferInfo, data, &len);	
	
	if (* (NMUInt32 *) data != kQueryFlag)
		return kWasUserData;

	data += sizeof (NMUInt32);
	len = bufLen - sizeof (NMUInt32);
	
	//	Read the rest
	OTReadBuffer(&mDatagramEndpoint->mBufferInfo, data, &len);	

	//	This COULD be an enumeration query.  Verify
	i = (kQuerySize / sizeof (NMUInt32)) - 1;	//iterations of looking at UInt32s
	block = (NMUInt32 *) data;

	do
	{
		isQuery = (*block++ == kQueryFlag);
	} while ((--i > 0) && isQuery);
		
	//	Now, if it was a query then we need to respond
	return (isQuery ? kWasQuery : kWasUserData);
}

//----------------------------------------------------------------------------------------
// OTIPEndpoint::ResetAddressForUnreliableTransport
//----------------------------------------------------------------------------------------

//  This function currently does nothing for IP.
void 
OTIPEndpoint::ResetAddressForUnreliableTransport(OTAddress *inAddress)
{
	UNUSED_PARAMETER(inAddress);
}

