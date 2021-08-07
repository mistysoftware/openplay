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

#ifndef __IP_ENUMERATION__
#define __IP_ENUMERATION__

//	------------------------------	Includes

//	------------------------------	Public Types
	enum
	{
		kQueryFlag      = 0xFAFBFCFD,
		kReplyFlag      = 0xAFBFCFDF,
		kQuerySize      = 512,
		kMaxGameNameLen = 31, 
		kNMEnumDataLen  = 1024		//dair, increase data size from 256
	};

	typedef struct IPEnumerationResponsePacket
	{
		NMUInt32	responseCode;
		NMType		gameID;
		NMUInt32	version;
		NMUInt32	host;
		NMUInt16	port;
		NMUInt16	pad;
		char		name[kMaxGameNameLen+1];
		NMUInt32	customEnumDataLen;
	} IPEnumerationResponsePacket;

//	------------------------------	Public Functions


#ifdef __cplusplus
extern "C" {
#endif

	extern NMSInt32      build_ip_enumeration_request_packet(char *buffer);

	extern NMBoolean	is_ip_request_packet(char *buffer, NMUInt32 length, NMType gameID);

	extern NMSInt32      build_ip_enumeration_response_packet(char *buffer, NMType gameID, 
																NMUInt32 version, 
																NMUInt32 host, 
																NMUInt16 port, 
																char *name, 
																NMUInt32 custom_data_length,
																void *custom_data);

	extern void       byteswap_ip_enumeration_packet(char *buffer);
		
	extern void *     extract_enumeration_data_from_ip_response(char *buffer);

#ifdef __cplusplus
}
#endif

#endif // __IP_ENUMERATION__

