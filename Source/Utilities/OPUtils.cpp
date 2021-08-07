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
#include 			"OpenPlay.h"
#endif

#include "OPUtils.h"
#include <stdarg.h>
#include <time.h>
#include "String_Utils.h"

#ifdef OP_PLATFORM_WINDOWS
	#if DEBUG
		#include <winsock.h>
	#endif
#elif (! defined(OP_PLATFORM_MAC_CFM))
	#include <errno.h>
	#include <sys/time.h>
	#include <unistd.h>
#endif


//	------------------------------	Private Definitions

//to use printf under macos9 in place of DebugStr, set this to 1
#define MACOS9_USEPRINTF 0
//same only under macosX
#define MACOSX_USEPRINTF 1

#define PRINT_CASE(x) case (x): name= #x; break;


//	------------------------------	Private Types

//	------------------------------	Private Variables
#if DEBUG
	char sz_temporary[1024];
#endif

#ifdef OP_PLATFORM_MAC_CFM
	NMBoolean gCheckedOSVersion = false;
	NMBoolean gRunningOSX;
	
		extern	OTClientContextPtr  gOTClientContext;
#endif

//	------------------------------	Private Functions



//	------------------------------	Public Variables


/* ------------- BEGIN CODE ------------- */

/* --------- macintosh code */

#ifdef OP_PLATFORM_MAC_CFM
//----------------------------------------------------------------------------------------
//checkMacOSVersion
// -figure out if we're running OS-X or not
//----------------------------------------------------------------------------------------
void checkMacOSVersion()
{
	UInt32 response;
	if(Gestalt(gestaltSystemVersion, (SInt32 *) &response) == noErr){
		if (response >= 0x1000)
			gRunningOSX = true;
		else
			gRunningOSX = false;
	}
}

#endif

//----------------------------------------------------------------------------------------
// debug_message 
//----------------------------------------------------------------------------------------

static void
debug_message(const char *inMessage)
{
#ifdef OP_PLATFORM_MAC_CFM
	
	//see if we're running OS-X
	if (gCheckedOSVersion == false)
		checkMacOSVersion();
		
	#if DEBUG
		NMBoolean usePrintf;

		if (gRunningOSX)
		{
			#if MACOSX_USEPRINTF
				usePrintf = true;
			#else
				usePrintf = false;
			#endif //MACOSX_USEPRINTF
		}
		else
		{
			#if MACOS9_USEPRINTF
				usePrintf = true;
			#else
				usePrintf = false;
			#endif //MACOS9_USEPRINTF
		}
		
		if (usePrintf)
		{
			printf("%s\n",inMessage);
		}	
		else
		{
	unsigned char	aMessagePString[256];
	NMSInt32			aSize = 0;

		while (*inMessage != 0)
		{
			aSize++;	
			aMessagePString[aSize] = *inMessage++;
			if (aSize == 255)
				break;
		}	
		aMessagePString[0] = aSize;
		DebugStr((ConstStr255Param)aMessagePString);
		}
	#else
		UNUSED_PARAMETER(inMessage)
	#endif	

#elif OP_PLATFORM_WINDOWS

	#if DEBUG
//LR		MessageBox(NULL, inMessage, NULL, MB_OK);
		OutputDebugString(inMessage);
		//OutputDebugString("\n\r");
	#else
		UNUSED_PARAMETER(inMessage)
	#endif	
#else
	#if DEBUG
		fprintf(stderr, "%s\n",inMessage);
		fflush(stderr);
	#else
		UNUSED_PARAMETER(inMessage)
	#endif
#endif	
}

//----------------------------------------------------------------------------------------
// machine_tick_count 
//----------------------------------------------------------------------------------------

NMUInt32
machine_tick_count(void)
{
#ifdef OP_PLATFORM_MAC_CFM
	return TickCount();
#elif defined(OP_PLATFORM_WINDOWS)
	return GetTickCount();
#else
  struct timeval tp;
  struct timezone tz;
  long result = gettimeofday(&tp,&tz);
  op_assert(result == 0);
  return (tp.tv_sec * 1000) + (tp.tv_usec/1000); //return milliseconds
#endif	
}

