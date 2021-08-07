/*
 * OpenPlay 2.2 release 1 (v2.2r1) build 040511
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
 * The Latest OpenPlay code is in CVS on SourceForge:
 *
 *						http://www.sourcefourge.net/projects/openplay
 *
 * The OpenPlay binary package is avaible on the Official Apple OpenPlay webpage:
 *
 *						http://developer.apple.com/darwin/projects/openplay/
 *
 * The OpenPlay Mailing list signup page is at:
 *
 *						http://lists.apple.com/mailman/listinfo/openplay-development
 *
 */

#ifndef __OPENPLAY__
#define __OPENPLAY__

/*-------------------------------------------------------------------------------------------
	Determine which platform we're compiling for: 
	
	We currently support the following platform builds:
	
		OP_PLATFORM_MAC_CFM: 	MacOS classic and MacOS Carbon using OpenTransport
			
			OP_PLATFORM_MAC_CARBON_FLAG: can be defined.
					
		OP_PLATFORM_WINDOWS: 		Windows 32-bit using the WinSock API
		
		OP_PLATFORM_MAC_MACHO:	MacOSXin this case we use bundles, frameworks like coreservices, carbon, etc
			
			OP_PLATFORM_MAC_CARBON_FLAG: can be defined.

		OP_PLATFORM_UNIX:		macOSX/linux/unix using POSIX API.

*/

	#ifdef __MWERKS__			/* Metrowerks CodeWarrior Compiler */

		#ifdef __INTEL__		/* Compiling for x86, assume windows */
			#define OP_PLATFORM_WINDOWS  1
			#define little_endian  1
			#define has_byte_type  1
		#elif defined(__POWERPC__)	/* Compiling for PowerPC, assume MacOS */

			#ifdef __MACH__
				#define OP_PLATFORM_MAC_MACHO	1
				#define OP_API_NETWORK_SOCKETS 1

				#include <machine/endian.h> /*bsd,osx,etc*/
			#else
				#define OP_PLATFORM_MAC_CFM		1
			#endif
			
			#define big_endian	1
			#undef  has_byte_type
		#else
		  #error "unsupported Metrowerks target"
		#endif
	
		#undef  has_unused_pragma	/* unused pragma can't be used in define 8-( */
	
	#elif defined(_MSC_VER)			/* microsoft compiler */
		#define little_endian 1
		#define OP_PLATFORM_WINDOWS  1
		#undef  has_unused_pragma 
		#undef  has_byte_type
		#undef  has_mark_pragma
	
	#elif defined(__MRC__)			/* mpw mrc */
		#define OP_PLATFORM_MAC_CFM		1
		#define big_endian  1

		#undef  has_unused_pragma 
		#undef  has_byte_type
	
	#elif defined(__WATCOMC__)		/* Watcom, set to fake Microsoft compiler */
		#define little_endian 1			
		#define MICROSOFT
		#define OP_PLATFORM_WINDOWS	1

		#define has_unused_pragma 1
		#define has_byte_type
	
	#else	/* lump everything else under Posix -- linux, unix, etc. */		

		#ifdef linux
			#include <endian.h>
		#else
			#include <machine/endian.h> /*bsd,osx,etc*/
		#endif 		

		#if defined(project_builder)
			#define OP_PLATFORM_MAC_MACHO 1
		#else
			#define OP_PLATFORM_UNIX 1

//
//		We make this decision later...
//
//		#elif defined(os_darwin)
//			#define OP_PLATFORM_MAC_MACHO 1
//			#define macosx_darwin_build 1

		#endif
		
		#ifdef linux
			#if __BYTE_ORDER == __BIG_ENDIAN
				#define big_endian 1
			#endif
			#if __BYTE_ORDER == __LITTLE_ENDIAN
				#define little_endian 1
			#endif		
		#else
			#ifdef __BIG_ENDIAN__
				#define big_endian 1
			#endif 
			#ifdef __LITTLE_ENDIAN__
				#define little_endian 1
			#endif
		#endif /* linux */
		
		#undef  has_unused_pragma
		#undef  has_byte_type			
	#endif
		
	/* make sure we've defined one and only one endian type */
	#if defined(big_endian) && defined(little_endian)
	 	#error "Somehow big and little endian got defined!!"
	#endif
	
	#ifndef big_endian
		#ifndef little_endian
			#error "need to define either big or little endian"
		#endif
	#endif
	
/*-------------------------------------------------------------------------------------------
	basic types - we may need to do some redefining for 64-bit systems eventually...
*/
	#if (!DOXYGEN)
		typedef unsigned char	NMBoolean;

		typedef unsigned long	NMUInt32;
		typedef signed long		NMSInt32;

		typedef unsigned short	NMUInt16;
		typedef signed short	NMSInt16;

		typedef unsigned char	NMUInt8;
		typedef signed char		NMSInt8;
	#endif
	
	typedef NMUInt16						NMInetPort;
	typedef unsigned char 					NMStr31[32];
	typedef const unsigned char *			NMConstStr31Param;

	
