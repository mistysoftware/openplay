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

#include "ATPassThrus.h"

//	------------------------------	Private Definitions
//	------------------------------	Private Types
//	------------------------------	Private Variables
//	------------------------------	Private Functions
//	------------------------------	Public Variables

//	--------------------	NMATSetLookupMethod


//----------------------------------------------------------------------------------------
// NMATSetLookupMethod
//----------------------------------------------------------------------------------------

NMErr
NMATSetLookupMethod(NMEndpointRef ep, long method)
{
NMErr					err;
SetLookupMethodPB	pb;
	
	pb.method = method;
	
	err = NMFunctionPassThrough(ep, kSetLookupMethodSelector, &pb);
	
	return err;
}

//----------------------------------------------------------------------------------------
// NMATSetSearchZones
//----------------------------------------------------------------------------------------

NMErr
NMATSetSearchZones(NMEndpointRef ep, StringPtr zones)
{
NMErr				err;
SetSearchZonesPB	pb;
	
	pb.zones = zones;
	
	err = NMFunctionPassThrough(ep, kSetSearchZonesSelector, &pb);
	
	return err;
}

//----------------------------------------------------------------------------------------
// NMATGetZoneList
//----------------------------------------------------------------------------------------

NMErr
NMATGetZoneList(NMEndpointRef ep, StringPtr *zoneList)
{
NMErr			err;
GetZoneListPB	pb;
	
	pb.zones = zoneList;
	
	err = NMFunctionPassThrough(ep, kGetZoneListSelector, &pb);
	
	return err;
}

//----------------------------------------------------------------------------------------
// NMATDisposeZoneList
//----------------------------------------------------------------------------------------

NMErr
NMATDisposeZoneList(NMEndpointRef ep, StringPtr zoneList)
{
NMErr				err;
DisposeZoneListPB	pb;
	
	pb.zones = zoneList;
	
	err = NMFunctionPassThrough(ep, kDisposeZoneListSelector, &pb);
	
	return err;
}
