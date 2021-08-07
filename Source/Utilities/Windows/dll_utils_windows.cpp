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

#include "OpenPlay.h"
#include "OPUtils.h"
#include "portable_files.h"

#define INCLUDE_MODULE_NAMES
#include "NetModulePrivate.h" // We had to include NetModulePrivate immediately for VC++ otherwise gave multipe definition warning....
#undef INCLUDE_MODULE_NAMES

#include "op_definitions.h"
#include "op_globals.h"
#include "op_resources.h"
#include "OPDLLUtils.h"


/* We must bind! */
/*--------------------------------Windows Section-----------------------------*/
	
//----------------------------------------------------------------------------------------
// bind_to_library
//----------------------------------------------------------------------------------------

NMErr bind_to_library(
	FileDesc *file,
	ConnectionRef *conn_id)
{
	NMErr err= kNMNoError;

	*conn_id= LoadLibrary(file->name);
	/* mb_printf("LoadLibrary(%s)=> 0x%x!", file->name, *conn_id); */
	if(*conn_id==NULL)
	{
		err= GetLastError();
	}
	
	return err;	
}
	
//----------------------------------------------------------------------------------------
// load_proc
//----------------------------------------------------------------------------------------

void *load_proc(
	ConnectionRef conn_id,
	short proc_id)
{
	void *func;

	op_assert(conn_id);
 	op_assert(NUMBER_OF_MODULE_FUNCTIONS==NUMBER_OF_MODULES_TO_LOAD);
	op_assert(proc_id>=0 && proc_id<NUMBER_OF_MODULES_TO_LOAD);
	func= GetProcAddress(conn_id, module_names[proc_id]);
	/*mb_printf("GetProcAddress(%x, %s)=> %x!", conn_id, module_names[proc_id], func); */
	/*mb_print_last_error(); */

	return func;
}
	
//----------------------------------------------------------------------------------------
// free_library
//----------------------------------------------------------------------------------------

void free_library(
	ConnectionRef conn_id)
{
	op_assert(conn_id);
	FreeLibrary(conn_id);
	/*mb_printf("FreeLibrary(%x)", conn_id); */

	return;
}
