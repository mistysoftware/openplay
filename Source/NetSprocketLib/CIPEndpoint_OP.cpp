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

#include "CIPEndpoint_OP.h"

#include "NSpGame.h"


//----------------------------------------------------------------------------------------
// COTIPEndpoint::COTIPEndpoint
//----------------------------------------------------------------------------------------

COTIPEndpoint::COTIPEndpoint(NSpGame *inGame) : CEndpoint(inGame)
{
}

//----------------------------------------------------------------------------------------
// COTIPEndpoint::COTIPEndpoint
//----------------------------------------------------------------------------------------

COTIPEndpoint::COTIPEndpoint(NSpGame *inGame, EPCookie *inUnreliableEndpointCookie, EPCookie *inCookie) :
	CEndpoint(inGame, inUnreliableEndpointCookie, inCookie)
{

}

//----------------------------------------------------------------------------------------
// COTIPEndpoint::~COTIPEndpoint
//----------------------------------------------------------------------------------------

COTIPEndpoint::~COTIPEndpoint()
{
}

//----------------------------------------------------------------------------------------
// COTIPEndpoint::InitAdvertiser
//----------------------------------------------------------------------------------------

NMErr COTIPEndpoint::InitAdvertiser(NSpProtocolPriv *inProt)
{
	NMErr		status;
//	InetAddress		addr;
//	TNetbuf			netBuf;
//	IPInfo			*info = (IPInfo *)inProt->GetCustomData();
	
//	::OTInitInetAddress(&addr, info->port, 0);
	
//	Initialize our request buffer
//	netBuf.buf = (NMUInt8 *)&addr;
//	netBuf.len = sizeof(InetAddress);
//	netBuf.maxlen = sizeof(InetAddress);
	
//	status = Init(1, kTCPName, kUDPName, &netBuf);
	status = Init(1, (PConfigRef) inProt);
	
//	if (info->port == 0)
//		info->port = ((InetAddress *)mEndpointCookie->address)->fPort;
		
	return status;
}

//----------------------------------------------------------------------------------------
// COTIPEndpoint::InitNonAdvertiser
//----------------------------------------------------------------------------------------

NMErr COTIPEndpoint::InitNonAdvertiser(NSpProtocolPriv *inProt)
{
//	return Init(0, kTCPName, kUDPName, NULL);
	return Init(0, (PConfigRef)inProt);
}

/*

//----------------------------------------------------------------------------------------
// COTIPEndpoint::ResetAddressForUnreliableTransport
//----------------------------------------------------------------------------------------

void COTIPEndpoint::ResetAddressForUnreliableTransport(OTAddress *inAddress)
{
UNUSED_PARAMETER(inAddress);
}
*/

//----------------------------------------------------------------------------------------
// COTIPEndpoint::MakeCopy
//----------------------------------------------------------------------------------------

CEndpoint *COTIPEndpoint::MakeCopy(EPCookie *inCookie)
{
	return  new COTIPEndpoint(mGame, NULL, inCookie);
}

/*

//----------------------------------------------------------------------------------------
// COTIPEndpoint::DoPostCreate
//----------------------------------------------------------------------------------------

void COTIPEndpoint::DoPostCreate(PEndpointRef inEndpoint)
{
}
*/