/*-------------------------------------------------------------------------------------------
	Specify any specific platfrom settings.
*/

	#if PRAGMA_ONCE
	#pragma once
	#endif
	
	#ifdef OP_PLATFORM_MAC_CFM

		#if TARGET_API_MAC_CARBON
			#define OP_PLATFORM_MAC_CARBON_FLAG 1
		#endif
		
			#include <ConditionalMacros.h>
			
			#ifndef __MACTYPES__
			#include <MacTypes.h>
			#endif
			#ifndef __EVENTS__
			#include <Events.h>
			#endif
			#ifndef __MENUS__
			#include <Menus.h>
			#endif
			#ifndef __OPENTRANSPORT__
			#include <OpenTransport.h>
			#endif
			#ifndef __OPENTRANSPORTPROVIDERS__
			#include <OpenTransportProviders.h>
			#endif

		#define FATAL_EXIT			exit(-1)
	
		/* Platform independent data references */
		typedef	EventRecord			NMEvent;
		typedef	Rect				NMRect;
		#if TARGET_API_MAC_CARBON
		typedef MenuRef				NMMenuRef;
		typedef	WindowRef			NMWindowRef;
		typedef	DialogRef			NMDialogPtr;
		#else
		typedef MenuHandle			NMMenuRef;
		typedef	WindowPtr			NMWindowRef;
		typedef	DialogPtr			NMDialogPtr;
		#endif

		typedef	NumVersion			NMNumVersion;

		#define OP_CALLBACK_API		CALLBACK_API
		#define OP_DEFINE_API_C		DEFINE_API_C
		
	#elif defined(OP_PLATFORM_WINDOWS)
	
		#define WIN32_LEAN_AND_MEAN /* We limit the import of certain libs... */
	
		#include <Windows.h>
	
		/* Platform independent data references */
		#define FATAL_EXIT        	FatalExit(0)

		typedef HWND				NMDialogPtr;
		typedef HWND				NMWindowRef;
		typedef RECT				NMRect;
		typedef HMENU				NMMenuRef;

		struct NMNumVersion {
												/* Numeric version part of 'vers' resource */
			NMUInt8 		majorRev;			/* 1st part of version number in BCD */
			NMUInt8 		minorAndBugRev;		/* 2nd & 3rd part of version number share a byte */
			NMUInt8 		stage;				/* stage code: dev, alpha, beta, final */
			NMUInt8 		nonRelRev;			/* revision level of non-released version */
		};
		typedef struct NMNumVersion	NMNumVersion;	

		typedef struct private_event
		{
			HWND			hWnd;
			UINT			msg;
			WPARAM		wParam;
			LPARAM		lParam;
		} NMEvent;

		#ifdef OPENPLAY_DLLEXPORT
			#define OP_DEFINE_API_C(_type) __declspec(dllexport) _type __cdecl
			#define OP_CALLBACK_API(_type, _name)              _type (__cdecl *_name)
		#else
			#define OP_DEFINE_API_C(_type)                     _type __cdecl
			#define OP_CALLBACK_API(_type, _name)              _type (__cdecl *_name)
		#endif /* OPENPLAY_DLLEXPORT */

	#elif defined(OP_PLATFORM_MAC_MACHO)

		#define OP_PLATFORM_MAC_CARBON_FLAG 1
		#include <Carbon/Carbon.h>

		#undef TCP_NODELAY	// defined by OT headers, need posix defs!
		#undef TCP_MAXSEG

		#define FATAL_EXIT          exit(EXIT_FAILURE)
		typedef  unsigned short  	word;

		/* Platform independent data references */
		typedef	EventRecord  		NMEvent;
		typedef	DialogPtr    		NMDialogPtr;	
		typedef	WindowRef			NMWindowRef;
		typedef	Rect         		NMRect;			
		typedef MenuRef				NMMenuRef;

		typedef	NumVersion			NMNumVersion;

		#define OP_DEFINE_API_C(_type) _type
		#define OP_CALLBACK_API(_type, _name) _type (*_name)

	#elif defined(OP_PLATFORM_UNIX)
	
		#define FATAL_EXIT          exit(EXIT_FAILURE)
		typedef	unsigned short  	word;

		typedef	void*  				NMDialogPtr;
		typedef	void*  				NMWindowRef;
		typedef void*				NMMenuRef;

		struct NMNumVersion {
												/* Numeric version part of 'vers' resource */
			NMUInt8 		majorRev;			/* 1st part of version number in BCD */
			NMUInt8 		minorAndBugRev;		/* 2nd & 3rd part of version number share a byte */
			NMUInt8 		stage;				/* stage code: dev, alpha, beta, final */
			NMUInt8 		nonRelRev;			/* revision level of non-released version */
		};
		typedef struct NMNumVersion	NMNumVersion;	

		typedef	struct _event
			{
				NMSInt32 unknown;       
		} NMEvent;
	
			typedef struct _rect
			{
				NMUInt32 top;
				NMUInt32 left;
				NMUInt32 bottom;
				NMUInt32 right;
		} NMRect;
		
		
		#define OP_DEFINE_API_C(_type) _type
		#define OP_CALLBACK_API(_type, _name) _type (*_name)
	
	#else
		#error unknown build type
	#endif 
	

		
/*-------------------------------------------------------------------------------------------
	define parameters
*/

	#ifdef has_unused_pragma
	  #define UNUSED_PARAMETER(x)  #pragma unused(x)
	#else
	  #define UNUSED_PARAMETER(x)  (void) (x);
	#endif
	
	#ifndef has_byte_type
	  #define  BYTE  unsigned char
	#endif
		
	
/** @addtogroup TypesAndConstants
 * @{
 */

	/*define all platform-dependent types once in a generalized manner for the documentation*/
	#if (DOXYGEN)
		/**Basic boolean type.*/
		typedef  unsigned char NMBoolean;
		/**32-bit unsigned integer.*/
		typedef na NMUInt32;
		/**32-bit signed integer.*/
		typedef na NMSInt32;
		/**16-bit unsigned integer.*/
		typedef na NMUInt16;
		/**16-bit signed integer.*/
		typedef na NMSInt16;
		/**8-bit unsigned integer.*/
		typedef unsigned char NMUInt8;
		/**8-bit signed integer.*/
		typedef char NMSInt8;
		/**Platform specific integer-based rect type */
		typedef struct NMRect
		{
			/**top*/
			na top;
			/**left*/
			na left;
			/**bottom*/
			na bottom;
			/**right*/
			na right;
		} NMRect;
		/**Platform specific event struct*/
		typedef struct na NMEvent;
		/**Platform specific pointer to a dialog object*/
		typedef struct na NMDialogPtr;
	#endif
		
