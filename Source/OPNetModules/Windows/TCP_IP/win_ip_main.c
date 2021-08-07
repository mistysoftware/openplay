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

#ifndef __OPENPLAY__
#include "OpenPlay.h"
#endif
#include "NetModule.h"
#include "win_ip_module.h"

//	------------------------------	Private Definitions
//	------------------------------	Private Types
//	------------------------------	Private Variables

static BOOL	register_callback_window(HINSTANCE hInstance);
static BOOL	unregister_callback_window(void);

//	------------------------------	Private Functions

BOOL WINAPI DllMain(HINSTANCE hInst, DWORD fdwReason, LPVOID lpvReserved);

//	------------------------------	Public Variables

/* sjb 19990317 this needs to be shared per library/process, not per thread */
module_data gModuleGlobals;
LPCSTR	szNetCallbackWndClass = TEXT("NetCallbackWndClass");

//	--------------------	NetModuleEntryPoint


//----------------------------------------------------------------------------------------
// DllMain
//----------------------------------------------------------------------------------------

BOOL WINAPI
DllMain(
	HINSTANCE	hInst,
	DWORD		fdwReason,
	LPVOID		lpvReserved)
{
	BOOL     success = false;			// ignored on everything but process attach

	UNUSED_PARAMETER(hInst)
	UNUSED_PARAMETER(lpvReserved)

	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH:
			// we just got initialized.  Note we can use the TlsAlloc to alloc thread local storage
			// lpvReserved is NULL for dynamic loads, non-Null for static loads
		
			// Register the fucking window class.
			if (register_callback_window(hInst)) {

				gModuleGlobals.application_instance= hInst;
			}
			else {

				DEBUG_PRINT("Unable to register the network window class!");
			}
			success = true;
			break;
			
		case DLL_THREAD_ATTACH:
			// called when a process creates another thread, so we can bind to it if we want..
			break;
			
		case DLL_THREAD_DETACH:
			// note this isn't necessary balanced by DLL_THREAD_ATTACH (ie first thread, or 
			//  if thread was already running before loading the dll)
			break;
			
		case DLL_PROCESS_DETACH:
			// Process exited, or FreeLibrary...
			// lpvReserved is NULL if called by FreeLibrary and non-NULL if called by process termination
			unregister_callback_window();
			gModuleGlobals.application_instance= NULL;
			break;
	}
	
	return success;
}

//----------------------------------------------------------------------------------------
// register_callback_window
//----------------------------------------------------------------------------------------

static BOOL
register_callback_window(HINSTANCE hInstance)
{
WNDCLASS	wc;


	machine_mem_zero(&wc, sizeof (wc));
	wc.style = 0;
	wc.lpfnWndProc = (WNDPROC)NetCallbackWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = sizeof (NMEndpointRef);
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance, NULL) ;
	wc.hCursor = LoadCursor(NULL, NULL);
	wc.hbrBackground = GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = szNetCallbackWndClass;

	return (RegisterClass(&wc));
}

//----------------------------------------------------------------------------------------
// unregister_callback_window
//----------------------------------------------------------------------------------------

static BOOL
unregister_callback_window(void)
{	

	return UnregisterClass(szNetCallbackWndClass, gModuleGlobals.application_instance);
}
