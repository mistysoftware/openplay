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

#ifndef __OPENPLAY__
#include 			"OpenPlay.h"
#endif
#ifndef __NETMODULE__
#include 			"NetModule.h"
#endif
#include "OPUtils.h"
#include "portable_files.h"

#include "op_definitions.h"
#include "op_globals.h"
#include "module_management.h"
#include <sioux.h>

static void release_openplay(void);

extern "C"{
	OSErr __myinitialize(CFragInitBlockPtr ibp);
	void __myterminate(void);
	OSErr __initialize(CFragInitBlockPtr ibp);
	void __terminate(void);
}

NMSInt16	gResourceReference;


//----------------------------------------------------------------------------------------
// __myinitialize
//----------------------------------------------------------------------------------------

OSErr __myinitialize(
	CFragInitBlockPtr ibp)
{
	OSErr	err;

	err = __initialize(ibp);
	if (err != kNMNoError)
		return err;

	/* we should always be kDataForkCFragLocator when we are a shared library.
	 * assuming this is is the case, we want to open our resource file so we
	 * can access our resources if needed. our termination routine will close
	 * the file.
         */
	if (ibp->fragLocator.where == kDataForkCFragLocator)
	{
	  NMSInt16 cur_res_file= CurResFile(); /* don't affect the parent's chain.. */
		
		initialize_openplay((FileDesc *) ibp->fragLocator.u.onDisk.fileSpec);
		//refresh_protocols(); /* must be after the res_refnum is set */
	}
	else
	{
		return -1;
	}


	//set up sioux in case we're using it for debug output- they won't be able to move the window, so stick it somewhere
	//unique..
	SIOUXSettings.userwindowtitle = "\pOpenPlayLib debug output";
	SIOUXSettings.toppixel = 50;
	SIOUXSettings.leftpixel = 70;
	SIOUXSettings.setupmenus = false;
	SIOUXSettings.standalone = false;

	return err;
}

//----------------------------------------------------------------------------------------
// __myterminate
//----------------------------------------------------------------------------------------

void __myterminate(
	void)
{
  /* unsure if this is necessary... */
	release_openplay();
	__terminate();		/*  "You're Terminated." */
}


static void release_openplay(
	void)
{
	loaded_modules_data *loaded_module = gOp_globals.loaded_modules;

	shutdown_report();
	
	/* Release all the endpoints.. */
	while(loaded_module)
	{
		loaded_modules_data *next_module = loaded_module->next;
		Endpoint *loaded_endpoint;
		NMBoolean done = false;
		NMUInt32 initial_ticks = machine_tick_count();
		
		while(!done && machine_tick_count()-initial_ticks < 10*MACHINE_TICKS_PER_SECOND)
		{
			loaded_endpoint = loaded_module->first_loaded_endpoint;
			if(loaded_endpoint)
			{
				while(loaded_endpoint)
				{
					Endpoint *next_endpoint = loaded_endpoint->next;
				
					if(loaded_endpoint->state != _state_closing && loaded_endpoint->state != _state_closed)
					{
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
	
	/* Free the endpoint cache */
#ifdef OP_API_NETWORK_OT
	if(gOp_globals.cache_size > 0)
	{
		NMSInt16 index;
		
		for(index = 0; index < gOp_globals.cache_size; ++index)
		{
			dispose_pointer(gOp_globals.endpoint_cache[index]);
			gOp_globals.endpoint_cache[index] = NULL;
		}
		gOp_globals.cache_size = 0;
	}
#endif
	
	/* Free the available modules list */
	if(gOp_globals.module_count)
	{
		gOp_globals.module_count = 0;
		dispose_pointer(gOp_globals.modules);
		gOp_globals.modules = NULL;
	}
}

//----------------------------------------------------------------------------------------
// op_idle_synchronous
//----------------------------------------------------------------------------------------

/* call this whenever you can (synchronously) */
void op_idle_synchronous(void)
{
	NMSInt16 index;

	while(gOp_globals.cache_size < MAXIMUM_CACHED_ENDPOINTS)
	{
		gOp_globals.endpoint_cache[gOp_globals.cache_size] = (Endpoint*)new_pointer(sizeof(Endpoint));
		if(gOp_globals.endpoint_cache[gOp_globals.cache_size])
		{
			machine_mem_zero(gOp_globals.endpoint_cache[gOp_globals.cache_size],sizeof(Endpoint));
			gOp_globals.cache_size += 1;
		} else {
			break; /* silently bail. */
		}
	}
	
	for(index = 0; index < gOp_globals.doomed_endpoint_count; ++index)
	{
		op_assert(gOp_globals.doomed_endpoints[index]);
		dispose_pointer(gOp_globals.doomed_endpoints[index]);
		gOp_globals.doomed_endpoints[index] = NULL;
	}
	gOp_globals.doomed_endpoint_count = 0;
}