//----------------------------------------------------------------------------------------
//
//  GetTimestampMilliseconds()
//
//  Platform-independent millisecond extraction routine.
//
//----------------------------------------------------------------------------------------

NMUInt32 GetTimestampMilliseconds()
{
	NMUInt32		timestamp = 0;
	NMUInt32 		clocksPerSec;
	
#ifdef OP_PLATFORM_MAC_CFM
	double		clocks;
#else
	clock_t		clocks;
#endif


#ifdef OP_PLATFORM_MAC_CFM
	UnsignedWide mSeconds;
	clocksPerSec = 1000000;  // Magic Number = Microseconds
	Microseconds( &mSeconds );
    
	// Convert to appropriate type in the floating point domain
	clocks = ( mSeconds.hi * 4294967296.0 + mSeconds.lo );
	 
#else
	clocksPerSec = MACHINE_TICKS_PER_SECOND;
	clocks = machine_tick_count();

#endif

	timestamp = (NMUInt32)((clocks / (double)clocksPerSec) * (double)MILLISECS_PER_SEC);

	return timestamp;
}

//----------------------------------------------------------------------------------------
//	csprintf
//----------------------------------------------------------------------------------------

char *
csprintf(char *buffer, char *format, ...)
{
va_list arglist;
	
	va_start(arglist, format);
	vsprintf(buffer, format, arglist);
	va_end(arglist);
	
	return buffer;
}

//----------------------------------------------------------------------------------------
//	psprintf
//----------------------------------------------------------------------------------------

NMSInt32
psprintf(unsigned char *buffer, const char *format, ...)
{
va_list		arglist;
NMSInt32	return_value;
NMSInt16	length;
	
	va_start(arglist, format);
	return_value = vsprintf((char *)buffer + 1, format, arglist);
	va_end(arglist);
	
	buffer[0] = (unsigned char)((length = strlen((char *)buffer + 1)) > 255) ? 255 : length;
	
	return return_value;
}
#if DEBUG
	//----------------------------------------------------------------------------------------
	//	_assertion_failure
	//----------------------------------------------------------------------------------------

	void
	_assertion_failure(
		char		*information,
		char		*file,
		NMUInt32	line,
		NMBoolean	fatal)
	{
	  char buffer[256];

	  sprintf((char *) buffer, "%s in %s,#%d: %s", fatal ? "op_halt" : "op_pause", file, line,
	          information ? information : "<no reason given>");

	  debug_message(buffer);
		
	  if (fatal) 
	    FATAL_EXIT;
	}

	//----------------------------------------------------------------------------------------
	// op_dprintf
	//----------------------------------------------------------------------------------------

	NMSInt32 op_dprintf(const char *format, ...)
	{
	  char		buffer[257];   /* [length byte] + [255 string bytes] + [null] */
	  va_list	arglist;
	  NMSInt32	return_value;

	  va_start(arglist, format);
	  return_value = vsprintf(buffer, format, arglist);
	  va_end(arglist);

	  debug_message(buffer);	
		
	  return(return_value);
	}

