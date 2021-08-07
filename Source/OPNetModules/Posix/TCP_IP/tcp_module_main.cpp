/* 
 *-------------------------------------------------------------
 * Description:
 *   Functions which are main entry points for module library.
 *
 *------------------------------------------------------------- 
 *   Author: Kevin Holbrook
 *  Created: June 23, 1999
 *
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


#include "NetModule.h"
#include "NetModulePrivate.h"
#include "tcp_module.h"
#include "OPUtils.h"
#include <stdio.h>

#ifdef OP_API_NETWORK_SOCKETS
	#include <pthread.h>
#endif

//  ------------------------------  Private Prototypes
extern "C"{
void _init(void);
void _fini(void);

#ifdef OP_API_NETWORK_WINSOCK
		BOOL WINAPI DllMain(HINSTANCE hInst, DWORD fdwReason, LPVOID lpvReserved);
#endif
}

//	------------------------------ Variables
static NMModuleInfo		gModuleInfo;
static const NMUInt32 moduleID = 'Inet';
static const char *kModuleName = "TCP/IP";
static const char *kModuleCopyright = "1996-2004 Apple Computer, Inc.";

NMEndpointPriv *endpointList = NULL;
NMUInt32 endpointListState = 0;
machine_lock *endpointListLock; //dont access the list without locking it!
machine_lock *endpointWaitingListLock;
machine_lock *notifierLock; //dont call the user back without locking it!

NMSInt32 module_inited = 0;

#ifdef OP_API_NETWORK_WINSOCK
	HINSTANCE application_instance;
#endif

extern "C"{

/* 
 * Function: _ini
 *--------------------------------------------------------------------
 * Parameters:
 *   none
 *
 * Returns:
 *   none
 *
 * Description:
 *   Function called when shared library is first loaded by system.
 *
 *--------------------------------------------------------------------
 */


void _init(void)
{
	DEBUG_ENTRY_EXIT("_init");
		
	// ecf - on the OSX bundle based builds, we don't seem to be re-initialized
	// so we have to share globals and _init may be called multiple times on the same
	// instance of our module - we must be prepared.
	module_inited++;
	if (module_inited != 1)
		return;
	//op_assert(module_inited == false);

	gModuleInfo.size= sizeof (NMModuleInfo);
	gModuleInfo.type = moduleID;
	strcpy(gModuleInfo.name, kModuleName);
	strcpy(gModuleInfo.copyright, kModuleCopyright);
	gModuleInfo.maxPacketSize = 1500;		// <ECF> is this right? i just copied from the mac one...
	gModuleInfo.maxEndpoints = kNMNoEndpointLimit;
	gModuleInfo.flags= kNMModuleHasStream | kNMModuleHasDatagram | kNMModuleHasExpedited;

	//create the lock for our main list
	endpointListLock = new machine_lock;
	endpointWaitingListLock = new machine_lock;
	notifierLock = new machine_lock;	

#ifdef OP_API_NETWORK_WINSOCK
	initWinsock();	//LR -- can not use sockets before they are started up!
#endif

	if (createWakeSocket())
		DEBUG_PRINT("createWakeSocket succeeded");
	else
#ifdef OP_API_NETWORK_WINSOCK
		DEBUG_PRINT("createWakeSocket failed, err = %d", WSAGetLastError() );
#else
		DEBUG_PRINT("createWakeSocket failed");
#endif
	
} /* _init */


/* 
 * Function: _fini
 *--------------------------------------------------------------------
 * Parameters:
 *   none
 *
 * Returns:
 *   none
 *
 * Description:
 *   Function called when shared library is unloaded by system.
 *
 *--------------------------------------------------------------------
 */

void _fini(void)
{
	DEBUG_ENTRY_EXIT("_fini");

	module_inited--;
	op_assert(module_inited >= 0);
	if (module_inited != 0)
		return;
			
	//op_assert(module_inited == true);

	//if we have a worker thread, kill it
	#if USE_WORKER_THREAD
		killWorkerThread();
	#endif
	
	//on windows, kill winsock
	#ifdef OP_API_NETWORK_WINSOCK
		shutdownWinsock();
	#endif
	
	delete endpointListLock;
	delete endpointWaitingListLock;
	delete notifierLock;
	
	disposeWakeSocket();

#ifdef OP_API_NETWORK_WINSOCK
	WSACleanup();
#endif
} /* _fini */


/* 
 *	Function: DllMain
	called when windows dll is loaded/unloaded.  Simply hooks to _init/_fini
 */
#ifdef OP_API_NETWORK_WINSOCK

BOOL WINAPI
DllMain(
	HINSTANCE	hInst,
	DWORD		fdwReason,
	LPVOID		lpvReserved)
{
	BOOL     success = true;			// ignored on everything but process attach

	UNUSED_PARAMETER(hInst)
	UNUSED_PARAMETER(lpvReserved)

	DEBUG_ENTRY_EXIT("DllMain");
	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH:
			// we just got initialized.  Note we can use the TlsAlloc to alloc thread local storage
			// lpvReserved is NULL for dynamic loads, non-Null for static loads
			application_instance = hInst;
			_init();
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
			_fini();
			break;
	}
	
	return success;
}
#endif

/* 
 * Function: NMGetModuleInfo
 *--------------------------------------------------------------------
 * Parameters:
 *   [IN/OUT] module_info = ptr to module information structure to
 *                          be filled in.
 *
 * Returns:
 *   Network module error code
 *     kNMNoError            = succesfully got module information
 *     kNMParameterError     = module_info was not a valid pointer
 *     kNMModuleInfoTooSmall = size of passed structure was wrong
 *
 * Description:
 *   Function to get network module information.
 *
 *--------------------------------------------------------------------
 */

NMErr NMGetModuleInfo(NMModuleInfo *module_info)
{
	DEBUG_ENTRY_EXIT("NMGetModuleInfo");
	if (module_inited < 1){
		op_warn("NMGetModuleInfo called when module not inited");
		return kNMInternalErr;
	}
	
  /* validate pointer */
  if (!module_info)
    return(kNMParameterErr);

  /* validate size of structure passed to us */
  if (module_info->size >= sizeof(NMModuleInfo))
  {
	short	size_to_copy = (module_info->size<gModuleInfo.size) ? module_info->size : gModuleInfo.size;
	
		machine_move_data(&gModuleInfo, module_info, size_to_copy);
  
    return(kNMNoError);
  }
  else
  {
    return(kNMModuleInfoTooSmall);
  }

} /* NMGetModuleInfo */


} //extern C