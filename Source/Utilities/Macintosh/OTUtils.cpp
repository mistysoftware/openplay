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

//Ä	------------------------------	Includes
#include <OpenTransport.h>
#include <Gestalt.h>
#include <string.h>
#include <math.h>

#include "OTUtils.h"
#include "OPUtils.h"
#include "NSpPrefix.h"



//Ä	------------------------------	Private Definitions

//Ä	------------------------------	Private Types

enum {
	k11VersionID 		= 0x01100000, 
	k111VersionID 		= 0x01110000,
	kSys71Version11ID	= 0x00008000
};

//	Because they changed the constant names is 1.1.1.  Grrrr!
//enum {	kMyGestaltOpenTptPresent = 0x00000001,
//		kMyGestaltOpenTptAppleTalkPresent = 0x00000004,
//		kMyGestaltOpentTptTCPPresent = 0x00000010};

//Ä	------------------------------	Private Variables

NMBoolean	OTUtils::sOTGestaltTested = false;
NMSInt32	OTUtils::sOTGestaltResult = 0;
NMSInt32	OTUtils::sOTVersion = 0;
NMBoolean	OTUtils::sOTInitialized = false;

#ifdef OP_PLATFORM_MAC_CARBON_FLAG
OTClientContextPtr	gOTClientContext;
#endif // OP_PLATFORM_MAC_CARBON_FLAG


//Ä	------------------------------	Private Functions
//Ä	------------------------------	Public Variables


//----------------------------------------------------------------------------------------
// OTUtils::HasOpenTransport
//----------------------------------------------------------------------------------------

NMBoolean
OTUtils::HasOpenTransport(void)
{
	GetOTGestalt();
	return (sOTGestaltResult & gestaltOpenTptPresentMask) == gestaltOpenTptPresentMask;
}

//----------------------------------------------------------------------------------------
// OTUtils::Version111OrLater
//----------------------------------------------------------------------------------------

NMBoolean
OTUtils::Version111OrLater()
{
	::Gestalt(gestaltOpenTptVersions, &sOTVersion);

	return (sOTVersion == kSys71Version11ID || sOTVersion >= k111VersionID);
}

//----------------------------------------------------------------------------------------
// OTUtils::GetOTGestalt
//----------------------------------------------------------------------------------------

void
OTUtils::GetOTGestalt()
{
	if (false == sOTGestaltTested)
	{
		if (::Gestalt(gestaltOpenTpt, &sOTGestaltResult) == kNMNoError)
			sOTGestaltTested = true;
	}
}

//----------------------------------------------------------------------------------------
// OTUtils::HasOpenTransportTCP
//----------------------------------------------------------------------------------------

NMBoolean
OTUtils::HasOpenTransportTCP(void)
{
	GetOTGestalt();

	return (sOTGestaltResult & 
			(gestaltOpenTptPresentMask + gestaltOpenTptTCPPresentMask))
			== (gestaltOpenTptPresentMask + gestaltOpenTptTCPPresentMask);
}

//----------------------------------------------------------------------------------------
// OTUtils::HasOpenTransportAppleTalk
//----------------------------------------------------------------------------------------

NMBoolean
OTUtils::HasOpenTransportAppleTalk(void)
{
	GetOTGestalt();

	return (sOTGestaltResult & 
			(gestaltOpenTptPresentMask + gestaltOpenTptAppleTalkPresentMask))
			== (gestaltOpenTptPresentMask + gestaltOpenTptAppleTalkPresentMask);
}

//----------------------------------------------------------------------------------------
// OTUtils::StartOpenTransport
//----------------------------------------------------------------------------------------

//see OPUtils.h for a definition of how reserveMemory and chunkSize work (they only affect apps in OS-9 or earlier)
NMErr
OTUtils::StartOpenTransport( OTProtocolTest inProtocolTest, long reserveMemory, long reserveChunkSize)
{
	NMErr status;
	
	if (sOTInitialized == true)
		return (kNMNoError);
	else
	{
		//	Make sure we have OT, because we're going to be using it	
		if (HasOpenTransport() == false) 
			return (kNSpOTNotPresentErr);

		if (Version111OrLater() == false)
			return (kNSpOTVersionTooOldErr);
		
		if (inProtocolTest == kOTCheckForAppleTalk) {
#ifndef OP_PLATFORM_MAC_CARBON_FLAG
			if (!HasOpenTransportAppleTalk())
				return kNMProtocolInitFailedErr;
#endif
		}

		if (inProtocolTest == kOTCheckForTCPIP) {
			if (!HasOpenTransportTCP())
				return kNMProtocolInitFailedErr;
		}
		
#ifdef OP_PLATFORM_MAC_CARBON_FLAG
		status = ::InitOpenTransportInContext(kInitOTForExtensionMask, &gOTClientContext);
#else
		status = ::InitOpenTransport();
#endif

		//we only use reserve-memory if not in OS-X
		checkMacOSVersion();
		if (!gRunningOSX)
		{
			long reserveChunkCount = ceil((float)reserveMemory/reserveChunkSize);
			NMErr err = ::InitOTMemoryReserve(131072,reserveChunkSize,1,reserveChunkCount);
			#if DEBUG
			//	OTMemoryReserveTest();
			#endif //DEBUG
		}	
		
		if (status != kNMNoError)
			return (kNSpInitializationFailedErr);

		sOTInitialized = true;

		//Ä	OT has a memory problem that causes our preallocation to happen every time a new
		//Ä	game is created.  We therefore move the pre-allocation to here, since it will only be
		//Ä	called once per instance of the application
		//NMUInt8 *foo = (NMUInt8 *) InterruptSafe_alloc(75000);

		//if (NULL == foo)
		//	return (kNSpMemAllocationErr);
			
		//InterruptSafe_free(foo);

		return (status);
	}
}