/*-------------------------------------------------------------------------------------------
	Define Openplay types and defines
*/
	
	/**4-char code type used throughout OpenPlay.*/
	typedef NMSInt32		NMType;
	/**OpenPlay error code.*/
	typedef NMSInt32    	NMErr;
	typedef NMErr			NMOSStatus;
	/**Used for flags throughout OpenPlay.*/
	typedef NMUInt32		NMFlags;
	/**Host identification for enumeration.*/
	typedef NMUInt32 		NMHostID;

	/**Callback codes passed to an endpoint's notifier function by OpenPlay.*/
	typedef enum
	{
		/**The endpoint has received a connection request - you should call either \ref ProtocolAcceptConnection() or \ref ProtocolRejectConnection() in response, though you do not have to do so immediately.*/
		kNMConnectRequest	= 1,
		/**One or more datagram messages have arrived at the endpoint - to receive them, you can call \ref ProtocolReceivePacket(). */
		kNMDatagramData		= 2,
		/**New stream data is available to read at the endpoint - to receive data from the stream, you can call \ref ProtocolReceive(). */
		kNMStreamData		= 3,
		/**An endpoint which had been flow-blocked (unable to send more data, generally due to saturated bandwidth) is now able to send again. */
		kNMFlowClear		= 4,
		/**Sent to a passive(host) endpoint when it has successfully accepted a connection and "handed it off" as a new endpoint, which is used to communicate with the connected client */
		kNMAcceptComplete	= 5,
		/**Sent to a new endpoint spawned from a passive(host) endpoint once it has been successfully formed */
		kNMHandoffComplete	= 6,
		/**Sent when an endpoint "dies"; generally when the connection is lost or the remote machine disconnects.  You should call \ref ProtocolCloseEndpoint() on the endpoint after receiving this message, and then wait for its \ref kNMCloseComplete message.*/
		kNMEndpointDied		= 7,
		/**The final message sent to an endpoint.  After closing an endpoint, you should wait for this message to be sure that no more straggling messages will arrive at the endpoint.  Calling \ref ProtocolIsAlive() can be used with the same purpose.*/
		kNMCloseComplete	= 8,
		/**allow servers to be called back to update the response data before an enumeration response is returned to a client.*/
		kNMUpdateResponse	= 9,

		/**Define the next free callback # available (also an easy way to avoid ending , on updates) */
		kNMNextFreeCallbackCode

	} NMCallbackCode;
	
	/**Special values for a \ref NMModuleInfoStruct 's maxEndpoints member.*/
	typedef enum
	{
		/**Any number of endpoints of this type may be created.*/
		kNMNoEndpointLimit	= 0xFFFFFFFF,
		/**This module is not able to host (e.g. an AOL module).*/
		kNMNoPassiveEndpoints	= 0
	} NMMaxEndpointsValue;
	
	/**Max lengths for various string types.*/
	typedef enum
	{
		/**Maximum length for a \ref NMModuleInfoStruct 's name.*/
		kNMNameLength	= 32,
		/**Maximum length for a \ref NMModuleInfoStruct 's copyright string.*/
		kNMCopyrightLen	= 32
	} NMMaxStringLength;
		
	/**A \ref NMModuleInfoStruct 's flags member may contain one or more of the following values.*/
	typedef enum
	{
		/**This NetModule can create endpoints with stream functionality.*/
		kNMModuleHasStream			= 0x00000001,
		/**This NetModule can create endpoints with datagram(packet) functionality.*/
		kNMModuleHasDatagram		= 0x00000002,
		/**???*/
		kNMModuleHasExpedited		= 0x00000004,
		/**For this NetModule to function correctly, \ref ProtocolIdle() must be called regularly.*/
		kNMModuleRequiresIdleTime	= 0x00000008
	} NMNetModuleFlag;
	
	/**Struct describing an OpenPlay NetModule in detail - used with \ref ProtocolGetEndpointInfo().*/
	struct NMModuleInfoStruct
	{
		/**Initialize this to the size of the struct*/
		NMUInt32	size;
		/**The NetModule's unique type.*/
		NMType		type;
		/**The NetModule's full name.*/
		char		name[kNMNameLength];
		/**The NetModule's copyright string.*/
		char		copyright[kNMCopyrightLen];
		/**The maximum size packets this NetModule can send.*/
		NMUInt32	maxPacketSize;
		/**The maximum number of concurrent endpoints this NetModule supports - may be set to the special values \ref kNMNoEndpointLimit or \ref kNMNoPassiveEndpoints.*/
		NMUInt32	maxEndpoints;
		/**Flags.*/
		NMFlags		flags;
	};
	typedef struct NMModuleInfoStruct NMModuleInfo;

	/**Define which types of address we are requesting in \Ref ProtocolGetAddress() */
	typedef enum
	{
		/** Get an IP Address string (ie, 127.0.0.1:128) */
		kNMIPAddressType,
		/**Get an OT Address of type AF_INET, AF_DNS, etc. */
		kNMOTAddressType

	}NMAddressType;

	/**Commands sent to the \ref NMEnumerationCallbackPtr function passed to \ref ProtocolStartEnumeration(); */
	typedef enum
	{
		/**The accompanying \ref NMEnumItemStruct denotes a new host has been found on the network.*/
		kNMEnumAdd	= 1,
		/**The accompanying \ref NMEnumItemStruct denotes a host that is no longer available on the network.*/
		kNMEnumDelete,
		/**The list of available hosts should be cleared.*/
		kNMEnumClear
	} NMEnumerationCommand;
	
	typedef enum
	{
		/**Neither packet nor stream functionality.*/
		kNMModeNone = 0,
		/**Packet functionality.*/
		kNMDatagramMode = 1,
		/**Stream functionality.*/
		kNMStreamMode = 2,
		/**Both packet and stream functionality.*/
		kNMNormalMode = kNMStreamMode + kNMDatagramMode
	} NMEndpointMode;
	
	/**Struct which is filled out for you by OpenPlay when enumerating hosts on the network.*/
	struct NMEnumItemStruct
	{
		/**Id of the host.  This value is passed to \ref ProtocolBindEnumerationToConfig() to set a \ref PConfigRef to the address of given host.*/
		NMHostID	id;
		/**Name of the NetModule.*/
		char 		*name;
		/**Length of the custom enumeration data which was passed to \ref ProtocolCreateConfig() when creating the \ref PConfigRef used for this emumeration.*/
		NMUInt32	customEnumDataLen;
		/**Custom enumeration data which was passed to \ref ProtocolCreateConfig() when creating the \ref PConfigRef used for this enumeration.*/
		void		*customEnumData;
	};
	typedef struct NMEnumItemStruct NMEnumerationItem;
	
	/**Function pointer type passed to \ref ProtocolStartEnumeration() to be called back by OpenPlay when new hosts appear,disappear,etc until \ref ProtocolEndEnumeration() is called to end enumeration.*/
	typedef void (*NMEnumerationCallbackPtr)(
									void 				*	inContext,
									NMEnumerationCommand	inCommand, 
									NMEnumerationItem		*item);
	
	
	/**Flags that can be passed to endpoint functions.*/
	typedef enum
	{
		/**Specifies the function is not to return until the operation is complete - waiting if necessary for data to come in, etc.  OpenPlay by default is a non-blocking system*/
		kNMBlocking	= 0x00000001
	}NMDataTranferFlag;
	
	/**OpenPlay Error Codes */
	typedef enum
	{
		/**OpenPlay has run out of memory. Ouch.*/
		kNMOutOfMemoryErr			= -4999,
		/**Invalid parameter passed.*/
		kNMParameterErr				= -4998,
		/**Operation timed out before completing.*/
		kNMTimeoutErr				= -4997,
		/**An invalid or corrupt PConfigRef was passed.*/
		kNMInvalidConfigErr			= -4996,
		/**Version is too new*/
		kNMNewerVersionErr			= -4995,
		/**The endpoint could not be opened.*/
		kNMOpenFailedErr			= -4994,
		/**The host rejected the connection attempt.*/
		kNMAcceptFailedErr			= -4993,
		/**The provided config string is not large enough.*/
		kNMConfigStringTooSmallErr	= -4992,
		/**A required resource could not be found/loaded.*/
		kNMResourceErr				= -4991,
		/**The provided \ref PConfigRef is already enumerating.*/
		kNMAlreadyEnumeratingErr	= -4990,
		/**The provided \ref PConfigRef is not enumerating.*/
		kNMNotEnumeratingErr		= -4989,
		/**The provided \ref PConfigRef is unable to enumerate.*/
		kNMEnumerationFailedErr		= -4988,
		/**There is no more data to be retrieved. This is normal.*/
		kNMNoDataErr				= -4987,
		/**The protocol was not able to init.  It may not be able to run on the system.*/
		kNMProtocolInitFailedErr	= -4985,
		/**An internal error occurred.  Uh oh.*/
		kNMInternalErr				= -4984,
		/**?? - this may only be internal.*/
		kNMMoreDataErr				= -4983,
		/**The provided selector is not recognized by the item's pass-through function.*/
		kNMUnknownPassThrough		= -4982,
		/**Provided address was not found.*/
		kNMAddressNotFound			= -4981,
		/**Provided address not yet valid.*/
		kNMAddressNotValidYet		= -4980,
		/**The endpoint is not in the proper mode.  You may have tried to receive stream data from an endpoint with only packet functionality, etc.*/
		kNMWrongModeErr				= -4979,
		/**The endpoint is in a bad state.*/
		kNMBadStateErr				= -4978,
		/**Too much data provided.*/
		kNMTooMuchDataErr			= -4977,
		/**Can't block in the current state.*/
		kNMCantBlockErr				= -4976,
		/**The endpoint is currently flow-blocked. (clogged)  You will be notified with a \ref kNMFlowClear message when it is clear, or you can keep sending until you get no error.*/
		kNMFlowErr					= -4974,
		/**An error occurred within the protocol during the operation.*/
		kNMProtocolErr				= -4793,
		/**The provided \ref NMModuleInfo is too small*/
		kNMModuleInfoTooSmall		= -4792,
		/**Internal use only... a carbon module was found on OSX that only functions in classic (such as appletalk).*/
		kNMClassicOnlyModuleErr		= -4791,	
		/**??*/
		kNMBadPacketDefinitionErr		= -4790,
		/**??*/
		kNMBadShortAlignmentErr			= -4789,
		/**??*/
		kNMBadLongAlignmentErr			= -4788,
		/**??*/
		kNMBadPacketDefinitionSizeErr	= -4787,
		/**The NetModule was not of a compatible or legal version.*/
		kNMBadVersionErr				= -4786,
		/**There is no NetModule with given index.*/
		kNMNoMoreNetModulesErr			= -4785,
		/**Given NetModule was not found.*/
		kNMModuleNotFoundErr			= -4784,
		/**The function does not seem to exist in the NetModule.*/
		kNMFunctionNotBoundErr			= -4783,
/*LR -- should no longer be used!
		//NULL pointer encountered.
		err_NilPointer					= -4782,	// was 'nilP'
		kNMNilPointer					= err_NilPointer,
		//Assertion failed.
		err_AssertFailed				= -4781,	// was 'asrt';
		kNMAssertFailed				= err_AssertFailed,
*/
		/**No error! woohoo!*/
		kNMNoError					= 0
	} NMErrorCode;
	
