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

#include "OTIPEnumerator.h"
#include "Exceptions.h"
#include "OTUtils.h"

//	------------------------------	Private Definitions
//	------------------------------	Private Types
//	------------------------------	Private Variables

NetNotifierUPP	OTIPEnumerator::mNotifier(OTIPEnumerator::Notifier);

//	------------------------------	Private Functions
//	------------------------------	Public Variables


//----------------------------------------------------------------------------------------
// OTIPEnumerator::OTIPEnumerator
//----------------------------------------------------------------------------------------

OTIPEnumerator::OTIPEnumerator(
	NMIPConfigPriv				*inConfig,
	NMEnumerationCallbackPtr	inCallback,
	void						*inContext,
	NMBoolean					inActive)
	: IPEnumerator(inConfig, inCallback, inContext, inActive)
{
	bDataWaiting = false;
	mEP = kOTInvalidEndpointRef;
	
	//	Setup the udata structure
	mIncomingData.addr.buf = (NMUInt8 *) &mAddress;
	mIncomingData.addr.len = mIncomingData.addr.maxlen = sizeof (InetAddress);
	mIncomingData.udata.buf = NULL;
	mLastQueryTimeStamp;
	mLastIdleTimeStamp;
	bFirstIdle = true;
	mEnumPeriod = 250;
	bGotInterfaceInfo = false;
}

//----------------------------------------------------------------------------------------
// OTIPEnumerator::~OTIPEnumerator
//----------------------------------------------------------------------------------------

OTIPEnumerator::~OTIPEnumerator()
{
}

//----------------------------------------------------------------------------------------
// OTIPEnumerator::StartEnumeration
//----------------------------------------------------------------------------------------

NMErr
OTIPEnumerator::StartEnumeration(void)
{
	TEndpointInfo	info;
	NMErr			status = kNMNoError;
	TBind			request;
	TOption			optBuf;
	//NMUInt8			optBuf[64];
	//NMUInt8			fooBuf[32];
	TOptMgmt		cmd;
	//NMUInt8 			*foo = fooBuf;

	
	//	If they don't want us to actively get the enumeration, there is nothing to do
	if (! bActive)
		return kNMNoError;
	
	//	first clear out any current items
	(mCallback)(mContext, kNMEnumClear, NULL);	// [Edmark/PBE] 11/16/99 added

	bFirstIdle = true;
	
	//	Create an OT endpoint
	mEP = OTOpenEndpoint(OTCreateConfiguration(kUDPName), 0, &info, &status);
	if (status)
		goto error;

	// fill in the option request
	cmd.flags = T_NEGOTIATE;
	cmd.opt.len = kOTFourByteOptionSize;
	cmd.opt.maxlen = kOTFourByteOptionSize;
	cmd.opt.buf = (NMUInt8*)&optBuf;

	// fill in the toption struct
	optBuf.len = sizeof(TOption);
	optBuf.level = INET_IP;
	optBuf.name = kIP_BROADCAST;
	optBuf.status = 0;
	optBuf.value[0] = 1;

/*
	cmd.opt.len = 0;
	cmd.opt.maxlen = 64;
	cmd.opt.buf = (NMUInt8*)optBuf;
	cmd.flags = T_NEGOTIATE;

	//	Option management kinda sucks
	strcpy((char *) fooBuf, "Broadcast = 1");
	status = OTCreateOptions(kRawIPName, (char **)&foo, &cmd.opt);
*/

	status = OTOptionManagement(mEP, &cmd, &cmd);
	if (status)
		goto error;
	
	//	Allocate the buffer for receiving the endpoint
	mIncomingData.udata.buf = (NMUInt8 *) InterruptSafe_alloc(info.tsdu);
	if (mIncomingData.udata.buf == NULL){
		status = kNMBadStateErr;
		goto error;
	}
	
	mIncomingData.udata.maxlen = info.tsdu;
	
	//	Bind it
	request.addr.buf = NULL;
	request.addr.len = 0;
	request.addr.maxlen = 0;
	request.qlen = 0;
	
	status = OTBind(mEP, &request, NULL);
	if (status)
		goto error;

	OTSetNonBlocking(mEP);
	
	//	Get our interface info (for the broadcast address)
	//	Do this after we bind so that we know the interface is live
	if (! bGotInterfaceInfo)
	{
		status = OTInetGetInterfaceInfo(&mInterfaceInfo, kDefaultInetInterface);
		if (status)
			goto error;
		
		bGotInterfaceInfo = true;
	}
	
	//	Install notifier
	status = OTInstallNotifier(mEP, mNotifier.fUPP, this);
	if (status)
		goto error;
	
	//	Make is asynchronous
	status = OTSetAsynchronous(mEP);
	if (status)
		goto error;
	
	//	Send out the query
	mEnumPeriod = 250;
	status = SendQuery();

error:
	if (status)
	{
		if (mEP)
		{
			OTCloseProvider(mEP);			// ignore error
			mEP = kOTInvalidEndpointRef;
		}
	}
	return status;
}

//----------------------------------------------------------------------------------------
// OTIPEnumerator::IdleEnumeration
//----------------------------------------------------------------------------------------

