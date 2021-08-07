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

#include "portable_files.h"

#ifndef __OPENPLAY__
#include "OpenPlay.h"
#endif
#include "OPUtils.h"

#include "NetModule.h"
#include "NetModulePrivate.h"

#include "op_definitions.h"
#include "op_globals.h"
#include "OPDLLUtils.h"
#include "module_management.h"

/* ----------- local prototypes  */
static void remove_loaded_module(loaded_modules_data *loaded_module);

/* ---------- code */

//----------------------------------------------------------------------------------------
// bind_to_protocol
//----------------------------------------------------------------------------------------

NMErr bind_to_protocol(
	NMType type,
	FileDesc *spec,
	ConnectionRef *connection)
{
	loaded_modules_data *loaded_module;
	NMErr err= kNMNoError;
	NMBoolean found= false;

	loaded_module= gOp_globals.loaded_modules;
	while(!found && loaded_module)
	{
		if(loaded_module->type==type)
		{
			*connection= loaded_module->connection;
			loaded_module->connection_binded_count+= 1;
			found= true;
		}
		loaded_module= loaded_module->next;
	}

	/* Must bind to it.. */
	if(!found)
	{
		ConnectionRef new_connection;
	
		err= bind_to_library(spec, &new_connection);
		if(!err)
		{
			loaded_module= (loaded_modules_data *) new_pointer(sizeof(loaded_modules_data));
			if(loaded_module)
			{
				/* clear! */
				machine_mem_zero(loaded_module, sizeof(loaded_modules_data));
				loaded_module->type= type;
				loaded_module->connection= new_connection;
				loaded_module->connection_binded_count= 1;
				loaded_module->first_loaded_endpoint= NULL;

				/* attach.. */
				loaded_module->next= gOp_globals.loaded_modules;
				gOp_globals.loaded_modules= loaded_module;
				/* DEBUG_PRINT("INITIAL BIND"); */
				/* and return the right thing.. */
				*connection= loaded_module->connection;
			} else {
				free_library(new_connection);
				err= kNMOutOfMemoryErr;
			}
		}
	}
	
	return err;
}

//----------------------------------------------------------------------------------------
// unbind_from_protocol
//----------------------------------------------------------------------------------------

void unbind_from_protocol(
	NMType type,
	NMBoolean synchronous)
{
	loaded_modules_data *loaded_module;
	NMBoolean found= false;

	loaded_module= gOp_globals.loaded_modules;
	while(!found && loaded_module)
	{
		loaded_modules_data *next_module= loaded_module->next;
	
		if(loaded_module->type==type)
		{
			op_assert(loaded_module->connection_binded_count>=1);
			loaded_module->connection_binded_count-= 1;
			/* DEBUG_PRINT("UNBIND FROM PROTOCOL: New Count: %d", loaded_module->connection_binded_count);	*/
			if(synchronous && loaded_module->connection_binded_count==0)
			{
				remove_loaded_module(loaded_module);
			}
			found= true;
		}
		
		/* because we may have freed it.. */
		loaded_module= next_module;
	}
	op_assert(found);
}

//----------------------------------------------------------------------------------------
// module_management_idle_time
//----------------------------------------------------------------------------------------

void module_management_idle_time(
	void)
{
	loaded_modules_data *loaded_module= gOp_globals.loaded_modules;

	/* look for things with a 0 connection count and unbind them... */
	while(loaded_module)
	{
		loaded_modules_data *next_module= loaded_module->next;

		if(loaded_module->connection_binded_count==0)
		{
			remove_loaded_module(loaded_module);
		}
		loaded_module= next_module;
	}
}

//----------------------------------------------------------------------------------------
// add_endpoint_to_loaded_list
//----------------------------------------------------------------------------------------

void add_endpoint_to_loaded_list(
	Endpoint *endpoint)
{
	loaded_modules_data *loaded_module;
	NMBoolean found= false;

	loaded_module= gOp_globals.loaded_modules;
	while(!found && loaded_module)
	{
		if(loaded_module->type==endpoint->type)
		{
			/* add it to the head of the list.. */
			endpoint->next= loaded_module->first_loaded_endpoint;
			loaded_module->first_loaded_endpoint= endpoint;
			loaded_module->endpoints_instantiated+= 1;
			found= true;
			/*DEBUG_PRINT("ADDING endpoint to loaded list: New Count:  %d", loaded_module->endpoints_instantiated);	*/
		}
		loaded_module= loaded_module->next;
	}
	op_assert(found);
}