/*	------------------------------	Public Definitions */

	
	/**When creating a NetModule on mac (classic/carbon versions), set its creator code to this.*/
	#define NET_MODULE_CREATOR			'OPLY'
	/**Before passing a NMProtocol to GetIndexedProtocol(), initialize its \ref version member to this.*/
	#define CURRENT_OPENPLAY_VERSION	(1)
	
/*	------------------------------	Public Types */

	
	typedef NMSInt16 _packet_data_size;
	
	typedef enum
	{
		/**Two! Two bytes! ... ah ah ah ah!*/
		k2Byte	= -1,
		/**Four! Four bytes! ... ah ah ah ah!*/
		k4Byte	= -2
	} NMByteCount;
	
	/**Struct describing a specific OpenPlay protocol, obtained from \ref GetIndexedProtocol().*/
	struct NMProtocolStruct
	{
		/**Must be initialized to \ref CURRENT_OPENPLAY_VERSION before using the struct.*/
		NMSInt32	version;
		/**The protocol's unique type identifier.*/
		NMType		type;
		/**The protocol's name (a c string).*/
		char		name[kNMNameLength];
	};
	typedef struct NMProtocolStruct NMProtocol;
		
	/**Opaque reference to an OpenPlay Endpoint. All data sending/receiving in OpenPlay occurs through Endpoints.*/
	typedef struct Endpoint *PEndpointRef;
	
	/**Opaque reference to a protocol configuration.  Stores the NetModule type, game name, etc, as well as any custom settings for the protocol (addresses, port numbers, etc). Required in order to create a \ref PEndpointRef.*/
	typedef struct ProtocolConfig *PConfigRef;
	
	/**Function pointer type passed to \ref ProtocolOpenEndpoint() or ProtocolAcceptConnection() to be called back by OpenPlay when events occur such as data arrival.*/	
	typedef void (*PEndpointCallbackFunction)(
								PEndpointRef	 		inEndpoint, 
								void 					*inContext,
								NMCallbackCode			inCode, 
								NMErr		 			inError, 
								void 					*inCookie);
	
	/**Flags passed when creating an endpoint.*/
	typedef enum
	{
		/**Blank value - passed alone will open a passive(host) endpoint.*/
		kOpenNone	= 0x00,
		/**Create an active (client) endpoint.  Without this flag, a passive(host) endpoint is created.*/
		kOpenActive	= 0x01
	} NMOpenFlags;

	/**A few established NetModule types.*/
	typedef enum
	{
		/**AppleTalk NetModule type.*/
		kATModuleType = 'Atlk',
		/**TCPIP NetModule type.*/
		kIPModuleType = 'Inet'
	} NMEstablishedModuleTypes;
	
/** @}*/	
/*	------------------------------	Public Variables */

/*	------------------------------	Public Functions */	
	
