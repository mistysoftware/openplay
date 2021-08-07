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

#ifndef __COTIPENDPOINT__
#define __COTIPENDPOINT__

//	------------------------------	Includes

	#ifndef __NETMODULE__
		#include "NetModule.h"
	#endif
		#include "CEndpoint_OP.h"

	#ifdef OP_API_NETWORK_OT
		#include <OpenTptInternet.h>
	#endif

//	------------------------------	Public Types


	class COTIPEndpoint : public CEndpoint
	{
	public:
		enum {host_Port = 3333};
		COTIPEndpoint(NSpGame *inGame);
		COTIPEndpoint(NSpGame *inGame,  EPCookie *inUnreliableEndpointCookie, EPCookie *inCookie);
		~COTIPEndpoint();

		NMErr 		InitAdvertiser(NSpProtocolPriv *inProt);
		NMErr		InitNonAdvertiser(NSpProtocolPriv *inProt);

	protected:
	//	virtual	void	 ResetAddressForUnreliableTransport(OTAddress *inAddress);
		virtual CEndpoint	*MakeCopy(EPCookie *inReliableCookie);
	//	virtual	const char *GetReliableProtocolName(void) {return kTCPName;}
	//	virtual	const char *GetUnreliableProtocolName(void) {return kUDPName;}
	//	virtual	void		DoPostCreate(PEndpointRef inEndpoint);
					
	};

#endif // __COTIPENDPOINT__