//----------------------------------------------------------------------------------------
// valid_endpoint
//----------------------------------------------------------------------------------------

NMBoolean valid_endpoint(
	Endpoint *endpoint)
{
	NMBoolean valid= false;
	
	if(endpoint)
	{
		loaded_modules_data *loaded_module;

		loaded_module= gOp_globals.loaded_modules;
		while(!valid && loaded_module)
		{
			if(loaded_module->type==endpoint->type)
			{
				Endpoint *ep;
				
				ep= loaded_module->first_loaded_endpoint;
				while(ep && ep!=endpoint) ep= ep->next;
				
				if(ep)
				{
					valid= true;
				}
			}
			loaded_module= loaded_module->next;
		}
	}
	
	return valid;
}

//----------------------------------------------------------------------------------------
// remove_endpoint_from_loaded_list
//----------------------------------------------------------------------------------------

void remove_endpoint_from_loaded_list(
	Endpoint *endpoint)
{
	loaded_modules_data *loaded_module;
	NMBoolean found= false;
	
	loaded_module= gOp_globals.loaded_modules;
	while(!found && loaded_module)
	{
		if(loaded_module->type==endpoint->type)
		{
			/* remove from the list */
			if(endpoint==loaded_module->first_loaded_endpoint)
			{
				loaded_module->first_loaded_endpoint= endpoint->next;
			} else {
				Endpoint *previous_endpoint;
				
				previous_endpoint= loaded_module->first_loaded_endpoint;
				while(previous_endpoint && previous_endpoint->next != endpoint)
				{
					previous_endpoint= previous_endpoint->next;
				}
				op_assert(previous_endpoint);
				previous_endpoint->next= endpoint->next;
			}
		
			op_assert(loaded_module->endpoints_instantiated>=1);

			loaded_module->endpoints_instantiated-= 1;
			/* DEBUG_PRINT("REMOVE endpoint from loaded list: New Count: %d", loaded_module->endpoints_instantiated); */	
			found= true;
		}
		loaded_module= loaded_module->next;
	}
	op_assert(found);
}

//----------------------------------------------------------------------------------------
// count_endpoints_of_type
//----------------------------------------------------------------------------------------

NMSInt32 count_endpoints_of_type(
	NMType type)
{
	loaded_modules_data *loaded_module;
	NMBoolean found= false;
	NMSInt32 count= 0;
	
	loaded_module= gOp_globals.loaded_modules;
	while(!found && loaded_module)
	{
		if(loaded_module->type==type)
		{
#if DEBUG
			Endpoint *ep;
			NMSInt32 actual_count= 0;
			char temp[100];
			
			ep= loaded_module->first_loaded_endpoint;
			while(ep) ep= ep->next, actual_count+= 1;
			op_vassert(actual_count==loaded_module->endpoints_instantiated,
				csprintf(temp, "actual count: %d != %d", actual_count, loaded_module->endpoints_instantiated));
#endif

			/* add it to the head of the list.. */
			count= loaded_module->endpoints_instantiated;
			found= true;
		}
		loaded_module= loaded_module->next;
	}
	
	return count;
}

/* ----------- local code */

//----------------------------------------------------------------------------------------
// remove_loaded_module
//----------------------------------------------------------------------------------------

static void remove_loaded_module(
	loaded_modules_data *loaded_module)
{
	/* %% LR -- at times this routine fails calling free() on a non-malloc'd block! */
	op_assert(loaded_module != NULL);

	/* remove it from the list.. */
	if(loaded_module == gOp_globals.loaded_modules)
	{
		gOp_globals.loaded_modules = loaded_module->next;
	}
	else	/* if more than one module, adjust the linked list */
	{
		loaded_modules_data *previous_module = gOp_globals.loaded_modules;
		
		while(previous_module && previous_module->next != loaded_module)
			previous_module = previous_module->next;

		op_assert(previous_module);
		previous_module->next = loaded_module->next;
	}

	/* free the library. */
	op_assert(loaded_module->connection_binded_count==0);
	op_assert(loaded_module->first_loaded_endpoint==NULL);

	/* Unfortunately, we are getting called from within a callback from the
         * NetModule on the pc.  Therefore, we can't unload the module.  
         * If we did, we would royally mess up our stack.
         */
	/* free_library(loaded_module->connection); */

	/* and free the module. */
	dispose_pointer(loaded_module);
}