/*-------------------------------------------------------------------------------------------
	Define Netsprocket types and defines
*/

	#define kNSpStr32Len 32
		
	enum {
		kNSpMaxPlayerNameLen		= 31,
		kNSpMaxGroupNameLen			= 31,
		kNSpMaxPasswordLen			= 31,
		kNSpMaxGameNameLen			= 31,
		kNSpMaxDefinitionStringLen	= 255
	};
	
	/* NetSprocket basic types */
	typedef struct OpaqueNSpGameReference* 			NSpGameReference;
	typedef struct OpaqueNSpProtocolReference*		NSpProtocolReference;
	typedef struct OpaqueNSpProtocolListReference*  NSpProtocolListReference;
	typedef struct OpaqueNSpAddressReference*		NSpAddressReference;

	typedef NMSInt32 						NSpEventCode;
	typedef NMSInt32 						NSpGameID;
	typedef NMSInt32 						NSpPlayerID;
	typedef NSpPlayerID 					NSpGroupID;
	typedef NMUInt32 						NSpPlayerType;
	typedef NMSInt32 						NSpFlags;
	typedef unsigned char					NSpPlayerName[kNSpStr32Len];

	/* Individual player info */	

	struct NSpPlayerInfo {
		NSpPlayerID 						id;
		NSpPlayerType 						type;
		unsigned char 						name[kNSpStr32Len];
		NMUInt32 							groupCount;
		NSpGroupID 							groups[1];
	};
	typedef struct NSpPlayerInfo NSpPlayerInfo;

	typedef NSpPlayerInfo *					NSpPlayerInfoPtr;

	/* list of all players */
	
	struct NSpPlayerEnumeration {
		NMUInt32 							count;
		NSpPlayerInfoPtr 					playerInfo[1];
	};
	typedef struct NSpPlayerEnumeration		NSpPlayerEnumeration;
	typedef NSpPlayerEnumeration *			NSpPlayerEnumerationPtr;

	/* Individual group info */
	
	struct NSpGroupInfo {
		NSpGroupID 							id;
		NMUInt32 							playerCount;
		NSpPlayerID 						players[1];
	};
	typedef struct NSpGroupInfo				NSpGroupInfo;
	typedef NSpGroupInfo *					NSpGroupInfoPtr;

	/* List of all groups */
	
	struct NSpGroupEnumeration {
		NMUInt32 							count;
		NSpGroupInfoPtr 					groups[1];
	};
	typedef struct NSpGroupEnumeration		NSpGroupEnumeration;
	typedef NSpGroupEnumeration *			NSpGroupEnumerationPtr;

	/* Topology types */
	
	typedef NMUInt32 						NSpTopology;
	enum {
		kNSpClientServer					= 0x00000001
	};
	
	/* Game information */

	struct NSpGameInfo {
		NMUInt32 							maxPlayers;
		NMUInt32 							currentPlayers;
		NMUInt32 							currentGroups;
		NSpTopology 						topology;
		NMUInt32 							reserved;
		unsigned char 						name[kNSpStr32Len];
		unsigned char 						password[kNSpStr32Len];
	};
	typedef struct NSpGameInfo				NSpGameInfo;
	
	/* Structure used for sending and receiving network messages */

	struct NSpMessageHeader {
		NMUInt32 							version;		/* Used by NetSprocket.  Don't touch this */
		NMSInt32 							what;			/* The kind of message (e.g. player joined) */
		NSpPlayerID 						from;			/* ID of the sender */
		NSpPlayerID 						to;				/* (player or group) id of the intended recipient */
		NMUInt32 							id;				/* Unique ID for this message & (from) player */
		NMUInt32 							when;			/* Timestamp for the message */
		NMUInt32 							messageLen;		/* Bytes of data in the entire message (including the header) */
	};
	typedef struct NSpMessageHeader NSpMessageHeader;

	/* NetSprocket-defined message structures */

	struct NSpErrorMessage {
		NSpMessageHeader 				header;
		NMErr	 						error;
	};

	typedef struct NSpErrorMessage		NSpErrorMessage;
	
	struct NSpJoinRequestMessage {
		NSpMessageHeader 				header;
		unsigned char 					name[kNSpStr32Len];
		unsigned char 					password[kNSpStr32Len];
		NMUInt32 						theType;
		NMUInt32 						customDataLen;
		NMUInt8 						customData[1];
	};

	typedef struct NSpJoinRequestMessage	NSpJoinRequestMessage;
	
	struct NSpJoinApprovedMessage {
		NSpMessageHeader 				header;
	};
	typedef struct NSpJoinApprovedMessage	NSpJoinApprovedMessage;
	
	struct NSpJoinDeniedMessage {
		NSpMessageHeader 				header;
		unsigned char 					reason[256];
	};
	typedef struct NSpJoinDeniedMessage		NSpJoinDeniedMessage;
	
	struct NSpPlayerJoinedMessage {
		NSpMessageHeader 				header;
		NMUInt32 						playerCount;
		NSpPlayerInfo 					playerInfo;
	};
	typedef struct NSpPlayerJoinedMessage	NSpPlayerJoinedMessage;
	
	struct NSpPlayerLeftMessage {
		NSpMessageHeader 				header;
		NMUInt32 						playerCount;
		NSpPlayerID 					playerID;
		NSpPlayerName 					playerName;
	};
	typedef struct NSpPlayerLeftMessage		NSpPlayerLeftMessage;
	
	struct NSpHostChangedMessage {
		NSpMessageHeader 				header;
		NSpPlayerID 					newHost;
	};
	typedef struct NSpHostChangedMessage	NSpHostChangedMessage;
	
	struct NSpGameTerminatedMessage {
		NSpMessageHeader 				header;
	};
	typedef struct NSpGameTerminatedMessage	NSpGameTerminatedMessage;
	
	struct NSpCreateGroupMessage {
		NSpMessageHeader 				header;
		NSpGroupID 						groupID;
		NSpPlayerID 					requestingPlayer;
	};
	typedef struct NSpCreateGroupMessage	NSpCreateGroupMessage;
	
	struct NSpDeleteGroupMessage {
		NSpMessageHeader 				header;
		NSpGroupID 						groupID;
		NSpPlayerID 					requestingPlayer;
	};
	typedef struct NSpDeleteGroupMessage	NSpDeleteGroupMessage;
	
	struct NSpAddPlayerToGroupMessage {
		NSpMessageHeader 				header;
		NSpGroupID 						group;
		NSpPlayerID 					player;
	};
	typedef struct NSpAddPlayerToGroupMessage NSpAddPlayerToGroupMessage;
	
	struct NSpRemovePlayerFromGroupMessage {
		NSpMessageHeader 				header;
		NSpGroupID 						group;
		NSpPlayerID 					player;
	};
	typedef struct NSpRemovePlayerFromGroupMessage NSpRemovePlayerFromGroupMessage;
	
	struct NSpPlayerTypeChangedMessage {
		NSpMessageHeader 				header;
		NSpPlayerID 					player;
		NSpPlayerType 					newType;
	};
	typedef struct NSpPlayerTypeChangedMessage NSpPlayerTypeChangedMessage;

	/* Different kinds of messages.  These can NOT be bitwise ORed together */

	enum {
		kNSpSendFlag_Junk			= 0x00100000,					/* will be sent (try once) when there is nothing else pending */
		kNSpSendFlag_Normal			= 0x00200000,					/* will be sent immediately (try once) */
		kNSpSendFlag_Registered		= 0x00400000					/* will be sent immediately (guaranteed, in order) */
	};
	
	
	/* Options for message delivery.  These can be bitwise ORed together with each other,
			as well as with ONE of the above */
	enum {
		kNSpSendFlag_FailIfPipeFull	= 0x00000001,
		kNSpSendFlag_SelfSend		= 0x00000002,
		kNSpSendFlag_Blocking		= 0x00000004
	};
	
	
	/* Options for Hosting Joining, and Deleting games */
	enum {
		kNSpGameFlag_DontAdvertise		= 0x00000001,
		kNSpGameFlag_ForceTerminateGame = 0x00000002
	};
	
	/* Message "what" types */
	/* Apple reserves all negative "what" values (anything with high bit set) */
	enum {
		kNSpSystemMessagePrefix		= (NMSInt32)0x80000000,

		kNSpJoinRequest				= kNSpSystemMessagePrefix | 0x00000001,
		kNSpJoinApproved			= kNSpSystemMessagePrefix | 0x00000002,
		kNSpJoinDenied				= kNSpSystemMessagePrefix | 0x00000003,
		kNSpPlayerJoined			= kNSpSystemMessagePrefix | 0x00000004,
		kNSpPlayerLeft				= kNSpSystemMessagePrefix | 0x00000005,
		kNSpHostChanged				= kNSpSystemMessagePrefix | 0x00000006,
		kNSpGameTerminated			= kNSpSystemMessagePrefix | 0x00000007,
		kNSpGroupCreated			= kNSpSystemMessagePrefix | 0x00000008,
		kNSpGroupDeleted			= kNSpSystemMessagePrefix | 0x00000009,
		kNSpPlayerAddedToGroup		= kNSpSystemMessagePrefix | 0x0000000A,
		kNSpPlayerRemovedFromGroup	= kNSpSystemMessagePrefix | 0x0000000B,
		kNSpPlayerTypeChanged		= kNSpSystemMessagePrefix | 0x0000000C,
		kNSpJoinResponse			= kNSpSystemMessagePrefix | 0x0000000D,

		kNSpError					= kNSpSystemMessagePrefix | 0x7FFFFFFF
	};

	// dair, Added join response message
	#define kNSpMaxJoinResponseLen	1280	// Buffer is a set size due to creation at interrupt time

	typedef struct {
		NSpMessageHeader 				header;
		NMUInt32 						responseDataLen;
		NMUInt8 						responseData[kNSpMaxJoinResponseLen];
	} NSpJoinResponseMessage;

	/** Special TPlayerIDs  for sending messages */
	typedef enum
	{
		/**Send message to all connected players (host is not a player!) */
		kNSpAllPlayers				= 0,
		/**Used to send a message to the NSp game's host */
		kNSpHostID					= 1,
		/**For use in a headless server setup */
		kNSpMasterEndpointID		= -1		/* was kNSpHostOnly */
	}NSpPlayerSpecials;
	
	#ifndef __MACTYPES__
	
		/* NetSprocket Error Codes */
		enum {
			kNSpInitializationFailedErr	= -30360,
			kNSpAlreadyInitializedErr	= -30361,
			kNSpTopologyNotSupportedErr	= -30362,
			kNSpPipeFullErr				= -30364,
			kNSpHostFailedErr			= -30365,
			kNSpProtocolNotAvailableErr	= -30366,
			kNSpInvalidGameRefErr		= -30367,
			kNSpInvalidParameterErr		= -30369,
			kNSpOTNotPresentErr			= -30370,
			kNSpOTVersionTooOldErr		= -30371,
			kNSpMemAllocationErr		= -30373,
			kNSpAlreadyAdvertisingErr	= -30374,
			kNSpNotAdvertisingErr		= -30376,
			kNSpInvalidAddressErr		= -30377,
			kNSpFreeQExhaustedErr		= -30378,
			kNSpRemovePlayerFailedErr	= -30379,
			kNSpAddressInUseErr			= -30380,
			kNSpFeatureNotImplementedErr = -30381,
			kNSpNameRequiredErr			= -30382,
			kNSpInvalidPlayerIDErr		= -30383,
			kNSpInvalidGroupIDErr		= -30384,
			kNSpNoPlayersErr			= -30385,
			kNSpNoGroupsErr				= -30386,
			kNSpNoHostVolunteersErr		= -30387,
			kNSpCreateGroupFailedErr	= -30388,
			kNSpAddPlayerFailedErr		= -30389,
			kNSpInvalidDefinitionErr	= -30390,
			kNSpInvalidProtocolRefErr	= -30391,
			kNSpInvalidProtocolListErr	= -30392,
			kNSpTimeoutErr				= -30393,
			kNSpGameTerminatedErr		= -30394,
			kNSpConnectFailedErr		= -30395,
			kNSpSendFailedErr			= -30396,
			kNSpMessageTooBigErr		= -30397,
			kNSpCantBlockErr			= -30398,
			kNSpJoinFailedErr			= -30399
		};
		
	#endif	/*	__MACTYPES__	*/
	
	
