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


#ifndef __NETMODULE__
#include 			"NetModule.h"
#endif

#include "portable_files.h"
#include "OPUtils.h"

#define INCLUDE_MODULE_NAMES

#include "OpenPlay.h"
#include "op_definitions.h"
#include "op_globals.h"
#include "op_resources.h"
#include "OPDLLUtils.h"

/* We must bind! */
/*----------------------------------posix Section-----------------------------*/

#ifdef OP_API_PLUGIN_MACHO
	#include "op_dlfcn.h"
	#define dlopen op_dlopen
	#define dlsym op_dlsym
	#define dlerror op_dlerror
	#define dlclose op_dlclose
#else
	#include <dlfcn.h>
#endif

extern "C" {

/* Returns 0 on success, !0 otherwise */ 
NMErr bind_to_library(FileDesc *file, ConnectionRef *conn_id)
{
#ifdef OP_API_PLUGIN_MACHO
		Boolean didLoad = false;

		op_assert(file->bundle != NULL);
		didLoad = CFBundleLoadExecutable(file->bundle);
	if (didLoad) {
			void (*initFunc) (void);
			//search for an "_init" function, and call it if we find one
			//so we can just use the standard posix configuration
			initFunc = (void (*) (void))CFBundleGetFunctionPointerForName(file->bundle,CFSTR("_init"));
			if (initFunc)
				initFunc();
			*conn_id = file->bundle;
			return kNMNoError;
	}
	else
		{
			DEBUG_PRINT("couldn't load executable");		
			return 1;
		}
	#else
		void* lib_handle;

		lib_handle = dlopen((char*)file->name, RTLD_NOW);

		DEBUG_PRINT("dlopen(%s) = %p\n", file->name, lib_handle);

		if (lib_handle == NULL)
		{
		#ifdef DEBUG
			printf("%s\n", dlerror());
		#endif
			return(1);
		} 
		*conn_id = (ConnectionRef)lib_handle;

		return(0);
	#endif
}

void *load_proc(ConnectionRef conn_id, short proc_id)
{
  	void* proc_ptr = NULL;

	op_assert(conn_id);
 	op_assert(NUMBER_OF_MODULE_FUNCTIONS==NUMBER_OF_MODULES_TO_LOAD);
	op_assert(proc_id>=0 && proc_id<NUMBER_OF_MODULES_TO_LOAD);

	if ((proc_id >= 0) && (proc_id < NUMBER_OF_MODULES_TO_LOAD))
	{
	#ifdef OP_API_PLUGIN_MACHO

			CFStringRef funcName = CFStringCreateWithCString(kCFAllocatorDefault,module_names[proc_id],kCFStringEncodingMacRoman);

			op_assert(funcName);
			proc_ptr = (void*)CFBundleGetFunctionPointerForName(conn_id,funcName);
			CFRelease(funcName);

	#else

		//	We seem to need to prefix an underscore to get them on darwin
		//	I think updating the dlcompat code might negate this requirement.... ecf

		#ifdef OP_API_PLUGIN_POSIX_DARWIN
				char symbolName[128];

				sprintf(symbolName,"_%s",module_names[proc_id]);
			#else
				char *symbolName = (char*)module_names[proc_id];
			#endif

			proc_ptr = dlsym((void *)conn_id, symbolName);
		#endif
	}
	  
	return (proc_ptr);
}


void free_library(ConnectionRef conn_id)
{
#if (project_builder)
	void (*termFunc) (void);
	//search for a "_fini" function, and call it if we find one
	//so we can just use the standard posix configuration
	termFunc = (void (*) (void))CFBundleGetFunctionPointerForName(conn_id,CFSTR("_fini"));
	if (termFunc)
		termFunc();
//%% LR causes lockup!	CFBundleUnloadExecutable(conn_id);
#else
	dlclose((void *)conn_id);
#endif
}
}