//----------------------------------------------------------------------------------------
// OTUtils::CloseOpenTransport
//----------------------------------------------------------------------------------------

void
OTUtils::CloseOpenTransport(void)
{
	if (sOTInitialized)
	{

	//kill our reserve
	TermOTMemoryReserve();
	
#ifdef OP_PLATFORM_MAC_CARBON_FLAG
		::CloseOpenTransportInContext(gOTClientContext);
		gOTClientContext = NULL;
#else
		::CloseOpenTransport();
#endif // OP_PLATFORM_MAC_CARBON_FLAG

		sOTInitialized = false;
	}
}

//----------------------------------------------------------------------------------------
// OTUtils::DoNegotiateTCPNoDelayOption
//----------------------------------------------------------------------------------------

NMErr
OTUtils::DoNegotiateTCPNoDelayOption(EndpointRef ep, NMBoolean optValue)
{
NMUInt8			buf[kOTFourByteOptionSize];

TOption*		opt;
TOptMgmt        req;
NMErr	        err;
NMBoolean       isAsync = false;
        
	opt = (TOption*)buf;                            // set option ptr to buffer
	req.opt.buf     = buf;
	req.opt.len     = sizeof (buf);
	req.opt.maxlen = sizeof (buf);                   // were using ret for the
	                                                // return result also.
	req.flags       = T_NEGOTIATE;                  // negotiate for option


	opt->level      = INET_TCP;                      // dealing with an IP Level
	                                                // function
	opt->name       = TCP_NODELAY;
	opt->len        = kOTFourByteOptionSize;
	*(NMUInt32*)opt->value = optValue;				// set the desired option
	                                                // level, true or false

	if (OTIsSynchronous(ep) == false)               // check whether ep sync or
	                                                // not
	{
        isAsync = true;                 // set flag if async
        OTSetSynchronous(ep);                   // set endpoint to sync 
	}
	                        
	err = OTOptionManagement(ep, &req, &req);

	if (isAsync == true)                            // restore ep state if
	                                                // necessary
        OTSetAsynchronous(ep);
	        
	        // if no error then check the option status value
	if (err == kNMNoError)
	{
        if (opt->status != T_SUCCESS)   // if not T_SUCCESS, return
	                                                // the status
            err = opt->status;
	}       // otherwise return kNMNoError

	return err;		
}

//----------------------------------------------------------------------------------------
// OTUtils::DoNegotiateIPReuseAddrOption
//----------------------------------------------------------------------------------------

NMErr
OTUtils::DoNegotiateIPReuseAddrOption(EndpointRef ep, NMBoolean reuseState)
{
NMUInt8           buf[kOTFourByteOptionSize];     // define buffer for fourByte
                                                // Option size
TOption*        opt;                            // option ptr to make items
                                                // easier to access
TOptMgmt        req;
NMErr	        err;
NMBoolean       isAsync = false;
    
    opt = (TOption*)buf;                            // set option ptr to buffer
    req.opt.buf     = buf;
    req.opt.len     = sizeof (buf);
    req.opt.maxlen = sizeof (buf);                   // were using ret for the
                                                    // return result also.
    req.flags       = T_NEGOTIATE;                  // negotiate for option


    opt->level      = INET_IP;                      // dealing with an IP Level
                                                    // function
    opt->name       = kIP_REUSEADDR;
    opt->len        = kOTFourByteOptionSize;
    *(NMUInt32*)opt->value = reuseState;              // set the desired option
                                                    // level, true or false

    if (OTIsSynchronous(ep) == false)               // check whether ep sync or
                                                    // not
    {
        isAsync = true;                 // set flag if async
        OTSetSynchronous(ep);                   // set endpoint to sync 
    }
                            
    err = OTOptionManagement(ep, &req, &req);
    
    if (isAsync == true)                            // restore ep state if
                                                    // necessary
        OTSetAsynchronous(ep);
            
            // if no error then check the option status value
    if (err == kNMNoError)
    {
        if (opt->status != T_SUCCESS)   // if not T_SUCCESS, return
                                                    // the status
            err = opt->status;
    }       // otherwise return kNMNoError

    return err;
}

//----------------------------------------------------------------------------------------
// OTUtils::SetFourByteOption
//----------------------------------------------------------------------------------------