#ifdef __cplusplus
extern "C" {
#endif

	#if PRAGMA_IMPORT
	#pragma import on
	#endif

	#if PRAGMA_STRUCT_ALIGN
		#pragma options align=power
	#elif PRAGMA_STRUCT_PACKPUSH
		#pragma pack(push, 2)
	#elif PRAGMA_STRUCT_PACK
		#pragma pack(2)
	#endif

	
	
/*-------------------------------------------------------------------------------------------
	OPENPLAY API
*/
	/* ----------- Protocol Management Layer */
		
		OP_DEFINE_API_C(NMErr) 
		GetIndexedProtocol			(		NMSInt16 		index, 
											NMProtocol *	protocol);
		
	/* ----------- config functions */
	
		OP_DEFINE_API_C(NMErr) 
		ProtocolCreateConfig			(	NMType 			type, 
											NMType 			game_id, 
											const char *	game_name, 
											const void *	enum_data, 
											NMUInt32 		enum_data_len, 
											char *			configuration_string, 
											PConfigRef *	inConfig);
													
		OP_DEFINE_API_C(NMErr)
		ProtocolDisposeConfig			(	PConfigRef config);

		OP_DEFINE_API_C(NMType) 
		ProtocolGetConfigType			(	PConfigRef config );

		OP_DEFINE_API_C(NMErr) 
		ProtocolGetConfigString			(	PConfigRef config, 
											char *config_string, 
											NMSInt16 max_length);
									
		OP_DEFINE_API_C(NMErr) 
		ProtocolGetConfigStringLen		(	PConfigRef		config, 
											NMSInt16 *		length);
										
		OP_DEFINE_API_C(NMErr) 
		ProtocolConfigPassThrough		(	PConfigRef 	config, 
											NMUInt32 inSelector, 
											void *inParamBlock);
	
	/* ----------- enumeration functions */
	
		OP_DEFINE_API_C(NMErr) 
		ProtocolStartEnumeration		(	PConfigRef 					config, 
											NMEnumerationCallbackPtr	inCallback,
											void *						inContext,
											NMBoolean 					inActive);
										
		OP_DEFINE_API_C(NMErr) 
		ProtocolIdleEnumeration			(	PConfigRef config);

		OP_DEFINE_API_C(NMErr)
		ProtocolEndEnumeration			(	PConfigRef config);
		
		OP_DEFINE_API_C(NMErr) 
		ProtocolBindEnumerationToConfig	(	PConfigRef 	config, 
											NMHostID 	inID);		
		OP_DEFINE_API_C(void) 
		ProtocolStartAdvertising		(	PEndpointRef endpoint);

		OP_DEFINE_API_C(void) 
		ProtocolStopAdvertising			(	PEndpointRef endpoint);
	
	/* ---------- opening/closing */
	
		OP_DEFINE_API_C(NMErr) 
		ProtocolOpenEndpoint			(	PConfigRef inConfig,
											PEndpointCallbackFunction inCallback,
											void *inContext, 
											PEndpointRef *outEndpoint, 
											NMOpenFlags flags);
									
		OP_DEFINE_API_C(NMErr) 
		ProtocolCloseEndpoint			(	PEndpointRef endpoint, 
											NMBoolean inOrderly);
	
	/* ----------- options */
	
		OP_DEFINE_API_C(NMErr) 
		ProtocolSetTimeout				(	PEndpointRef endpoint, 
											NMSInt32 timeout);
								
		OP_DEFINE_API_C(NMBoolean) 
		ProtocolIsAlive					(	PEndpointRef endpoint);
		
		OP_DEFINE_API_C(NMErr) 
		ProtocolIdle					(	PEndpointRef endpoint);
		
		OP_DEFINE_API_C(NMErr) 
		ProtocolFunctionPassThrough		(	PEndpointRef endpoint, 
											NMUInt32 inSelector,
											void *inParamBlock);
										
		OP_DEFINE_API_C(NMErr) 
		ProtocolSetEndpointContext		(	PEndpointRef endpoint, 
											void *newContext);
	
		OP_DEFINE_API_C(NMErr)
		ProtocolGetEndpointIdentifier ( PEndpointRef endpoint,
										 char *identifier_string, 
										 NMSInt16 max_length);

	/* ----------- connections */
	
		OP_DEFINE_API_C(NMErr) 
		ProtocolAcceptConnection		(	PEndpointRef endpoint, 
											void *inCookie, 
											PEndpointCallbackFunction inNewCallback,
											void *inNewContext);
		
		OP_DEFINE_API_C(NMErr) 
		ProtocolRejectConnection		(	PEndpointRef endpoint, 
											void *inCookie);
	
	/* ----------- sending/receiving */	
	
		OP_DEFINE_API_C(NMErr) 
		ProtocolSendPacket				(	PEndpointRef endpoint, 
											void *inData, 
											NMUInt32 inLength, 
											NMFlags inFlags);
										
		OP_DEFINE_API_C(NMErr) 
		ProtocolReceivePacket			(	PEndpointRef endpoint, 
											void *outData, 
											NMUInt32 *ioSize, 
											NMFlags *outFlags);
	
	/* ------------ entering/leaving notifiers */

		OP_DEFINE_API_C(NMErr) 
		ProtocolEnterNotifier			(	PEndpointRef endpoint,
											NMEndpointMode endpointMode);

		OP_DEFINE_API_C(NMErr)
		ProtocolLeaveNotifier			(	PEndpointRef endpoint,
											NMEndpointMode endpointMode);


	/* ----------- streaming */
	
		OP_DEFINE_API_C(NMSInt32) 
		ProtocolSend					(	PEndpointRef endpoint, 
											void *inData, 
											NMUInt32 inSize, 
											NMFlags inFlags);
												
		OP_DEFINE_API_C(NMErr) 
		ProtocolReceive					(	PEndpointRef endpoint, 
											void *outData, 
											NMUInt32 *ioSize, 
											NMFlags *outFlags);
	
	/* ----------- information functions */
	
		OP_DEFINE_API_C(NMErr) 
		ProtocolGetEndpointInfo			(	PEndpointRef endpoint, 
											NMModuleInfo *info);

		OP_DEFINE_API_C(NMErr)
		ProtocolGetEndpointAddress		(	PEndpointRef endpoint,
											NMAddressType addressType,
											void **outAddress);

		OP_DEFINE_API_C(NMErr)
		ProtocolFreeEndpointAddress		(	PEndpointRef endpoint,
											void **outAddress);

	/* ----------- miscellaneous */
	
		OP_DEFINE_API_C(NMErr)
		ValidateCrossPlatformPacket		(	_packet_data_size *		packet_definition, 
											NMSInt16 				packet_definition_length, 
											NMSInt16 				packet_length);
		
		OP_DEFINE_API_C(NMErr)
		SwapCrossPlatformPacket			(	_packet_data_size *	packet_definition, 
											NMSInt16 			packet_definition_length, 
											void *				packet_data, 
											NMSInt16 			packet_length);
	
	
	/* ----------- dialog functions */
		OP_DEFINE_API_C(NMErr) 
		ProtocolSetupDialog				(	NMDialogPtr dialog, 
											NMSInt16 frame, 
											NMSInt16 inBaseItem, 
											PConfigRef config);
								
		OP_DEFINE_API_C(NMBoolean) 
		ProtocolHandleEvent				(	NMDialogPtr dialog,
											NMEvent *	event,
											PConfigRef	config);
								
		OP_DEFINE_API_C(NMErr) 
		ProtocolHandleItemHit			(	NMDialogPtr dialog, 
											NMSInt16	inItemHit,
											PConfigRef	config);
									
		OP_DEFINE_API_C(NMBoolean) 
		ProtocolDialogTeardown			(	NMDialogPtr dialog, 
											NMBoolean 	update_config,
											PConfigRef 	config);
									
		OP_DEFINE_API_C(void) 
		ProtocolGetRequiredDialogFrame	(	NMRect *	r, 
											PConfigRef	config);
	
	
/*-------------------------------------------------------------------------------------------
	NETSPROCKET API
*/
	
	/************************  Initialization  ************************/
	OP_DEFINE_API_C( NMErr )
	NSpInitialize					(NMUInt32 				inStandardMessageSize,
									 NMUInt32 				inBufferSize,
									 NMUInt32 				inQElements,
									 NSpGameID 				inGameID,
									 NMUInt32 				inTimeout);
	
	
	/**************************  Protocols  **************************/
	/* Programmatic protocol routines */
	OP_DEFINE_API_C( NMErr )
	NSpProtocol_New					(const char *			inDefinitionString,
									 NSpProtocolReference *	outReference);
	
	OP_DEFINE_API_C( void )
	NSpProtocol_Dispose				(NSpProtocolReference 	inProtocolRef);
	
	OP_DEFINE_API_C( NMErr )
	NSpProtocol_ExtractDefinitionString (NSpProtocolReference  inProtocolRef,
										 char *					outDefinitionString);
	
	
	/* Protocol list routines */
	OP_DEFINE_API_C( NMErr )
	NSpProtocolList_New				(NSpProtocolReference 	inProtocolRef,
									 NSpProtocolListReference * outList);
	
	OP_DEFINE_API_C( void )
	NSpProtocolList_Dispose			(NSpProtocolListReference  inProtocolList);
	
	OP_DEFINE_API_C( NMErr )
	NSpProtocolList_Append			(NSpProtocolListReference  inProtocolList,
									 NSpProtocolReference 	inProtocolRef);
	
	OP_DEFINE_API_C( NMErr )
	NSpProtocolList_Remove			(NSpProtocolListReference  inProtocolList,
									 NSpProtocolReference 	inProtocolRef);
	
	OP_DEFINE_API_C( NMErr )
	NSpProtocolList_RemoveIndexed	(NSpProtocolListReference  inProtocolList,
									 NMUInt32 				inIndex);
	
	OP_DEFINE_API_C( NMUInt32 )
	NSpProtocolList_GetCount		(NSpProtocolListReference  inProtocolList);
	
	OP_DEFINE_API_C( NSpProtocolReference )
	NSpProtocolList_GetIndexedRef	(NSpProtocolListReference  inProtocolList,
									 NMUInt32 				inIndex);
	
	
	/* Helpers */
	OP_DEFINE_API_C( NSpProtocolReference )
	NSpProtocol_CreateAppleTalk		(const unsigned char 	inNBPName[kNSpStr32Len],
									 const unsigned char 	inNBPType[kNSpStr32Len],
									 NMUInt32 				inMaxRTT,
									 NMUInt32 				inMinThruput);
	
	OP_DEFINE_API_C( NSpProtocolReference )
	NSpProtocol_CreateIP			(NMUInt16 				inPort,
									 NMUInt32 				inMaxRTT,
									 NMUInt32 				inMinThruput);
	
	
	/***********************  Human Interface  ************************/
	
	typedef OP_CALLBACK_API( NMBoolean , NSpEventProcPtr )(NMEvent *inEvent);
	
	OP_DEFINE_API_C( NSpAddressReference )
	NSpDoModalJoinDialog			(const unsigned char 	inGameType[kNSpStr32Len],
									 const unsigned char 	inEntityListLabel[256],
									 unsigned char 			ioName[kNSpStr32Len],
									 unsigned char 			ioPassword[kNSpStr32Len],
									 NSpEventProcPtr 		inEventProcPtr);
	
	OP_DEFINE_API_C( NMBoolean )
	NSpDoModalHostDialog			(NSpProtocolListReference	ioProtocolList,
									 unsigned char 				ioGameName[kNSpStr32Len],
									 unsigned char 				ioPlayerName[kNSpStr32Len],
									 unsigned char 				ioPassword[kNSpStr32Len],
									 NSpEventProcPtr 			inEventProcPtr);
		
	/*********************  Hosting and Joining  **********************/
	OP_DEFINE_API_C( NMErr )
	NSpGame_Host					(NSpGameReference *		outGame,
									 NSpProtocolListReference  inProtocolList,
									 NMUInt32 				inMaxPlayers,
									 const unsigned char 	inGameName[kNSpStr32Len],
									 const unsigned char 	inPassword[kNSpStr32Len],
									 const unsigned char 	inPlayerName[kNSpStr32Len],
									 NSpPlayerType 			inPlayerType,
									 NSpTopology 			inTopology,
									 NSpFlags 				inFlags);
	
	OP_DEFINE_API_C( NMErr )
	NSpGame_Join					(NSpGameReference *		outGame,
									 NSpAddressReference 	inAddress,
									 const unsigned char 	inName[kNSpStr32Len],
									 const unsigned char 	inPassword[kNSpStr32Len],
									 NSpPlayerType 			inType,
									 void *					inCustomData,
									 NMUInt32 				inCustomDataLen,
									 NSpFlags 				inFlags);
	
	OP_DEFINE_API_C( NMErr )
	NSpGame_EnableAdvertising		(NSpGameReference 		inGame,
									 NSpProtocolReference 	inProtocol,
									 NMBoolean 				inEnable);
	
	OP_DEFINE_API_C( NMErr )
	NSpGame_Dispose					(NSpGameReference 		inGame,
									 NSpFlags 				inFlags);
	
	OP_DEFINE_API_C( NMErr )
	NSpGame_GetInfo					(NSpGameReference 		inGame,
									 NSpGameInfo *			ioInfo);
	
	/**************************  Messaging  **************************/
	OP_DEFINE_API_C( NMErr )
	NSpMessage_Send					(NSpGameReference 		inGame,
									 NSpMessageHeader *		inMessage,
									 NSpFlags 				inFlags);
	
	OP_DEFINE_API_C( NSpMessageHeader *)
	NSpMessage_Get					(NSpGameReference 		inGame);
	
	OP_DEFINE_API_C( NMUInt32 )
	NSpMessage_GetBacklog( NSpGameReference inGame );

	OP_DEFINE_API_C( void )
	NSpMessage_Release				(NSpGameReference 		inGame,
									 NSpMessageHeader *		inMessage);
	
	/* Helpers */
	OP_DEFINE_API_C( NMErr )
	NSpMessage_SendTo				(NSpGameReference 		inGame,
									 NSpPlayerID 			inTo,
									 NMSInt32 				inWhat,
									 void *					inData,
									 NMUInt32 				inDataLen,
									 NSpFlags 				inFlags);
	
	
	/*********************  Player Information  **********************/
	OP_DEFINE_API_C( NMErr )
	NSpPlayer_ChangeType			(NSpGameReference 		inGame,
									 NSpPlayerID 			inPlayerID,
									 NSpPlayerType 			inNewType);
	
	OP_DEFINE_API_C( NMErr )
	NSpPlayer_Remove				(NSpGameReference 		inGame,
									 NSpPlayerID 			inPlayerID);
	
	
	OP_DEFINE_API_C( NSpPlayerID )
	NSpPlayer_GetMyID				(NSpGameReference 		inGame);
	
	OP_DEFINE_API_C( NMErr )
	NSpPlayer_GetInfo				(NSpGameReference 		inGame,
									 NSpPlayerID 			inPlayerID,
									 NSpPlayerInfoPtr *		outInfo);
	
	OP_DEFINE_API_C( void )
	NSpPlayer_ReleaseInfo			(NSpGameReference 		inGame,
									 NSpPlayerInfoPtr 		inInfo);
	
	OP_DEFINE_API_C( NMErr )
	NSpPlayer_GetEnumeration		(NSpGameReference 		inGame,
									 NSpPlayerEnumerationPtr * outPlayers);
	
	OP_DEFINE_API_C( void )
	NSpPlayer_ReleaseEnumeration	(NSpGameReference 		inGame,
									 NSpPlayerEnumerationPtr  inPlayers);
	
	OP_DEFINE_API_C( NMSInt32 )
	NSpPlayer_GetRoundTripTime		(NSpGameReference 		inGame,
									 NSpPlayerID 			inPlayer);
	
	OP_DEFINE_API_C( NMSInt32 )
	NSpPlayer_GetThruput			(NSpGameReference 		inGame,
									 NSpPlayerID 			inPlayer);
	
	
	/*********************  Group Management  **********************/
	OP_DEFINE_API_C( NMErr )
	NSpGroup_New					(NSpGameReference 		inGame,
									 NSpGroupID *			outGroupID);
	
	OP_DEFINE_API_C( NMErr )
	NSpGroup_Dispose				(NSpGameReference 		inGame,
									 NSpGroupID 			inGroupID);
	
	OP_DEFINE_API_C( NMErr )
	NSpGroup_AddPlayer				(NSpGameReference 		inGame,
									 NSpGroupID 			inGroupID,
									 NSpPlayerID 			inPlayerID);
	
	OP_DEFINE_API_C( NMErr )
	NSpGroup_RemovePlayer			(NSpGameReference 		inGame,
									 NSpGroupID 			inGroupID,
									 NSpPlayerID 			inPlayerID);
	
	OP_DEFINE_API_C( NMErr )
	NSpGroup_GetInfo				(NSpGameReference 		inGame,
									 NSpGroupID 			inGroupID,
									 NSpGroupInfoPtr *		outInfo);
	
	OP_DEFINE_API_C( void )
	NSpGroup_ReleaseInfo			(NSpGameReference 		inGame,
									 NSpGroupInfoPtr 		inInfo);
	
	OP_DEFINE_API_C( NMErr )
	NSpGroup_GetEnumeration			(NSpGameReference 		inGame,
									 NSpGroupEnumerationPtr * outGroups);
	
	OP_DEFINE_API_C( void )
	NSpGroup_ReleaseEnumeration		(NSpGameReference 		inGame,
									 NSpGroupEnumerationPtr  inGroups);
	
	
	/**************************  Utilities  ***************************/
	OP_DEFINE_API_C( NMNumVersion )
	NSpGetVersion					(void);
	
	OP_DEFINE_API_C( void )
	NSpSetConnectTimeout			(NMUInt32 				inSeconds);
	
	OP_DEFINE_API_C( void )
	NSpClearMessageHeader			(NSpMessageHeader *		inMessage);
	
	OP_DEFINE_API_C( NMUInt32 )
	NSpGetCurrentTimeStamp			(NSpGameReference 		inGame);
	
	OP_DEFINE_API_C( NSpAddressReference )
	NSpCreateATlkAddressReference	(	char *theName, 
										char *theType, 
										char *theZone);
	
	OP_DEFINE_API_C( NSpAddressReference )
	NSpCreateIPAddressReference		(	const char *	inIPAddress, 
										const char *	inIPPort);
	
	OP_DEFINE_API_C( void )
	NSpReleaseAddressReference		(	NSpAddressReference inAddress);

	OP_DEFINE_API_C(NMErr)
	NSpPlayer_FreeAddress			(	NSpGameReference	inGame,
										void				**outAddress);

	OP_DEFINE_API_C( NMErr )
	NSpPlayer_GetIPAddress			(	NSpGameReference 	inGame,
										NSpPlayerID 		inPlayerID,
										char **				outAddress);
	
	
	/************************ Mac-CFM-Versions-Only routines ****************/
	#ifdef OP_PLATFORM_MAC_CFM
		OP_DEFINE_API_C( NMErr )
		NSpPlayer_GetOTAddress		(	NSpGameReference 	inGame,
										NSpPlayerID 		inPlayerID,
										OTAddress **		outAddress);
		
		OP_DEFINE_API_C( NSpAddressReference )
		NSpConvertOTAddrToAddressReference (OTAddress *			inAddress);
		
		OP_DEFINE_API_C( OTAddress *)
		NSpConvertAddressReferenceToOTAddr (NSpAddressReference  inAddress);
		
		OP_DEFINE_API_C( void )
		NSpReleaseOTAddress	(OTAddress 		*inAddress);
		
	#endif
	
	/************************ Advanced/Async routines ****************/
	typedef OP_CALLBACK_API( void , NSpCallbackProcPtr )(	NSpGameReference inGame, 
														void *inContext, 
														NSpEventCode inCode, 
														NMErr inStatus, 
														void *inCookie);
	
	OP_DEFINE_API_C( NMErr )
	NSpInstallCallbackHandler		(NSpCallbackProcPtr 	inHandler,
									 void *					inContext);
	
	
	// dair, added NSpJoinResponseMessage support
	typedef OP_CALLBACK_API( NMBoolean , NSpJoinRequestHandlerProcPtr )(	NSpGameReference 		inGame, 
																		NSpJoinRequestMessage *	inMessage, 
																		void *					inContext, 
																		unsigned char *			outReason,
																		NSpJoinResponseMessage *outMessage);
	
	OP_DEFINE_API_C( NMErr )
	NSpInstallJoinRequestHandler	(NSpJoinRequestHandlerProcPtr  inHandler,
									 void *							inContext);
	
	
	typedef OP_CALLBACK_API( NMBoolean , NSpMessageHandlerProcPtr )(	NSpGameReference 	inGame, 
																	NSpMessageHeader *	inMessage, 
																	void *				inContext);
	
	OP_DEFINE_API_C( NMErr )
	NSpInstallAsyncMessageHandler	(NSpMessageHandlerProcPtr  inHandler,
									 void *						inContext);


	#if PRAGMA_STRUCT_ALIGN
		#pragma options align=reset
	#elif PRAGMA_STRUCT_PACKPUSH
		#pragma pack(pop)
	#elif PRAGMA_STRUCT_PACK
		#pragma pack()
	#endif

	#ifdef PRAGMA_IMPORT_OFF
		#pragma import off
	#elif PRAGMA_IMPORT
		#pragma import reset
	#endif

#ifdef __cplusplus
}
#endif

#endif /*	__OPENPLAY__	*/
