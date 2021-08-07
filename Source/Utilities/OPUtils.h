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

#ifndef __OPUTILS__
#define __OPUTILS__

//	------------------------------	Includes

	#ifndef __OPENPLAY__
	#include 			"OpenPlay.h"
	#endif

	#include <stdio.h>
	#include <stddef.h>
	#include <stdlib.h>
	#include <string.h>
	
	
//	------------------------------	Public Definitions

	#ifndef true
		#define true	(1)
	#endif
	#ifndef false
		#define false	(0)
	#endif

	#define NONE		(-1)

// byteswapping
	#define SWAP2(q) ((((NMUInt16)(q))>>8) | ((((NMUInt16)(q))<<8)&0xff00))

	#define SWAP4(q) (((((NMUInt32) (q)))>>24) | ((((NMUInt32) (q))>>8)&0xff00) | ((((NMUInt32) (q))<<8)&0xff0000) | ((((NMUInt32) (q))<<24)&0xff000000))


	#define kVariableLengthArray	1


	#define MILLISECS_PER_SEC		1000

	#ifdef OP_PLATFORM_MAC_CFM

	  #define MACHINE_TICKS_PER_SECOND  (60)

	#elif defined(OP_PLATFORM_WINDOWS)

	  #define MACHINE_TICKS_PER_SECOND  (1000)

	#elif defined(OP_PLATFORM_UNIX) || defined(OP_PLATFORM_MAC_MACHO)
	  
	  #include <time.h>
	  #define MACHINE_TICKS_PER_SECOND  (1000) //we're using millisecs here now

	#else
	  #error "Porting error - definition needed for MACHINE_TICKS_PER_SECOND"
	#endif

//	------------------------------	Public Types 

	enum
	{
	  KILO = 1024,
	  MEG  = (KILO * KILO)
	};

//	------------------------------	Public Variables
#ifdef OP_PLATFORM_MAC_CFM
//call checkMacOSVersion somewhere in your code 
//before you use this (at non-interrupt time)

extern NMBoolean gRunningOSX; 

#endif

//	------------------------------	Public Functions

#if (OP_PLATFORM_MAC_CFM)

	void checkMacOSVersion();

#endif

