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

#ifndef __OTIPENDPOINT__
#define __OTIPENDPOINT__

//	------------------------------	Includes

	#include <OpenTptAppleTalk.h>

	#include "OTEndpoint.h"
	#include "NetModuleIP.h"

//	------------------------------	Public Types

	class OTIPEndpoint : public OTEndpoint
	{
	public:
		OTIPEndpoint(NMEndpointRef inRef, NMUInt32 inMode = kNMNormalMode);
		virtual ~OTIPEndpoint();
		
		virtual	NMErr		Initialize(NMConfigRef inConfig);
		
				void		SetConfig(NMIPConfigPriv *inConfig);
				
		virtual const char *GetStreamProtocolName(void) { return kTCPName;}
		virtual const char	*GetStreamListenerProtocolName(void) {return "tilisten, tcp";}	// because we can use the "tilisten" modifier
		virtual const char *GetDatagramProtocolName(void) {return kUDPName;}
		
		virtual	void	GetConfigAddress(TNetbuf *outBuf, NMBoolean inActive);
		virtual void	SetConfigAddress(TNetbuf *inBuf);
		virtual NMErr	GetIdentifier(char* outIdStr, NMSInt16 inMaxSize);

	protected:
		virtual	NMErr	DoBind(PrivateEndpoint *inEP, TBind *inRequest, TBind *outReturned);
		virtual void	MakeEnumerationResponse(void);
		virtual NMErr	PreprocessPacket(void);
		virtual	NMErr	SendOpenConfirmation(void);
		virtual	NMErr	WaitForOpenConfirmation(void);
		virtual	NMErr	HandleAsyncOpenConfirmation(void);
		
		//	Functions for doing the Right Thing with addresses
		virtual NMBoolean	AllocAddressBuffer(TNetbuf *ioBuf);
		virtual void	FreeAddressBuffer(TNetbuf *inBuf);
		virtual NMBoolean	AddressesEqual(TNetbuf *inAddr1, TNetbuf *inAddr2);
		virtual	void	ResetAddressForUnreliableTransport(OTAddress *inAddress);
		
		NMIPConfigPriv		mConfig;
	};

#endif // __OTIPENDPOINT__
