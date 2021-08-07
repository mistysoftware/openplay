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

#ifndef __FIND_FILES__
#define __FIND_FILES__

#include "OPUtils.h"

//	------------------------------	Public Definitions

	#define WILDCARD_TYPE '****'
	#define FIND_FILE_VERSION (0)

	enum
	{
		_fill_buffer, 			/* Fill the buffer with matches */
		_callback_only			/* Ignore the buffer, and call the callback for each. */
	};

	enum
	{
		_ff_create_buffer= 0x0001,	/* Create the destination buffer, free later w/ Dispose */
		_ff_alphabetical= 0x0002,	/* Matches are returned in alphabetical order */
		_ff_recurse= 0x0004,		/* Recurse when I find a subdirectory */
		_ff_callback_with_catinfo= 0x0008 /* Callback with CInfoPBRec as second paramter */
	};

//	------------------------------	Public Types

	struct find_file_pb {
		NMSInt16 version;			/* Version Control (Set to 0)		<-  */
		NMSInt16 flags;			/* Search flags 			<-  */
		NMSInt16 search_type;		/* Search type				<-  */

		FileDesc start_search_from;	/* Start of search (for macintosh, ignores name) */
		FileType type_to_find;		/* OSType to find			<-  */
		
		FileDesc *buffer; 		/* Destination				<-> */
		NMSInt16 max;			/* Maximum matches to return		<-  */
		NMSInt16 count;			/* Count of matches found 		->  */

		/* Callback	functions, if returns TRUE, you want to add it.  If */
		/*  callback==NULL, adds all found.					<-  */
		NMBoolean (*callback)(FileDesc *file, void *data);
		void *user_data;		/* Passed to callback above.		<-  */
	};
	typedef struct find_file_pb	find_file_pb;


//	------------------------------	Public Functions
	
	/* Find the files! */
	NMErr find_files( find_file_pb *param_block);

	/* Useful function for comparing FSSpecs... */
	NMBoolean equal_filedescs(FileDesc *a, FileDesc *b);


#endif // __FIND_FILES__
