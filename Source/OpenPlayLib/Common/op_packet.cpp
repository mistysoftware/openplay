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

#ifndef __OPENPLAY__
#include "OpenPlay.h"
#endif
#include "OPUtils.h"


//----------------------------------------------------------------------------------------
// ValidateCrossPlatformPacket
//----------------------------------------------------------------------------------------

/* Check alignment & padding */
NMErr 
ValidateCrossPlatformPacket(
	_packet_data_size *	packet_definition, 
	NMSInt16 			packet_definition_length,
	NMSInt16 			packet_length)
{
	NMSInt16 index, length;
	_packet_data_size *current = packet_definition;
	NMErr err = kNMNoError;

	op_assert(sizeof(NMSInt16)==2);
	op_assert(sizeof(NMSInt32)==4);

	length = 0;
	for(index= 0; !err && index<packet_definition_length; ++index)
	{
		if(*current < 0)
		{
			switch(*current)
			{
				case k2Byte:
					if(length & 1) err = kNMBadShortAlignmentErr;
					length += 2;
					break;

				case k4Byte:
					if(length & 3) err= kNMBadLongAlignmentErr;
					length += 4;
					break;
					
				default:
					err = kNMBadPacketDefinitionErr;
					break;
			}
		} else {
			length += *current;
		}
		current++;
	}
	
	if(length != packet_length)
	{
		err= kNMBadPacketDefinitionSizeErr;
	}

	return err;	
}

//----------------------------------------------------------------------------------------
// SwapCrossPlatformPacket
//----------------------------------------------------------------------------------------

NMErr 
SwapCrossPlatformPacket(
	_packet_data_size *packet_definition, 
	NMSInt16 packet_definition_length,
	void *packet_data, 
	NMSInt16 packet_length)
{
	NMSInt16 	index, length;
	_packet_data_size *current = packet_definition;
	BYTE *		data = (BYTE *) packet_data;
	NMErr 		err= kNMNoError;
	NMSInt32 	validateresult;
	
	op_assert(sizeof(NMSInt16)==2);
	op_assert(sizeof(NMSInt32)==4);

/* 19990129 sjb don't do work in op_assert, since it may not do whats in the op_assert params! */
	validateresult = ValidateCrossPlatformPacket(packet_definition, packet_definition_length, packet_length);
	op_assert(validateresult == 0);

	length = 0;	
	for(index = 0; index < packet_definition_length; ++index)
	{
		if(*current<0)
		{
			switch(*current)
			{
				case k2Byte:
					*((NMSInt16 *) data) = SWAP2(*((NMSInt16 *) data));
					data += sizeof(NMSInt16);
					length += sizeof(NMSInt16);
					break;

				case k4Byte:
					*((NMSInt32 *) data)= SWAP4(*((NMSInt32 *) data));  /* [Edmark/PBE] 11/10/99 changed SWAP2 to SWAP4 */
					data += sizeof(NMSInt32);
					length += sizeof(NMSInt32);
					break;
					
				default:
					err = kNMBadPacketDefinitionErr;
					break;
			}
		} else {
			data += *current;
			length += *current;
		}
		current++;
	}
	
	return err;
}
