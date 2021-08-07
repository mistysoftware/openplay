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

#ifndef __CONFIGURATION__
#define __CONFIGURATION__

#include "OpenPlay.h"

//	------------------------------	Public Definitions

	#define LABEL_TERM          '='
	#define TOKEN_SEPARATOR     '\t'
	#define MAX_BIN_DATA_LEN    256

	#define STRING_DATA     0
	#define LONG_DATA       1
	#define	BOOLEAN_DATA    2
	#define BINARY_DATA     3	/* no greater than 256 bytes */

//	------------------------------	Public Functions

#ifdef __cplusplus
extern "C" {
#endif

/*	get_token	get data of the token_type with the token_label from the configuration string s.
 *
 *	returns true if the data was successfully retrieved.
 *	returns false if there was no data of that type, invalid data, or too much data for the buffer.
 *	io_length is size of buffer(in), and length of data retrieved(out).
 */

	extern NMBoolean get_token( const char * s, const char * token_label, short token_type, void * token_data, long * io_length );

/*	put_token	put data of the token_type with the token_label into the configuration string s
 *
 *	returns true if the data was successfully written
 *	returns false if the config string was not long enough to hold the data, or if the token_type was invalid
 */

	extern NMBoolean put_token( char * s, long s_max_length, const char * token_label, short token_type, void * token_data, long data_len );

#ifdef __cplusplus
}
#endif

#endif // __CONFIGURATION__