NMErr
OTIPEnumerator::IdleEnumeration(void)
{
NMErr	status = kNMNoError;
NMUInt32		timeSinceLastIdle;
	
	if (! bActive)
		return kNMNoError;
		
	if (bDataWaiting)
		status = ReceiveDatagram();
	
	if (bFirstIdle)
	{
		timeSinceLastIdle = 0;
		bFirstIdle = false;
	}
	else
	{
		timeSinceLastIdle = OTElapsedMilliseconds(&mLastIdleTimeStamp);
	}

	// dair, only update time stamp if at least 1ms has passed
	// Otherwise idling in a tight loop will invoke IdleList with a delta of 0,
	// which prevents servers from ever being aged and being unregistered.
	if (timeSinceLastIdle > 0)
	{
		OTGetTimeStamp(&mLastIdleTimeStamp);
		IdleList(timeSinceLastIdle);
	}
	
	if (OTElapsedMilliseconds(&mLastQueryTimeStamp) > mEnumPeriod)
	{
		if (mEnumPeriod < kMaxTimeBetweenPings)
			mEnumPeriod *= 2;

		SendQuery();
	}

	return status;
}

//----------------------------------------------------------------------------------------
// OTIPEnumerator::EndEnumeration
//----------------------------------------------------------------------------------------

NMErr
OTIPEnumerator::EndEnumeration(void)
{
NMErr	status;
	
	if (! bActive)
		return kNMNoError;
		
	if (mEP == kOTInvalidEndpointRef)
		return kNMNoError;
		
	status = OTUnbind(mEP);

	if (status == kNMNoError)
	{
		for (NMSInt32 i = 0; i < 1000; i++)
		{
			if (OTGetEndpointState(mEP) == T_UNBND)
				break;
		}
	}

	status = OTCloseProvider(mEP);
	
	InterruptSafe_free(mIncomingData.udata.buf);
	
	return kNMNoError;
}

//----------------------------------------------------------------------------------------
// OTIPEnumerator::Notifier
//----------------------------------------------------------------------------------------

pascal void
OTIPEnumerator::Notifier(
	void		*contextPtr,
	OTEventCode	code,
	OTResult	result,
	void		*cookie)
{
OTIPEnumerator	*theEnumerator = (OTIPEnumerator *) contextPtr;
	
	UNUSED_PARAMETER(result);
	UNUSED_PARAMETER(cookie);

	switch (code)
	{
		case T_DATA:
			theEnumerator->bDataWaiting = true;
		break;
	}
}

//----------------------------------------------------------------------------------------
// OTIPEnumerator::ReceiveDatagram
//----------------------------------------------------------------------------------------

NMErr
OTIPEnumerator::ReceiveDatagram(void)
{
	DEBUG_ENTRY_EXIT("IPEnumerator::ReceiveDatagram");

NMErr		status = kNMNoError;
OTResult	result;
OTFlags		flags = 0;
	
	do
	{
		result = OTRcvUData(mEP, &mIncomingData, &flags);

		if (result == kOTLookErr)
			HandleLookErr();
		else if (result == kNMNoError)
			HandleReply(mIncomingData.udata.buf, mIncomingData.udata.len, (InetAddress *) mIncomingData.addr.buf);

	} while (result != kNMNoError && result == kOTLookErr);

	if (result == kOTNoDataErr)
		bDataWaiting = false;
		
	return status;

}

//----------------------------------------------------------------------------------------
// OTIPEnumerator::SendQuery
//----------------------------------------------------------------------------------------

NMErr
OTIPEnumerator::SendQuery(void)
{
	DEBUG_ENTRY_EXIT("IPEnumerator::SendQuery");

TUnitData	udata;
OTResult	result;
InetAddress	broadcastAddr;
	
	//	Set up the unit data structure
	//	fBroadcastAddr doesn't seem to get set right, so construct it
	OTInitInetAddress(&broadcastAddr, mConfig.address.fPort, mInterfaceInfo.fAddress | ~mInterfaceInfo.fNetmask);

	udata.addr.buf = (NMUInt8 *) &broadcastAddr;
	udata.addr.len = sizeof (InetAddress);
	udata.opt.len = 0;
	udata.udata.len = kQuerySize;
	udata.udata.buf = mPacketData;
	
	do
	{
		result = OTSndUData(mEP, &udata);

		if (result < 0)
		{
			DEBUG_PRINT("OTSndUData returned an error: %ld", result);

			if (result == kOTLookErr)
				HandleLookErr();
		}
	} while (result != kNMNoError && result == kOTLookErr);
	
	//	Set the timestamp so that we'll know when to ping again
	OTGetTimeStamp(&mLastQueryTimeStamp);
	
	return result;
}

//----------------------------------------------------------------------------------------
// OTIPEnumerator::HandleLookErr
//----------------------------------------------------------------------------------------

void
OTIPEnumerator::HandleLookErr(void)
{
	DEBUG_ENTRY_EXIT("IPEnumerator::HandleLookErr");

OTResult	result;
NMErr	status;
TUDErr		udErr;
InetAddress	addr;
	
	udErr.addr.buf = (NMUInt8 *) &addr;
	udErr.addr.len = udErr.addr.maxlen = sizeof(InetAddress);
	udErr.opt.maxlen = 0;
	
	
	result = OTLook(mEP);
	DEBUG_PRINT("OTLook returned %ld", result);

	switch (result)
	{
		case T_DATA:
			ReceiveDatagram();
			break;
			
		case T_UDERR:
			status = OTRcvUDErr(mEP, &udErr);
			DEBUG_PRINT("OTRcvUDErr returned %ld", status);
			break;

		default:
			DEBUG_PRINT("Unhandled look error!");
			break;
	}
}
