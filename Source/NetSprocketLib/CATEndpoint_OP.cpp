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


#include "NSpPrefix.h"
#include "CATEndpoint_OP.h"

#include "String_Utils.h"
#include "NSpGame.h"


//----------------------------------------------------------------------------------------
// COTATEndpoint::COTATEndpoint
//----------------------------------------------------------------------------------------
// Constructor that doesn't do any NBP registration

COTATEndpoint::COTATEndpoint(NSpGame *inGame) : CEndpoint(inGame)
{
}

//----------------------------------------------------------------------------------------
// COTATEndpoint::COTATEndpoint
//----------------------------------------------------------------------------------------

COTATEndpoint::COTATEndpoint(NSpGame *inGame, EPCookie *inUnreliableEndpointCookie, EPCookie *inCookie) : CEndpoint(inGame, inUnreliableEndpointCookie, inCookie)
{
	
}

//----------------------------------------------------------------------------------------
// COTATEndpoint::~COTATEndpoint
//----------------------------------------------------------------------------------------

COTATEndpoint::~COTATEndpoint()
{
}

//----------------------------------------------------------------------------------------
// COTATEndpoint::InitAdvertiser
//----------------------------------------------------------------------------------------

NMErr
COTATEndpoint::InitAdvertiser(NSpProtocolPriv *inProt)
{
//NMErr		status = kNMNoError;
		
//	mMaxRTT = inProt->GetMaxRTT();
//	mMinThruput = inProt->GetMinThruput();
	
//	return Init(1, kADSPName, kDDPName, NULL);	
	return Init(1, (PConfigRef)inProt);	

}

//----------------------------------------------------------------------------------------
// COTATEndpoint::InitNonAdvertiser
//----------------------------------------------------------------------------------------
NMErr
COTATEndpoint::InitNonAdvertiser(NSpProtocolPriv *inProt)
{
//	return Init(0, kADSPName, kDDPName, NULL);
	return Init(0, (PConfigRef)inProt);
}

/*

//----------------------------------------------------------------------------------------
// COTATEndpoint::ResetAddressForUnreliableTransport
//----------------------------------------------------------------------------------------
void
COTATEndpoint::ResetAddressForUnreliableTransport(OTAddress *inAddress)
{
	((DDPAddress *) inAddress)->fDDPType = 11;
}

//----------------------------------------------------------------------------------------
// COTATEndpoint::DoPostCreate
//----------------------------------------------------------------------------------------
//	Set the AppleTalk endpoint so that you can see it in NBP on the same machine
void
COTATEndpoint::DoPostCreate(PEndpointRef inEndpoint)
{

	NMSInt32 result = ::OTIoctl((EndpointRef) inEndpoint, ATALK_IOC_FULLSELFSEND, (void *)1);
	
}
*/

//----------------------------------------------------------------------------------------
// COTATEndpoint::MakeCopy
//----------------------------------------------------------------------------------------

CEndpoint *
COTATEndpoint::MakeCopy(EPCookie *inCookie)
{
	return  new COTATEndpoint(mGame, NULL, inCookie);
}

//----------------------------------------------------------------------------------------
// COTATEndpoint::Host
//----------------------------------------------------------------------------------------
//	This could be cleaned up a bit...
/* LR -- 020317 -- as in removed, it just calls thru to the CEndpont anyway, just use it! */
#if 0
NMBoolean COTATEndpoint::Host(NMBoolean inAdvertise)
{
	/*
//	MapperRef	mapper;
//	NMErr	status;
//	TRegisterRequest	req;
//	TRegisterReply		rep;
	
	if (inAdvertise == false && mNameID != 0)
	{		
		return CEndpoint::Host(inAdvertise);
	}
	else if (mNameID == 0)
	{
		return CEndpoint::Host(inAdvertise);

	}
	return true;
*/
	
	return CEndpoint::Host(inAdvertise);	
}
#endif
