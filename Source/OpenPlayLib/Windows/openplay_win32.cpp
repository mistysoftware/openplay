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

#include "op_definitions.h"
#include "op_globals.h"
#include "module_management.h"

static void release_openplay(void);

/* export everything.. */
#ifdef __WATCOMC__ 
	#pragma aux OpenPlayEntryPoint export
	#pragma aux GetIndexedProtocol export
	#pragma aux ProtocolCreateConfig export
	#pragma aux ProtocolDisposeConfig export 
	#pragma aux ProtocolGetConfigString export
	#pragma aux ProtocolGetConfigStringLen export
	#pragma aux ProtocolSetupDialog export
	#pragma aux ProtocolHandleEvent export
	#pragma aux ProtocolGetRequiredDialogFrame export
	#pragma aux ProtocolDialogTeardown export
	#pragma aux ProtocolStartEnumeration export
	#pragma aux ProtocolIdleEnumeration export
	#pragma aux ProtocolEndEnumeration export
	#pragma aux ProtocolBindEnumerationToConfig export
	#pragma aux ProtocolOpenEndpoint export
	#pragma aux ProtocolCloseEndpoint export
	#pragma aux ProtocolSetTimeout export
	#pragma aux ProtocolIsAlive export
	#pragma aux ProtocolIdle export
	#pragma aux ProtocolFunctionPassThrough export
	#pragma aux ProtocolSendPacket export
	#pragma aux ProtocolReceivePacket export
	#pragma aux ProtocolAcceptConnection export
	#pragma aux ProtocolRejectConnection export
	#pragma aux ProtocolSend export
	#pragma aux ProtocolReceive export
	#pragma aux ProtocolGetEndpointInfo export
	#pragma aux ValidateCrossPlatformPacket export
	#pragma aux SwapCrossPlatformPacket export
	#pragma aux ProtocolConfigPassThrough export
#endif


extern "C"{
	BOOL WINAPI OpenPlayEntryPoint(HINSTANCE hInst, DWORD fdwReason, LPVOID lpvReserved);

//ECF 011103 codewarrior is giving me problems if the dll entrypoint is not "default"
//so we define the default name here
BOOL WINAPI DllMain(HINSTANCE hInst, DWORD fdwReason, LPVOID lpvReserved);
BOOL WINAPI DllMain(HINSTANCE hInst, DWORD fdwReason, LPVOID lpvReserved)
{
	return OpenPlayEntryPoint(hInst,fdwReason,lpvReserved);
}

}
	
//----------------------------------------------------------------------------------------
// OpenPlayEntryPoint
//----------------------------------------------------------------------------------------

BOOL WINAPI OpenPlayEntryPoint(
	HINSTANCE hInst,
	DWORD fdwReason,
	LPVOID lpvReserved)
{
  BOOL      success = false; /* ignored on everything but process attach */
  FileDesc  file;

  UNUSED_PARAMETER(lpvReserved)

  switch(fdwReason)
  {
    case DLL_PROCESS_ATTACH:
      /* we just got initialized.  Note we can use the TlsAlloc to alloc thread local storage
       * lpvReserved is NULL for dynamic loads, non-Null for static loads
       */
      GetModuleFileName(hInst, file.name, sizeof(file.name));
      initialize_openplay(&file);
      gOp_globals.dll_instance = hInst;
      refresh_protocols(); /* must be after the hInst is set */
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
      release_openplay();
    break;
	}

  /*  mb_printf("Returning: %d", success); */
	
  return success;
}


static void release_openplay(
	void)
{
	struct loaded_modules_data *loaded_module = gOp_globals.loaded_modules;

	shutdown_report();
	
	/* Release all the endpoints.. */
	while(loaded_module)
	{
		struct loaded_modules_data *next_module = loaded_module->next;
		struct Endpoint *loaded_endpoint;
		NMBoolean done = false;
		long initial_ticks = machine_tick_count();
		
		while(!done && machine_tick_count()-initial_ticks < 10*MACHINE_TICKS_PER_SECOND)
		{
			loaded_endpoint = loaded_module->first_loaded_endpoint;
			if(loaded_endpoint)
			{
				done = true; //start optimistic
				while(loaded_endpoint)
				{
					struct Endpoint *next_endpoint = loaded_endpoint->next;
				
					if(loaded_endpoint->state != _state_closing && loaded_endpoint->state != _state_closed)
					{
						NMErr err;
                                          /* dprintf("Trying to free endpoint at %x (this was abortive)", loaded_endpoint); */
						DEBUG_PRINT("calling ProtocolCloseEndpoint, %s:%d", __FILE__, __LINE__);
						err = ProtocolCloseEndpoint(loaded_endpoint, false);
						//if we closed ok, we wanna keep waiting
						if (!err)
							done = false;
					}
					else
						done = false; //its probably closing
					loaded_endpoint = next_endpoint;
				}
			} else {
				/* this module is clean. */
				done = true;
			}
		}

		op_vwarn(done, "WARNING: release_openplay was unable to close all netmodules!!");
		
		/* Free the library */
		loaded_module = next_module; /* cached so we don't free too early.. */
	}

	/* Remove all the modules with zero counts (which should be all of them) */
	module_management_idle_time();
	
	/* Free the available modules list */
	if(gOp_globals.module_count)
	{
		gOp_globals.module_count = 0;
		dispose_pointer(gOp_globals.modules);
		gOp_globals.modules = NULL;
	}
	
	return;
}

/* call this whenever you can (synchronously) */
void op_idle_synchronous(void)
{
	return;
}