#ifdef OP_PLATFORM_MAC_CFM

	//----------------------------------------------------------------------------------------
	// dprintf_oterr
	//----------------------------------------------------------------------------------------

		void
		dprintf_oterr(
			EndpointRef	ep,
			char		*message,
			NMErr	err,
			char		*file,
			NMSInt32	line)
		{
		char	*name;
		char	error[256];
		char	state[256];

			if (err)
			{
				switch (err)
				{
					PRINT_CASE(kOTOutOfMemoryErr);
					PRINT_CASE(kOTNotFoundErr);
					PRINT_CASE(kOTDuplicateFoundErr);
					PRINT_CASE(kOTBadAddressErr);
					PRINT_CASE(kOTBadOptionErr);
					PRINT_CASE(kOTAccessErr);
					PRINT_CASE(kOTBadReferenceErr);
					PRINT_CASE(kOTNoAddressErr);
					PRINT_CASE(kOTOutStateErr);
					PRINT_CASE(kOTBadSequenceErr);
					PRINT_CASE(kOTSysErrorErr);
					PRINT_CASE(kOTLookErr);
					PRINT_CASE(kOTBadDataErr);
					PRINT_CASE(kOTBufferOverflowErr);
					PRINT_CASE(kOTFlowErr);
					PRINT_CASE(kOTNoDataErr);
					PRINT_CASE(kOTNoDisconnectErr);
					PRINT_CASE(kOTNoUDErrErr);
					PRINT_CASE(kOTBadFlagErr);
					PRINT_CASE(kOTNoReleaseErr);
					PRINT_CASE(kOTNotSupportedErr);
					PRINT_CASE(kOTStateChangeErr);
					PRINT_CASE(kOTNoStructureTypeErr);
					PRINT_CASE(kOTBadNameErr);
					PRINT_CASE(kOTBadQLenErr);
					PRINT_CASE(kOTAddressBusyErr);
					PRINT_CASE(kOTIndOutErr);
					PRINT_CASE(kOTProviderMismatchErr);
					PRINT_CASE(kOTResQLenErr);
					PRINT_CASE(kOTResAddressErr);
					PRINT_CASE(kOTQFullErr);
					PRINT_CASE(kOTProtocolErr);
					PRINT_CASE(kOTBadSyncErr);
					PRINT_CASE(kOTCanceledErr);
					PRINT_CASE(kEPERMErr);
			//		PRINT_CASE(kENOENTErr);
					PRINT_CASE(kENORSRCErr);
					PRINT_CASE(kEINTRErr);
					PRINT_CASE(kEIOErr);
					PRINT_CASE(kENXIOErr);
					PRINT_CASE(kEBADFErr);
					PRINT_CASE(kEAGAINErr);
			//		PRINT_CASE(kENOMEMErr);
					PRINT_CASE(kEACCESErr);
					PRINT_CASE(kEFAULTErr);
					PRINT_CASE(kEBUSYErr);
			//		PRINT_CASE(kEEXISTErr);
					PRINT_CASE(kENODEVErr);
					PRINT_CASE(kEINVALErr);
					PRINT_CASE(kENOTTYErr);
					PRINT_CASE(kEPIPEErr);
					PRINT_CASE(kERANGEErr);
					PRINT_CASE(kEWOULDBLOCKErr);
			//		PRINT_CASE(kEDEADLKErr);
					PRINT_CASE(kEALREADYErr);
					PRINT_CASE(kENOTSOCKErr);
					PRINT_CASE(kEDESTADDRREQErr);
					PRINT_CASE(kEMSGSIZEErr);
					PRINT_CASE(kEPROTOTYPEErr);
					PRINT_CASE(kENOPROTOOPTErr);
					PRINT_CASE(kEPROTONOSUPPORTErr);
					PRINT_CASE(kESOCKTNOSUPPORTErr);
					PRINT_CASE(kEOPNOTSUPPErr);
					PRINT_CASE(kEADDRINUSEErr);
					PRINT_CASE(kEADDRNOTAVAILErr);
					PRINT_CASE(kENETDOWNErr);
					PRINT_CASE(kENETUNREACHErr);
					PRINT_CASE(kENETRESETErr);
					PRINT_CASE(kECONNABORTEDErr);
					PRINT_CASE(kECONNRESETErr);
					PRINT_CASE(kENOBUFSErr);
					PRINT_CASE(kEISCONNErr);
					PRINT_CASE(kENOTCONNErr);
					PRINT_CASE(kESHUTDOWNErr);
					PRINT_CASE(kETOOMANYREFSErr);
					PRINT_CASE(kETIMEDOUTErr);
					PRINT_CASE(kECONNREFUSEDErr);
					PRINT_CASE(kEHOSTDOWNErr);
					PRINT_CASE(kEHOSTUNREACHErr);
					PRINT_CASE(kEPROTOErr);
					PRINT_CASE(kETIMEErr);
					PRINT_CASE(kENOSRErr);
					PRINT_CASE(kEBADMSGErr);
					PRINT_CASE(kECANCELErr);
					PRINT_CASE(kENOSTRErr);
					PRINT_CASE(kENODATAErr);
					PRINT_CASE(kEINPROGRESSErr);
					PRINT_CASE(kESRCHErr);
					PRINT_CASE(kENOMSGErr);
					PRINT_CASE(kOTClientNotInittedErr);
					PRINT_CASE(kOTPortHasDiedErr);
					PRINT_CASE(kOTPortWasEjectedErr);
					PRINT_CASE(kOTBadConfigurationErr);
					PRINT_CASE(kOTConfigurationChangedErr);
					PRINT_CASE(kOTUserRequestedErr);
					PRINT_CASE(kOTPortLostConnection);

					default:
						name = "< unknown >";
						break;
				}
				strcpy(error,name);
				
				if (ep)
				{
					switch (OTGetEndpointState(ep))
					{	
						PRINT_CASE(T_UNINIT);
						PRINT_CASE(T_UNBND);
						PRINT_CASE(T_IDLE);
						PRINT_CASE(T_OUTCON);
						PRINT_CASE(T_INCON);
						PRINT_CASE(T_DATAXFER);
						PRINT_CASE(T_OUTREL);
						PRINT_CASE(T_INREL);

						default:
							name = "< unknown >";
							break;
					}
				}
				else
					name = "<unknown>";

				strcpy(state,name);
					
				DEBUG_PRINT("EP: %p (state: %s), OTError for %s: %s (%d). File: %s, %d",ep, state, message, error, err, file, line);
			}
		}