#ifdef __cplusplus
extern "C" {
#endif

	// ------- prototypes for machine specific stuff


//====================================================================================================================
// debug aids....
//====================================================================================================================



	#if DEBUG
	
		extern char sz_temporary[1024];

		extern void _assertion_failure(char *assertion, char *file, NMUInt32 line, NMBoolean fatal);

		#define op_halt() _assertion_failure((char *)NULL, __FILE__, __LINE__, true)
		#define op_vhalt(diag) _assertion_failure(diag, __FILE__, __LINE__, true)
		#define op_assert(expr) if (!(expr)) _assertion_failure(#expr, __FILE__, __LINE__, true)
		#define op_vassert(expr,diag) if (!(expr)) _assertion_failure(diag, __FILE__, __LINE__, true)
		#define op_warn(expr) if (!(expr)) _assertion_failure(#expr, __FILE__, __LINE__, false)
		#define op_vwarn(expr,diag) if (!(expr)) _assertion_failure(diag, __FILE__, __LINE__, false)
		#define op_pause() _assertion_failure((char *)NULL, __FILE__, __LINE__, false)
		#define op_vpause(diag) _assertion_failure(diag, __FILE__, __LINE__, false)
		#define op_vassert_return(expr,diag,err) if (!(expr)) { _assertion_failure(diag, __FILE__, __LINE__, true); return err; }
		#define op_vassert_justreturn(expr,diag) if (!(expr)) { _assertion_failure(diag, __FILE__, __LINE__, true); return; }

		extern	NMSInt32						op_dprintf(const char *format, ...);
		#define DEBUG_PRINT						op_dprintf
		#define DEBUG_PRINTonERR(format,err)	if(err) op_dprintf(format,err);
		
		#ifdef OP_PLATFORM_MAC_CFM

			void 	dprintf_oterr		(EndpointRef endpoint, char *message, NMErr err, char *file, NMSInt32 line);
			#define DEBUG_NETWORK_API(endpoint, message, err)	if (err != kNMNoError)	dprintf_oterr(endpoint, message, err, __FILE__, __LINE__)

		#else

			NMErr	dprintf_err			( char	*message, NMErr	err, char *file,NMSInt32	line);
			#define DEBUG_NETWORK_API(message, err)	if (err != kNMNoError) { dprintf_err(message, err, __FILE__, __LINE__);}

		#endif

	#else

		#define op_halt()
		#define op_vhalt(diag)
		#define op_assert(expr)
		#define op_vassert(expr,diag)
		#define op_warn(expr)
		#define op_vwarn(expr,diag)
		#define op_pause()
		#define op_vpause(diag)
		#define op_vassert_return(expr,diag,err) if (!(expr)) {  return err; }
		#define op_vassert_justreturn(expr,diag) if (!(expr)) {  return; }

		#define DEBUG_PRINT
		#define DEBUG_PRINTonERR(format,err)

		#ifdef OP_PLATFORM_MAC_CFM
			#define DEBUG_NETWORK_API(endpoint, message, err)
		#else
			#define DEBUG_NETWORK_API( message,err)
		#endif
			
	#endif // DEBUG=false


//====================================================================================================================
// Memory copy/ move functions....
//====================================================================================================================

	#ifdef OP_PLATFORM_MAC_CFM

		#define machine_move_data(srcPtr, destPtr, dataLength)	BlockMoveData(srcPtr, destPtr, dataLength);
		#define machine_copy_data(srcPtr, destPtr, dataLength)	BlockMoveData(srcPtr, destPtr, dataLength);
		#define machine_mem_zero(startLoc, zeroLength) 			OTMemzero(startLoc, zeroLength);

	#else

		#define machine_move_data(srcPtr, destPtr, dataLength)	memmove(destPtr, srcPtr, dataLength);
		#define machine_copy_data(srcPtr, destPtr, dataLength)	memcpy(destPtr, srcPtr, dataLength);
		#define machine_mem_zero(startLoc, zeroLength)			memset(startLoc, 0, zeroLength)
	
	#endif //OP_PLATFORM_MAC_CFM

//====================================================================================================================
// Alloc/Free functions....
//====================================================================================================================

	#ifdef OP_PLATFORM_MAC_CFM
		
			#define  new_pointer(size)      NewPtr(size)
			#define  dispose_pointer(x)  	if (x != NULL) {DisposePtr((char*)x);}
	
			/////////////////////////Quinn's memory allocation functions//////////////////////////
			extern NMErr InitOTMemoryReserve(ByteCount freeHeapSpaceRequired, ByteCount chunkSize, 
											ItemCount minChunks, ItemCount maxChunks);
			// Initialises the module.  Creates an OT memory reserve using 
			// chunkSize chunks.  At least minChunks will be allocated.  At most 
			// maxChunks will be allocated.  There will be at least freeHeapSpaceRequired 
			// bytes left in the heap after the call.
			//
			// There are some simple rules to following for these parameters.
			// 
			// o In order for this module to be effective chunkSize should be
			//   significantly bigger than the largest block you will allocate 
			//   using OTAllocMemFromReserve.
			//
			// o You should set freeHeapSpaceRequired to at least 32 KB, preferably 
			//   a larger value like 64 or 128 KB.  Mac OS does not deal well if 
			//   if you have very little free space in your application partition.
			//
			// o If you intend to use OTAllocMemFromReserve for your 
			//   primary application allocator, (minChunks * chunkSize) + freeHeapSpaceRequired + static allocation size
			//   should be approximately equal to your minimum application partition 
			//   size.
			//
			// o Don’t pass $FFFFFFFF to maxChunks, even if you want your entire 
			//   application partition (except for freeHeapSpaceRequired bytes) 
			//   to be used for the OT memory reserve.  This will work on current 
			//   systems (because OTAllocMemInContext will be allocating from 
			//   your application partition) but it may cause problems on future 
			//   systems (if OTAllocMemInContext is revised to get memory from 
			//   a read/write file mapping, for example).  Thus is actually the 
			//   case on Mac OS X.  This module does not make a lot of sense on 
			//   Mac OS X, but it will work properly if you pass a reasonable 
			//   value for maxChunks.
			//
			// o It’s likely that passing anything other than nil for clientContext 
			//   is a bad idea.  This routine allocates the routine using OTAllocMemInContext, 
			//   passing clientContext as the context parameter.  If you pass a non-nil 
			//   value (not the primary application context), it’s likely that 
			//   OTAllocMemInContext won’t allocate memory from the application heap 
			//   and thus your 
			
			extern NMErr UpkeepOTMemoryReserve(void);
			//refills the module - if your reserve has been used and then
			//freed, OT may not keep the memory available - call this periodically
			//to ensure you always have it.
	
			extern void TermOTMemoryReserve(void);
			// Terminates the module.
	
			extern void *OTAllocMemFromReserve(OTByteCount size);
			// Allocates memory from OT and, if that fails, returns part of 
			// the memory reserve to OT and tries again.
	
			#if DEBUG
				extern void OTMemoryReserveTest(void);
			#endif

	#elif defined(OP_PLATFORM_WINDOWS)

		#define  new_pointer(size)      (void *)GlobalAlloc(GPTR, size)
		#define  dispose_pointer(x)  	if (x != NULL) { GlobalFree((HGLOBAL) x); }

	#else

		#define  new_pointer(size)      malloc(size)
		#define  dispose_pointer(x)  	if (x != NULL) { free(x); }

	#endif

//====================================================================================================================
// InterruptSafe Alloc/Free functions....
//====================================================================================================================

	#ifdef OP_PLATFORM_MAC_CFM
	
		//#ifdef	OP_PLATFORM_MAC_CARBON_FLAG					// carbon opentransport fixes and workarounds
		//extern	OTClientContextPtr				gOTClientContext;
		//#define	InterruptSafe_alloc(s)			OTAllocMemInContext(s, gOTClientContext)
		//#else

			#define InterruptSafe_alloc(size) 			OTAllocMemFromReserve(size)

		//#endif

		#define InterruptSafe_free(mem)				OTFreeMem(mem)
		
		//memory reserve functions to augment 

	#else

		#define InterruptSafe_alloc(size) 			new_pointer(size)
		#define InterruptSafe_free(mem)				dispose_pointer(mem)

	#endif


//====================================================================================================================
// Utility functions....
//====================================================================================================================


	
	extern NMSInt32 	psprintf(unsigned char *buffer, const char *format, ...);

	extern char *		csprintf(char *buffer, char *format, ...);

	extern NMUInt32 	machine_tick_count(void);

	extern	NMUInt32	GetTimestampMilliseconds();

#ifdef __cplusplus
}
#endif // __cplusplus

#endif