OTResult
OTUtils::SetFourByteOption(
		EndpointRef	inEndpoint,
		OTXTILevel	inLevel,
		OTXTIName	inName,
		NMUInt32		inValue)
{
OTResult	theErr;
TOption		option;
TOptMgmt	request;
TOptMgmt	result;

	//Ä	Set up the option buffer to specify the otiponi and inValue to set
	option.len		=	kOTFourByteOptionSize;
	option.level	=	inLevel;
	option.name		=	inName;
	option.status	=	0;
	option.value[0]	=	inValue;

	//Ä	Setup request parameter for OTOptionManagement
	request.opt.buf	=	(NMUInt8 *) &option;
	request.opt.len	=	sizeof (option);
	request.flags	=	T_NEGOTIATE;
	
	//Ä	Set up reply parameter for OTOptionManagement
	result.opt.buf		=	(NMUInt8 *) &option;
	result.opt.maxlen	=	sizeof (option);

	theErr = OTOptionManagement(inEndpoint, &request, &result);
	
	if (kNMNoError == theErr)
		if (T_SUCCESS != option.status)
			theErr = option.status;

	return (theErr);
}

//----------------------------------------------------------------------------------------
// OTUtils::MakeInetAddressFromString
//----------------------------------------------------------------------------------------

NMErr
OTUtils::MakeInetAddressFromString(const char *addrString, InetAddress *outAddr)
{
	MapperRef		mapper = NULL;
	//NMErr			err;
	TLookupRequest	req;
	TLookupReply	rep;
	NMUInt32		len;
	char			buf[400];	//€ This should be based on something other than paranoia
	NMErr			status = kNMNoError;	

	len = strlen(addrString);

	//	Open a mapper to convert this into an InetAddress
	mapper = NM_OTOpenMapper(OTCreateConfiguration(kDNRName), 0, &status);
	if (status)
		goto error;

	if (mapper == NULL){
		status = kNSpMemAllocationErr;
		goto error;
	}

	//	Prepare the request and reply structures
	req.name.maxlen = len;
	req.name.len = len;
	req.name.buf = (NMUInt8 *) addrString;
	req.addr.len = 0;
	req.maxcnt = 1;
	req.timeout = 0;
	req.flags = 0;
	
	rep.names.buf = (NMUInt8 *)buf;
	rep.names.maxlen = 400;
	
	status = ::OTLookupName(mapper, &req, &rep);
	if (status)
		goto error;
	
	//	OT gives us the reply is a bizarre format.  The InetAddress is +4 into the reply
	machine_move_data(buf + 4, outAddr, sizeof(InetAddress));
	
	::OTCloseProvider(mapper);

error:
	if (status)
	{			
		if (mapper)
			::OTCloseProvider(mapper);
	}
	return status;
}

//----------------------------------------------------------------------------------------
// OTUtils::MakeDDPNBPAddressFromString
//----------------------------------------------------------------------------------------

DDPNBPAddress *	
OTUtils::MakeDDPNBPAddressFromString(char *addrString)
{
	DDPNBPAddress		*address = NULL;	
	NMUInt32			addressLength;
	NMErr				status = kNMNoError;

	address = new DDPNBPAddress;
	if (address == NULL){
		status = kNSpMemAllocationErr;
		goto error;
	}
	addressLength = OTInitDDPNBPAddress(address, addrString, 0, 0, 0, AF_ATALK_FAMILY);

error:
	if (status)
	{
		if (address != NULL)
		{			
			delete address;
			address = NULL;
		}
	}
	return address;
}

//----------------------------------------------------------------------------------------
// OTUtils::MakeInetNameFromAddress
//----------------------------------------------------------------------------------------

NMErr
OTUtils::MakeInetNameFromAddress(const InetHost inHost, InetDomainName ioName)
{
	InetSvcRef		service;
	NMErr status = kNMNoError;

	//	Open a mapper to convert this into an InetAddress
#ifndef OP_PLATFORM_MAC_CARBON_FLAG
	service = OTOpenInternetServices(kDefaultInternetServicesPath, 0, &status);
#else
	service = OTOpenInternetServicesInContext(kDefaultInternetServicesPath, 0, &status, gOTClientContext);
#endif
	if (status)
		goto error;

	if (service == NULL){
		status = kNSpMemAllocationErr;
		goto error;
	}

	status = OTInetAddressToName(service, inHost, ioName);
	if (status)
		goto error;
	
	::OTCloseProvider(service);

error:
	if (status)
	{
		if (service)
			::OTCloseProvider(service);
	}
	return status;
}

//----------------------------------------------------------------------------------------
// OTUtils::CopyNetbuf
//----------------------------------------------------------------------------------------

NMBoolean
OTUtils::CopyNetbuf(TNetbuf *inSource, TNetbuf *ioDest)
{
	//	Make sure the dest is big enough for the source
	if (inSource->len > ioDest->maxlen)
	{
		DEBUG_PRINT("Source buffer %d, dest %d.  Couldn't copy.", inSource->len, ioDest->maxlen);
		return false;
	}

	machine_move_data(inSource->buf, ioDest->buf, inSource->len);
	ioDest->len = inSource->len;
	
	return true;
}