#elif defined(OP_PLATFORM_WINDOWS)

	//----------------------------------------------------------------------------------------
	// dprintf_err
	//----------------------------------------------------------------------------------------

	NMErr
	dprintf_err(
		char	*message,
		NMErr	err,
		char		*file,
		NMSInt32	line)
	{
	char 	*name;
		
		if (err == SOCKET_ERROR)
			err = WSAGetLastError();
			
		switch (err)
		{
			PRINT_CASE(WSAEINTR);
			PRINT_CASE(WSAEBADF);
			PRINT_CASE(WSAEACCES);
			PRINT_CASE(WSAEFAULT);
			PRINT_CASE(WSAEINVAL);
			PRINT_CASE(WSAEMFILE);
			PRINT_CASE(WSAEWOULDBLOCK);
			PRINT_CASE(WSAEINPROGRESS);
			PRINT_CASE(WSAEALREADY);
			PRINT_CASE(WSAENOTSOCK);
			PRINT_CASE(WSAEDESTADDRREQ);
			PRINT_CASE(WSAEMSGSIZE);
			PRINT_CASE(WSAEPROTOTYPE);
			PRINT_CASE(WSAENOPROTOOPT);
			PRINT_CASE(WSAEPROTONOSUPPORT);
			PRINT_CASE(WSAESOCKTNOSUPPORT);
			PRINT_CASE(WSAEOPNOTSUPP);
			PRINT_CASE(WSAEPFNOSUPPORT);
			PRINT_CASE(WSAEAFNOSUPPORT);
			PRINT_CASE(WSAEADDRINUSE);
			PRINT_CASE(WSAEADDRNOTAVAIL);
			PRINT_CASE(WSAENETDOWN);
			PRINT_CASE(WSAENETUNREACH);
			PRINT_CASE(WSAENETRESET);
			PRINT_CASE(WSAECONNABORTED);
			PRINT_CASE(WSAECONNRESET);
			PRINT_CASE(WSAENOBUFS);
			PRINT_CASE(WSAEISCONN);
			PRINT_CASE(WSAENOTCONN);
			PRINT_CASE(WSAESHUTDOWN);
			PRINT_CASE(WSAETOOMANYREFS);
			PRINT_CASE(WSAETIMEDOUT);
			PRINT_CASE(WSAECONNREFUSED);
			PRINT_CASE(WSAELOOP);
			PRINT_CASE(WSAENAMETOOLONG);
			PRINT_CASE(WSAEHOSTDOWN);
			PRINT_CASE(WSAEHOSTUNREACH);
			PRINT_CASE(WSAENOTEMPTY);
			PRINT_CASE(WSAEPROCLIM);
			PRINT_CASE(WSAEUSERS);
			PRINT_CASE(WSAEDQUOT);
			PRINT_CASE(WSAESTALE);
			PRINT_CASE(WSAEREMOTE);
			PRINT_CASE(WSAEDISCON);
			PRINT_CASE(WSASYSNOTREADY);
			PRINT_CASE(WSAVERNOTSUPPORTED);
			PRINT_CASE(WSANOTINITIALISED);
			PRINT_CASE(WSAHOST_NOT_FOUND);
			PRINT_CASE(WSATRY_AGAIN);
			PRINT_CASE(WSANO_RECOVERY);
			PRINT_CASE(WSANO_DATA);
			default:
				name = "<unknown>";
				break;
		}

		DEBUG_PRINT("Winsock Err: %s - %s (%d) File: %s, %d", message, name, err,file, line);
		return err;
	}

