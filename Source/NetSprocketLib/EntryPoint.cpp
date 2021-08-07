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

//#include "platform.h"

#ifndef __OPENPLAY__
#include 			"OpenPlay.h"
#endif
#include "OPUtils.h"
#ifndef __NETMODULE__
#include 			"NetModule.h"
#endif

#if defined(OP_API_PLUGIN_WINDOWS)

	#ifdef __cplusplus
	extern "C" {
	#endif


	//----------------------------------------------------------------------------------------
	// NetSprocketEntryPoint
	//----------------------------------------------------------------------------------------

	BOOL WINAPI NetSprocketEntryPoint(HINSTANCE hInst, DWORD fdwReason, LPVOID lpvReserved);

	#ifdef __cplusplus
	}
	#endif

	BOOL WINAPI NetSprocketEntryPoint(
		HINSTANCE hInst,
		DWORD fdwReason,
		LPVOID lpvReserved)
	{
	UNUSED_PARAMETER(hInst)
	UNUSED_PARAMETER(lpvReserved)

	  NMBoolean      success = false; /* ignored on everything but process attach */
	//  FileDesc  file;
		
	  switch(fdwReason)
	  {
	    case DLL_PROCESS_ATTACH:
	      /* we just got initialized.  Note we can use the TlsAlloc to alloc thread local storage
	       * lpvReserved is NULL for dynamic loads, non-Null for static loads
	       */
	//      GetModuleFileName(hInst, file.name, sizeof(file.name));
	      success = true;
	    break;
				
	    case DLL_THREAD_ATTACH:
	      /* called when a process creates another thread, so we can bind to it if we want.. */
	    break;
				
	    case DLL_THREAD_DETACH:
	      /* note this isn't necessary balanced by DLL_THREAD_ATTACH (ie first thread, or 
	       *  if thread was already running before loading the dll)
	       */
	    break;
				
	    case DLL_PROCESS_DETACH:
	      /* Process exited, or FreeLibrary...
	       * lpvReserved is NULL if called by FreeLibrary and non-NULL if called by process termination
	       */
	    break;
		}

	  /*  DEBUG_PRINT("Returning: %d", success); */
		
	  return success;
	}

#endif // windows_build



