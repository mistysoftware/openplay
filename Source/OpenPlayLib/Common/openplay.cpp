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
 
//define our sections for doxygen to make start our documentation from here...

/** \addtogroup TypesAndConstants Types and Constants*/
/*\@{*/
/*\@}*/
/** \addtogroup Configuration Configuration Functions*/
/*\@{*/
/*\@}*/
/** \addtogroup EndpointManagement Endpoint Management Functions */
/*\@{*/
/*\@}*/
/** \addtogroup DataTransfer Data Transfer Functions*/
/*\@{*/
/*\@}*/
/** \addtogroup Enumeration Enumeration Functions */
/*\@{*/
/*\@}*/
/** \addtogroup HumanInterface Human Interface Functions */
/*\@{*/
/*\@}*/

/*! \mainpage OpenPlay "Protocol" API Documentation
 *
 * \section intro Introduction
 *
 * The OpenPlay library, in a nutshell, is a Net Module Manager. The Net Module Manager and the individual Net Modules provide various transports for OpenPlay clients. Any product that is written to the OpenPlay Protocol interface can work with any Net Module. To provide cross-platform connectivity, OpenPlay is available for Mac OS 9/X, Windows, and various flavors of Unix. 
 * The lowest level connection between an application and the Net Module Manager is the OpenPlay Protocol API. The connections between the Net Module Manager and the individual plug-ins represents the Net Module API (See the document "Writing OpenPlay Net Modules" for more info on the Net Module specification). 
 * Clients may choose to talk to the Net Module Manager directly via the Protocol interface, or through some other high-level abstraction API such as NetSprocket (also contained in the OpenPlay library). 
*
*This document is divided into the following sections:
*\li \ref TypesAndConstants
*\li \ref Configuration
*\li \ref EndpointManagement
*\li \ref DataTransfer
*\li \ref Enumeration
*\li \ref HumanInterface
*/


#include "portable_files.h"
 
#ifndef __OPENPLAY__
#include 			"OpenPlay.h"
#endif
#include "OPUtils.h"
#include "op_definitions.h"
#include "op_globals.h"
#include "module_management.h"

op_globals	gOp_globals;

/* ---------------- functions used on both platforms */

//----------------------------------------------------------------------------------------
// initialize_openplay
//----------------------------------------------------------------------------------------

void initialize_openplay( FileDesc *file )
{
	machine_mem_zero(&gOp_globals, sizeof(op_globals));

	if( file )
 		machine_copy_data( file, &gOp_globals.file_spec, sizeof(FileDesc) );

	gOp_globals.res_refnum = -1;
	
	op_idle_synchronous();
}


void shutdown_report(void)
{		
	#if (DEBUG)
	Endpoint *ep;
	loaded_modules_data *loaded_module;
	
		DEBUG_PRINT("*** OpenPlay Shutdown Report ****");
		
		loaded_module = gOp_globals.loaded_modules;
		while (loaded_module)
		{
			ep = loaded_module->first_loaded_endpoint;
			while (ep)
			{
				char *stateString;
				switch (ep->state){
					case _state_unknown: stateString = "_state_unknown"; break;
					case _state_closing: stateString = "_state_closing"; break;
					case _state_closed: stateString = "_state_closed"; break;
					default: stateString = "unknown state"; break;
				}
				DEBUG_PRINT("   endpoint exists at shutdown: %#10X; type:%c%c%c%c; active:%d parent: %#10X state:%s",
					ep,ep->type>>24,(ep->type>>16)&0xFF,(ep->type>>8)&0xFF,
					ep->type&0xFF,((ep->openFlags & kOpenActive) != 0),ep->parent,stateString);
				ep = ep->next;
			}
			loaded_module = loaded_module->next;
		}
		DEBUG_PRINT("*********************************");
	#endif //DEBUG
}