#elif defined(OP_PLATFORM_UNIX) || defined(OP_PLATFORM_MAC_MACHO)

	//----------------------------------------------------------------------------------------
	// dprintf_err
	//----------------------------------------------------------------------------------------

	NMErr
	dprintf_err(
		char	*message,
		NMErr	err,
		char		*file,
		NMSInt32	line)
	{
	char 	*name;
		
		if (err != -1)
		{
			DEBUG_PRINT("error value %d passed to dprintf_sockerr for %s. not checking errno. %s line %d",err,message,file,line);
			return 0;
		}
			
		switch (errno)
		{
			PRINT_CASE(EPERM);
			PRINT_CASE(ENOENT);
			PRINT_CASE(ESRCH);
			PRINT_CASE(EINTR);
			PRINT_CASE(EIO);
			PRINT_CASE(ENXIO);
			PRINT_CASE(E2BIG);
			PRINT_CASE(ENOEXEC);
			PRINT_CASE(EBADF);
			PRINT_CASE(ECHILD);
			PRINT_CASE(EDEADLK);
			PRINT_CASE(ENOMEM);
			PRINT_CASE(EACCES);
			PRINT_CASE(EFAULT);
			PRINT_CASE(ENOTBLK);
			PRINT_CASE(EBUSY);
			PRINT_CASE(EEXIST);
			PRINT_CASE(EXDEV);
			PRINT_CASE(ENODEV);
			PRINT_CASE(ENOTDIR);
			PRINT_CASE(EISDIR);
			PRINT_CASE(EINVAL);
			PRINT_CASE(ENFILE);
			PRINT_CASE(EMFILE);
			PRINT_CASE(ENOTTY);
			PRINT_CASE(ETXTBSY);
			PRINT_CASE(EFBIG);
			PRINT_CASE(ENOSPC);
			PRINT_CASE(ESPIPE);
			PRINT_CASE(EROFS);
			PRINT_CASE(EMLINK);
			PRINT_CASE(EPIPE);
			PRINT_CASE(EDOM);
			PRINT_CASE(ERANGE);
			PRINT_CASE(EWOULDBLOCK);
			PRINT_CASE(EINPROGRESS);
			PRINT_CASE(EALREADY);
			PRINT_CASE(ENOTSOCK);
			PRINT_CASE(EDESTADDRREQ);
			PRINT_CASE(EMSGSIZE);
			PRINT_CASE(EPROTOTYPE);
			PRINT_CASE(ENOPROTOOPT);
			PRINT_CASE(EPROTONOSUPPORT);
			PRINT_CASE(ESOCKTNOSUPPORT);
			PRINT_CASE(ENOTSUP);
			PRINT_CASE(EPFNOSUPPORT);
			PRINT_CASE(EAFNOSUPPORT);
			PRINT_CASE(EADDRINUSE);
			PRINT_CASE(EADDRNOTAVAIL);
			PRINT_CASE(ENETDOWN);
			PRINT_CASE(ENETUNREACH);
			PRINT_CASE(ENETRESET);
			PRINT_CASE(ECONNABORTED);
			PRINT_CASE(ECONNRESET);
			PRINT_CASE(ENOBUFS);
			PRINT_CASE(EISCONN);
			PRINT_CASE(ENOTCONN);
			PRINT_CASE(ESHUTDOWN);
			PRINT_CASE(ETOOMANYREFS);
			PRINT_CASE(ETIMEDOUT);
			PRINT_CASE(ECONNREFUSED);
			PRINT_CASE(ELOOP);
			PRINT_CASE(ENAMETOOLONG);
			PRINT_CASE(EHOSTDOWN);
			PRINT_CASE(EHOSTUNREACH);
			PRINT_CASE(ENOTEMPTY);
			PRINT_CASE(EUSERS);
			PRINT_CASE(EDQUOT);
			PRINT_CASE(ESTALE);
			PRINT_CASE(EREMOTE);
			PRINT_CASE(ENOLCK);
			PRINT_CASE(ENOSYS);
			PRINT_CASE(EOVERFLOW);
#if (OP_PLATFORM_MAC_MACHO)
				PRINT_CASE(EPROCLIM);
				PRINT_CASE(EBADRPC);
				PRINT_CASE(ERPCMISMATCH);
				PRINT_CASE(EPROGUNAVAIL);
				PRINT_CASE(EFTYPE);
				PRINT_CASE(ENEEDAUTH);
				PRINT_CASE(EPWROFF);
				PRINT_CASE(EDEVERR);
				PRINT_CASE(EBADEXEC);
				PRINT_CASE(EBADARCH);
				PRINT_CASE(ESHLIBVERS);
				PRINT_CASE(EBADMACHO);
				PRINT_CASE(EAUTH);
			#endif
			
			default:
				name = "<unknown>";
				break;
		}

		DEBUG_PRINT("Socket Err: %s - %s (%d) File: %s, %d", message, name, errno,file, line);
		return err;
	}

