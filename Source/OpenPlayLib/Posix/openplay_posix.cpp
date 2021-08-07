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

#include "portable_files.h"
#include "OPUtils.h"

#include "op_definitions.h"
#include "op_globals.h"
#include "module_management.h"

static void release_openplay(void);

extern "C"{

/* _init is called when the library is loaded */
void _init(void)
{
  initialize_openplay(NULL);
  refresh_protocols();
}
#pragma CALL_ON_LOAD _init

/* _fini is called when the library is unloaded */
void _fini(void)
{
  release_openplay();
}
#pragma CALL_ON_UNLOAD _fini

} //extern C

/* ---------------- functions used on both platforms */

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
				while(loaded_endpoint)
				{
					struct Endpoint *next_endpoint = loaded_endpoint->next;
				
					if(loaded_endpoint->state != _state_closing && loaded_endpoint->state != _state_closed)
					{
                                          /* dprintf("Trying to free endpoint at %x (this was abortive)", loaded_endpoint); */
						DEBUG_PRINT("calling ProtocolCloseEndpoint, %s:%d", __FILE__, __LINE__);
						ProtocolCloseEndpoint(loaded_endpoint, false);
					}
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
