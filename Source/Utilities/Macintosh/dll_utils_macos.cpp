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

#include "OPUtils.h"
#include "portable_files.h"

#define INCLUDE_MODULE_NAMES

#include "op_definitions.h"
#include "op_globals.h"
#include "op_resources.h"
#include "OPDLLUtils.h"
#include "String_Utils.h"

/* We must bind! */

/*-------------------------------Macintosh Section----------------------------*/


//----------------------------------------------------------------------------------------
// bind_to_library
//----------------------------------------------------------------------------------------

NMErr bind_to_library(
	FileDesc *file,
	ConnectionRef *conn_id)
{
	NMErr err= kNMNoError;
	Ptr main_address;
	char error_name[256];
		FSSpec	tempFSSpec;
		tempFSSpec = * (FSSpec *) file;

	//DEBUG_PRINT("Volume Ref: %ld\rPar ID: %ld\rFile Name: %#s.",tempFSSpec.vRefNum, tempFSSpec.parID, tempFSSpec.name);

	DEBUG_PRINT("Loading NetModule file: %#s.",tempFSSpec.name);

	/* again, should this be kLoadNewCopy? */
	err= GetDiskFragment(&tempFSSpec, 0L, kCFragGoesToEOF, 
		file->name, kLoadCFrag, conn_id, 
		&main_address, (unsigned char *)error_name);

	//ECF 011117 if we're running under MacOSX we unload the library and fail if it has a "ClassicOnly" export
	//this will keep lotsa people from wondering why appletalk isn't working under X...
	if(kNMNoError == err) //gmp 4/26/2004 added error code check
	{
		Boolean runningOSX = false;
		UInt32 response;

		//NOTE: Gestalt will return nonsense if OP is in an MP task! OP is not generally MP safe anyway...at this time!

		if(Gestalt(gestaltSystemVersion, (SInt32 *) &response) == noErr){
			if (response >= 0x1000)
				runningOSX = true;
			else
				runningOSX = false;
		}
		if (runningOSX){
			CFragSymbolClass sym_class;
			char *classicOnly = NULL;
			NMErr err2 = FindSymbol(*conn_id,(UInt8*)"\pClassicOnly",&classicOnly,&sym_class);
			if ((err2 == 0) && (classicOnly != NULL)){
				//unload the fragment and return an error
				err = kNMClassicOnlyModuleErr;
				CloseConnection(conn_id);
			}
		}
	}		

	if (err != kNMNoError)
	{
		char	msg[256];
		if (err == kNMClassicOnlyModuleErr)
			sprintf(msg, "%#s is a classic-only NetModule.  It was not loaded.",file->name);
		else
			sprintf(msg, "GetDiskFragment() Failed with Error %ld (Error \"%#s\")!", err, error_name);
		DEBUG_PRINT(msg);
	}
	else {
		//vpause("GetDiskFragment() Succeeded!");
	}
	
	return err;
}

//----------------------------------------------------------------------------------------
// load_proc
//----------------------------------------------------------------------------------------

void *load_proc(
	ConnectionRef	conn_id,
	NMSInt16		proc_id)
{
	Ptr module_info;
	char function_name[256];
	NMErr err;
	CFragSymbolClass sym_class;

 	op_assert(NUMBER_OF_MODULE_FUNCTIONS==NUMBER_OF_MODULES_TO_LOAD);
	op_assert(proc_id>=0 && proc_id<NUMBER_OF_MODULES_TO_LOAD);
	strcpy(function_name, module_names[proc_id]);
	c2pstr(function_name);
	err= FindSymbol(conn_id, (unsigned char *) function_name, &module_info, &sym_class);
	op_assert(err == noErr);
	
	return (void *) module_info;
}

//----------------------------------------------------------------------------------------
// free_library
//----------------------------------------------------------------------------------------

void free_library(
	ConnectionRef conn_id)
{
	CloseConnection(&conn_id);
}