#endif // OP_PLATFORM_WINDOWS = false

#endif 

#if OP_PLATFORM_MAC_CFM

// The following is based on code written by Quinn - see comments in OPUtils.h for what it does/how it works
/////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////

static OTLIFO gMemoryReserve = {nil};
static NMBoolean gReserveInited = false;
static ItemCount gMaxReserveChunks;
static ByteCount gFreeHeapSpaceRequired;
static ByteCount gReserveChunkSize;
static ItemCount gMinReserveChunks;
static ItemCount gReserveChunksAllocated;

#if DEBUG

	//static OTClientContextPtr gClientContext;

	static ItemCount CountReserveChunks(void)
	{
		ItemCount count;
		OTLink *cursor;
		
		count = 0;
		cursor = gMemoryReserve.fHead;
		while ( cursor != nil ) {
			count += 1;
			cursor = cursor->fNext;
		}
		return count;
	}

#endif

NMErr InitOTMemoryReserve(ByteCount freeHeapSpaceRequired, ByteCount chunkSize, 
										   ItemCount minChunks, ItemCount maxChunks)
	// See comment in interface part.
{
	NMErr	err;
	
	if (freeHeapSpaceRequired < 32768)
		freeHeapSpaceRequired = 32768;
	//op_assert(freeHeapSpaceRequired >= 32768);	// Leave less than this in your app heap is extremely dangerous!
	if (chunkSize < 8192)
		chunkSize = 8192;
	//op_assert(chunkSize >= 8192);					// Using this for smaller blocks is pretty silly.
	op_assert(minChunks != 0);					// Passing 0 is silly, and loop below doesn’t work properly.
	if (minChunks == 0)
		minChunks = 1;
	op_assert(maxChunks != 0);					// Passing 0 is silly, and loop below doesn’t work properly.
	if (maxChunks == 0)
		maxChunks = minChunks;
	op_assert(maxChunks != 0xFFFFFFFF);			// See comment in header file.
	op_assert(minChunks <= maxChunks);

	op_assert(gMemoryReserve.fHead == nil);		// You’ve probably initialised the module twice.
	
	gFreeHeapSpaceRequired = freeHeapSpaceRequired;
	gReserveChunkSize = chunkSize;
	gMinReserveChunks = minChunks;
	gReserveChunksAllocated = 0;
	gMaxReserveChunks = maxChunks;

	gReserveInited = true;
	
	err = UpkeepOTMemoryReserve();
	
	if (err == noErr) {
		// Check some post conditions.
		#if DEBUG
			op_assert( (gReserveChunksAllocated >= gMinReserveChunks) && (gReserveChunksAllocated <= gMaxReserveChunks) );
			op_assert( FreeMem() >= freeHeapSpaceRequired );
			op_assert( gReserveChunksAllocated == CountReserveChunks() );
		#endif

	} else {
		// Free up any chunks that we allocated.
		TermOTMemoryReserve();
		gReserveInited = false;
	}

	return err;	
}

