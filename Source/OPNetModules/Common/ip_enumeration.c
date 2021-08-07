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

/*	------------------------------	Includes */

#include "OpenPlay.h"
#include "OPUtils.h"

#include "ip_enumeration.h"

/*	------------------------------	Private Definitions */
/*	------------------------------	Private Types */
/*	------------------------------	Private Variables */
/*	------------------------------	Private Functions */
/*	------------------------------	Public Variables */

/*	--------------------	build_ip_enumeration_request_packet */


//----------------------------------------------------------------------------------------
// build_ip_enumeration_request_packet
//----------------------------------------------------------------------------------------

NMSInt32
build_ip_enumeration_request_packet(char *buffer)
{
NMUInt32	*request_packet = (NMUInt32 *) buffer;
NMSInt16	index;
NMFlags		query_value= kQueryFlag;
	
#ifdef little_endian
	query_value = SWAP4(query_value);
#endif
	
	for (index = 0; index<kQuerySize / sizeof (NMUInt32); ++index)
	{
		request_packet[index] = query_value;
	}

	return kQuerySize;
}

//----------------------------------------------------------------------------------------
// is_ip_request_packet
//----------------------------------------------------------------------------------------

NMBoolean
is_ip_request_packet(
	char		*buffer, 
	NMUInt32	length,
	NMType		gameID)
{
  NMBoolean  is_request = false;

  UNUSED_PARAMETER(gameID)   /* should probably use this.. */
	
	if (length == kQuerySize)
	{
	NMSInt16		index;
	NMUInt32	*query_flag = (NMUInt32 *) buffer;
	NMUInt32	query_value = kQueryFlag;
	
#ifdef little_endian
		query_value = SWAP4(query_value);
#endif
		
		for (index = 0, query_flag = (NMUInt32 *) buffer; 
			index<kQuerySize / sizeof (NMUInt32); 
			++index, query_flag++)
		{
			if (*query_flag != query_value)
				break;
		}
		
		if (index == kQuerySize / sizeof (NMUInt32))
			is_request = true;
	}
	
	return is_request;
}

//----------------------------------------------------------------------------------------
// build_ip_enumeration_response_packet
//----------------------------------------------------------------------------------------

NMSInt32
build_ip_enumeration_response_packet(
	char		*buffer, 
	NMType		gameID, 
	NMUInt32	version, 
	NMUInt32	host, 
	NMUInt16	port, 
	char		*name, 
	NMUInt32	custom_data_length,
	void		*custom_data)
{
IPEnumerationResponsePacket	*packet = (IPEnumerationResponsePacket *) buffer;
NMSInt32  				length = sizeof (IPEnumerationResponsePacket);
	
	packet->responseCode = kReplyFlag;
	packet->gameID = gameID;
	packet->host = host;
	packet->port = port;
	packet->version = version;
	machine_copy_data(name, packet->name, kMaxGameNameLen + 1);
	packet->name[kMaxGameNameLen] = 0;
	packet->customEnumDataLen = custom_data_length;

	if (custom_data_length)
	{
		machine_copy_data(custom_data,(char *) packet + sizeof (IPEnumerationResponsePacket), custom_data_length);

		length += custom_data_length;
	}
	
	return length;
}

//----------------------------------------------------------------------------------------
// byteswap_ip_enumeration_packet
//----------------------------------------------------------------------------------------

void
byteswap_ip_enumeration_packet(char *buffer)
{
	IPEnumerationResponsePacket	*packet = (IPEnumerationResponsePacket *) buffer;
	
#if (little_endian)
	packet->responseCode = SWAP4(packet->responseCode);
	packet->gameID = SWAP4(packet->gameID);
	packet->version = SWAP4(packet->version);
	packet->host = SWAP4(packet->host);
	packet->port = SWAP2(packet->port);
	packet->pad = SWAP2(packet->pad);
	packet->customEnumDataLen = SWAP4(packet->customEnumDataLen);
#endif
}

//----------------------------------------------------------------------------------------
// extract_enumeration_data_from_ip_response
//----------------------------------------------------------------------------------------

void *
extract_enumeration_data_from_ip_response(char *buffer)
{
IPEnumerationResponsePacket	*packet = (IPEnumerationResponsePacket *) buffer;

	op_assert(packet->responseCode == kReplyFlag);

	return (buffer + sizeof (IPEnumerationResponsePacket));
}