//ECF - added this to be able to keep a nice, full reserve as time goes on. - just call periodically at system-time
NMErr UpkeepOTMemoryReserve(void)
{
	NMErr err = noErr;
	Boolean done = false;
	OTLink *	thisChunk;
	
	if (gReserveInited == false)
		return paramErr;
		
	//if its full, return happy
	if (gReserveChunksAllocated >= gMaxReserveChunks)
		return 0;
		
	do {
		// Only try allocating this chunk if doing so will not 
		// break the minimum free space requirement.

		thisChunk = nil;
		if ( FreeMem() >= (gFreeHeapSpaceRequired + gReserveChunkSize) ) {
			//thisChunk = OTAllocMemInContext(chunkSize, clientContext);
			#if (OP_PLATFORM_MAC_CARBON_FLAG)
				thisChunk = (OTLink*)OTAllocMemInContext(gReserveChunkSize,gOTClientContext);
			#else
				thisChunk = (OTLink*)OTAllocMem(gReserveChunkSize);
			#endif
		}

		if (thisChunk == nil) {
			// Either the next block is going to push us below the free 
			// space requirement, or we tried to get the block and failed.
			if (gReserveChunksAllocated < gMinReserveChunks) {
				// Because we’re still trying to get our minimum chunks, 
				// this is a fatal error.
				err = memFullErr;
			} else {
				// The block was optional so we don’t need to error. 
				// We set done and leave the loop.
				done = true;
			}
		} else {
			// We got the block.  Put it into the reserve.  If we have 
			// all of our chunks, set done so that we leave the loop.
			OTLIFOEnqueue(&gMemoryReserve, (OTLink *) thisChunk);
			gReserveChunksAllocated += 1;
			if (gReserveChunksAllocated == gMaxReserveChunks) {
				done = true;
			}
		}
	} while (err == noErr && !done);

	return err;
}

void TermOTMemoryReserve(void)
	// See comment in interface part.
{
	do {
		OTLink *thisChunk;
		
		thisChunk = OTLIFODequeue(&gMemoryReserve);
		if (thisChunk != nil) {
			OTFreeMem(thisChunk);
		}
	} while ( gMemoryReserve.fHead != nil );
}

void *OTAllocMemFromReserve(OTByteCount size)
	// See comment in interface part.
{
	void *result;
	
	// First try to allocate the memory from the OT client pool.
	// If this succeeds, we’re done.  If it fails, the failure
	// will trigger OT to try and grow the client pool the next 
	// time SystemTask is called.  However, if the memory reserve 
	// still has memory we free a block of the reserve and retry 
	// the allocation.  Hopefully that will allow the allocation to 
	// succeed.
	//
	// Note that I originally wrapped this in a do {} while loop, 
	// however I deleted that because I figured that all of the 
	// blocks are the same size, so if freeing one block didn’t 
	// help then freeing another won’t help either.  Except 
	// OT might try to coallesce the memory blocks if they live 
	// in the same Mac OS Memory Manager memory block.  However, 
	// they probably won’t live in the same Memory Manager block 
	// because the blocks are large.  Regardless, freeing all of 
	// our reserve blocks at once will cause the entire reserve 
	// to go the first time if the client tries to allocate a 
	// block large than our block size, which is not good.
		
	//ECF:  we're doing the while() thing again - since we refill the cache
	// its not terribly catastrophic, and there is a chance it will work...
	
	//if the memory-reserve hasnt been inited yet, just return a normal call
	if (gReserveInited == false)
#ifdef OP_PLATFORM_MAC_CARBON_FLAG
		return OTAllocMemInContext(size,gOTClientContext);
	#else
		return OTAllocMem(size);
#endif
	
	//when running OSX we just return what we get the first time
	#if OP_PLATFORM_MAC_CARBON_FLAG
		if (gRunningOSX)
			return OTAllocMemInContext(size,gOTClientContext);
	
		result = OTAllocMemInContext(size,gOTClientContext);
	#else
		result = OTAllocMem(size);
	#endif
	while (result == nil) {
		OTLink *thisChunk;
		
		//if theres nothing left to release, give up
		if (gReserveChunksAllocated == 0)
			break;
			
		thisChunk = OTLIFODequeue(&gMemoryReserve);
		if (thisChunk != nil) {
			OTFreeMem(thisChunk);
			gReserveChunksAllocated--;
				
			#if (OP_PLATFORM_MAC_CARBON_FLAG)
				result = OTAllocMemInContext(size,gOTClientContext);
			#else
				result = OTAllocMem(size);
			#endif
		}
	}
	
	return result;
}

	#if DEBUG
	void OTMemoryReserveTest(void)
		// This routine is designed to help test OTMemoryReserve 
		// by allocating all the memory in the reserve and checking 
		// that it roughly matches the expected size of the reserve.
	{
		OTLink *thisBlock;
		OTList blockList;
		ByteCount bytesAllocated;
		ByteCount targetBytes = (((gReserveChunksAllocated * gReserveChunkSize) * 9) / 10);
		op_assert(gReserveChunkSize != 0);		// You must init the module before doing the test.

		#if (OP_PLATFORM_MAC_CARBON_FLAG)
			return; //cant use OTEnterIntertupt();
		#endif
		
		// Tell OT that we’re at interrupt time so that it won’t 
		// grow the client pool.
		
		#if (!OP_PLATFORM_MAC_CARBON_FLAG)
			OTEnterInterrupt();
		#endif
		
		// Allocate all of the memory that OT will give us - or until we get what we need...
		blockList.fHead = nil;
		bytesAllocated  = 0;
		do {
			thisBlock = (OTLink*)OTAllocMemFromReserve(1024);
			if (thisBlock != nil) {
				OTAddFirst(&blockList, thisBlock);
				bytesAllocated += 1024;
			}
		} while ((thisBlock != nil) && (bytesAllocated < targetBytes));
		
		// Check that it’s approproximately what the client asked for.
		
		// Note that the * 9 / 10 won’t work properly for very large client pools.

		op_assert( bytesAllocated >= targetBytes );

		// Free the memory we allocated.
		
		do {
			thisBlock = OTRemoveFirst(&blockList);
			if (thisBlock != nil) {
				OTFreeMem(thisBlock);
			}
		} while (thisBlock != nil);
		
		#if (!OP_PLATFORM_MAC_CARBON_FLAG)
			OTLeaveInterrupt();		
		#endif
		
	}
	#endif //DEBUG

#endif //OP_PLATFORM_MAC_CFM

