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
#include "OpenPlay.h"
#endif
#include <Winsock2.h>
#include "win_ip_module.h"


//	------------------------------	Private Definitions
//	------------------------------	Private Types

enum
{
	WM_NET_MESSAGE_START_FILTER		= (WM_USER + 100),
	WM_DATAGRAM_MESSAGE				= WM_NET_MESSAGE_START_FILTER,
	WM_STREAM_MESSAGE,
	WM_MY_DESTROY_MESSAGE,
	WM_NET_MESSAGE_END_FILTER
};

#define MARK_ENDPOINT_AS_VALID(e, t) ((e)->valid_endpoints |= (1<<(t)))

#define MAX_STRINGS_PER_GROUP 140 // must match that in build_rc.c

#define PRINT_CASE(x) case (x): name= #x; break;

//	------------------------------	Private Variables
	
	NMUInt32	gWin_IP_WSAStartupCount = 0;

//	------------------------------	Private Functions

static NMErr 		create_endpoint(	SOCKADDR_IN *hostAddr, NMEndpointCallbackFunction *inCallback, void *inContext, 
										NMEndpointRef *outEndpoint, NMBoolean inActive, NMBoolean create_sockets, NMSInt32 connectionMode, NMBoolean netSprocketMode,
										NMUInt32 version, NMUInt32 gameID);
static NMErr 		receive_data	(	NMEndpointRef inEndpoint, NMSInt16 which_socket, unsigned char *ioData, NMUInt32 *ioSize,
									NMFlags *outFlags);
static NMSInt32		send_data(NMEndpointRef inEndpoint, NMSInt16 which_socket, unsigned char *inData,  NMUInt32 inSize, NMFlags inFlags);
static char *		network_message_to_string(NMSInt32 event);
static NMErr 		create_socket(NMEndpointRef new_endpoint, NMSInt16 index, SOCKADDR_IN *hostAddr, NMBoolean inActive, NMUInt16 required_port);
static NMBoolean	internally_handled_datagram(NMEndpointRef endpoint);

// lookup machine takes default_port in network order.
static NMBoolean 	lookup_machine(char *machine, u_short default_port, SOCKADDR_IN *hostAddr);
static NMBoolean 	create_endpoint_thread_and_window(NMEndpointRef endpoint);
static void 		destroy_window_and_endpoint_thread(NMEndpointRef endpoint);
static DWORD WINAPI thread_proc(LPVOID lpThreadParameter);
static NMBoolean 	internally_handle_read_data(NMEndpointRef endpoint, NMSInt16 type);
NMErr 				wait_for_open_complete(NMEndpointRef endpoint);
static void 		send_datagram_socket(NMEndpointRef endpoint);
static void 		receive_udp_port(NMEndpointRef endpoint);

//	------------------------------	Public Variables

NMBoolean gAdvertisingGames = true;


#if windows_build
	extern LPCSTR szNetCallbackWndClass;
#endif

static NMBoolean b_tcp_initd = false;


//----------------------------------------------------------------------------------------
// register_callback_window
//----------------------------------------------------------------------------------------

static BOOL
register_callback_window(HINSTANCE hInstance)
{
WNDCLASS	wc;

	machine_mem_zero(&wc, sizeof (wc));
	wc.style = 0;
	wc.lpfnWndProc = (WNDPROC) NetCallbackWndProc;
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

static NMBoolean
unregister_callback_window(void)
{	
	return UnregisterClass(szNetCallbackWndClass, gModuleGlobals.application_instance);
}

//----------------------------------------------------------------------------------------
// NMStartAdvertising
//----------------------------------------------------------------------------------------

NMBoolean
NMStartAdvertising(NMEndpointRef inEndpoint)
{
  UNUSED_PARAMETER(inEndpoint)

  gAdvertisingGames = true;

  return(true);
}

//----------------------------------------------------------------------------------------
// NMStopAdvertising
//----------------------------------------------------------------------------------------

NMBoolean
NMStopAdvertising(NMEndpointRef inEndpoint)
{
  UNUSED_PARAMETER(inEndpoint)

  gAdvertisingGames = false;

  return(true);
}

/* Module initialization */


//----------------------------------------------------------------------------------------
// NMGetModuleInfo
//----------------------------------------------------------------------------------------

NMErr
NMGetModuleInfo(NMModuleInfo *outInfo)
{
NMErr	err;

	op_vassert_return((outInfo != NULL),"NMModuleInfo is NIL!",kNMParameterErr);

	if (outInfo->size >= sizeof (NMModuleInfo))
	{
		outInfo->size= sizeof (NMModuleInfo);
		outInfo->type= kModuleID;
		nm_getcstr(outInfo->name, strNAMES, idModuleName, kNMNameLength);
		nm_getcstr(outInfo->copyright, strNAMES, idCopyright, kNMCopyrightLen);
		outInfo->maxPacketSize= 1500;
		outInfo->maxEndpoints= kNMNoEndpointLimit;
		outInfo->flags= kNMModuleHasStream | kNMModuleHasDatagram | kNMModuleHasExpedited | kNMModuleRequiresIdleTime;
		err= kNMNoError;
	}
	else
	{
		err = kNMModuleInfoTooSmall;
	}
	
	return err;
}

/* Endpoint functions */

//----------------------------------------------------------------------------------------
// NMOpen
//----------------------------------------------------------------------------------------

NMErr
NMOpen(
	NMConfigRef					inConfig, 
	NMEndpointCallbackFunction	*inCallback, 
	void						*inContext, 
	NMEndpointRef				*outEndpoint, 
	NMBoolean					inActive)
{
	NMErr		err = kNMNoError;
	WSADATA		aWsaData;
	NMSInt16	aVersionRequested= MAKEWORD(1, 1);
  	
	op_vassert_return((inCallback != NULL),"Callback function is NIL!",kNMParameterErr);
	op_vassert_return((outEndpoint != NULL),"outEndpoint endpoint is NIL!",kNMParameterErr);
    op_vassert_return(inConfig->cookie==config_cookie, csprintf(sz_temporary, "cookie: 0x%x != 0x%x", inConfig->cookie, config_cookie),kNMParameterErr); 
	
	
	// Here we make connection with the Winsock library.
	// this can cause problems if we do it when loading the library
	if (gWin_IP_WSAStartupCount == 0)
	{
		err = WSAStartup(aVersionRequested, &aWsaData);

		if (! err)
		{
			// Make sure they support our version (will not fail with newer versions!)
			if (LOBYTE(aWsaData.wVersion) != LOBYTE(aVersionRequested) || HIBYTE(aWsaData.wVersion) != HIBYTE(aVersionRequested))
			{
				WSACleanup();
				DEBUG_PRINT("Version bad! (%d.%d) != %d.%d", HIBYTE(aWsaData.wVersion), LOBYTE(aWsaData.wVersion),
								HIBYTE(aVersionRequested), LOBYTE(aVersionRequested));
			}
		}
		else
		{
			DEBUG_PRINT("Unable to startup winsock!");
		}
	}
	if (! err) 
		gWin_IP_WSAStartupCount++;
	
	// DNS it...
	// I am not sure if this will always work, due to some obscure documentation:
	// ie it might not work with 128.234. notation, but will with dartagnan.bungie.com... 
	if (inActive)
	{	
		// sjb 19990323 if we came from the UI, then this was looked up already!
		if (strlen(inConfig->host_name))
		{
		SOCKADDR_IN	hostInfo;
			// everything going into lookup_machine should be in network order...
			
			// sjb 19990323 output from lookup_machine is in network order already
			//              if it was a name, but not if it was a number
			if (lookup_machine(inConfig->host_name, inConfig->hostAddr.sin_port, &hostInfo))
			{
				inConfig->hostAddr = hostInfo;
			}
			else
			{
				err = kNMAddressNotFound;
			}
		}	// host and port should be set if lookup_machine is not called
	}

	// We need to ignore the host IP number for a passive open. Keep only port number

	if (! err)
	{
		*outEndpoint = NULL;
		err = create_endpoint(&(inConfig->hostAddr), inCallback, inContext, outEndpoint, 
			inActive, true, inConfig->connectionMode, inConfig->netSprocketMode, inConfig->version, inConfig->gameID);

		if (!err)
		{
			// copy the name
			strcpy((*outEndpoint)->name, inConfig->name);
			
			err= wait_for_open_complete(*outEndpoint);
		}
	}

	if (err)
	{
		*outEndpoint= NULL;
	}
	
	return err;
}

//----------------------------------------------------------------------------------------
// NMClose
//----------------------------------------------------------------------------------------

// play with the SO_LINGER socket options...
// NMClose is now asynchronous for the mac, but it is synchronous on the PC, because it's threaded.

NMErr
NMClose(
	NMEndpointRef	inEndpoint, 
	NMBoolean		inOrderly)
{
	NMSInt32 index;

	op_vassert_return((inEndpoint != NULL),"inEndpoint is NIL!",kNMParameterErr);
    op_vassert_return(inEndpoint->cookie==kModuleID, csprintf(sz_temporary, "cookie: 0x%x != 0x%x", inEndpoint->cookie, kModuleID),kNMParameterErr); 

	for (index= 0; index<NUMBER_OF_SOCKETS; ++index)
	{
		if (inEndpoint->socket[index] != INVALID_SOCKET)
		{
			NMErr			err;
			struct linger	aLinger;

				// no more messages.
	 		err = WSAAsyncSelect(inEndpoint->socket[index], inEndpoint->window, 0, 0);
			DEBUG_NETWORK_API("AsyncSelect", err);

			if (!inOrderly && index != _datagram_socket)
			{
				aLinger.l_onoff = true;
				aLinger.l_linger = 0;
				err = setsockopt(inEndpoint->socket[index], SOL_SOCKET, SO_LINGER, (LPSTR)&aLinger, sizeof (aLinger));
				DEBUG_NETWORK_API("setsockopt for hard close", err);
			}

            // shut it down
	   		err = shutdown(inEndpoint->socket[index], SD_BOTH);

	    	if (err == SOCKET_ERROR && WSAGetLastError() != WSAENOTCONN)
			{
		 		DEBUG_NETWORK_API("Shutdown", err);
			}
	     	err = closesocket(inEndpoint->socket[index]);
	      	DEBUG_NETWORK_API("closesocket", err);
	    }
	}

    if (inEndpoint->window)
    {
	  destroy_window_and_endpoint_thread(inEndpoint);
	}

      // Tell them that it is closed, if necessary
   	if (inEndpoint->alive)
   	{
		inEndpoint->callback(inEndpoint, inEndpoint->user_context, kNMCloseComplete, 0, NULL);
		inEndpoint->alive = false;
	}

	inEndpoint->cookie = 'bad ';
	dispose_pointer(inEndpoint);
	
	if (gWin_IP_WSAStartupCount > 0)
	{
		gWin_IP_WSAStartupCount--;
		if (gWin_IP_WSAStartupCount == 0)
			WSACleanup();
	}

	return kNMNoError;
}

//----------------------------------------------------------------------------------------
// NMAcceptConnection
//----------------------------------------------------------------------------------------

NMErr
NMAcceptConnection(
	NMEndpointRef				inEndpoint, 
	void						*inCookie, 
	NMEndpointCallbackFunction	*inCallback, 
	void						*inContext)
{
	NMErr			err;
	NMEndpointRef	new_endpoint;

	UNUSED_PARAMETER(inCookie)

	op_vassert_return((inCallback != NULL),"Callback is NULL!",kNMParameterErr);
	op_vassert_return((inEndpoint != NULL),"inEndpoint is NIL!",kNMParameterErr);
    op_vassert_return(inEndpoint->cookie==kModuleID, csprintf(sz_temporary, "cookie: 0x%x != 0x%x", inEndpoint->cookie, kModuleID),kNMParameterErr); 

    // create all the data...
    err = create_endpoint(0, inCallback, inContext, &new_endpoint, false, false, inEndpoint->connectionMode,
			  inEndpoint->netSprocketMode, inEndpoint->version, inEndpoint->gameID);

	if (! err)
	{
   		// create the sockets..
		SOCKADDR_IN	remote_address;
		int			remote_length = sizeof (remote_address);

 		// make the accept call...
		new_endpoint->socket[_stream_socket]= accept(inEndpoint->socket[_stream_socket], (LPSOCKADDR) &remote_address, 
													   &remote_length);

		if (new_endpoint->socket[_stream_socket] != INVALID_SOCKET)
		{
			SOCKADDR_IN	address;
			NMUInt16	required_port;
			int			size = sizeof (address);

			err = getsockname(new_endpoint->socket[_stream_socket], (LPSOCKADDR) &address, &size);

	  		if (!err)
			{
		  		if (new_endpoint->netSprocketMode)
		  		{
		  			required_port= ntohs(address.sin_port);
				}
				else
		 			required_port = 0;

				     // if we need a datagram socket
		      	if (inEndpoint->connectionMode & 1)
		 		{
					// create the datagram socket with the default port and host set..
					err= create_socket(new_endpoint, _datagram_socket, &remote_address, true, required_port);
		     	}

	            // set the window to get the messages, for the stream (the datagram already has it..)
		     	if (! err)
		   		{
					err = WSAAsyncSelect(new_endpoint->socket[_stream_socket],
					       new_endpoint->window, WM_NET_MESSAGE_START_FILTER+_stream_socket,
					       FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE);
					DEBUG_NETWORK_API("WSAsyncSelect Socket (stream) (for Accept)", err);
		      	}
		      	else
			  		DEBUG_NETWORK_API("Create Socket (datagram) (for Accept)", err);
			}
	  		else
	      		DEBUG_NETWORK_API("Getsockname", err);
		}
		else
			DEBUG_NETWORK_API("Accept (for Accept)", err);

		if (err)
		{
			NMClose(new_endpoint, false);
			new_endpoint= NULL;
		}
		else
		{
			// copy whatever we need from the original endpoint...
			new_endpoint->host= inEndpoint->host;
			new_endpoint->port= inEndpoint->port;
			strcpy(new_endpoint->name, inEndpoint->name);
			new_endpoint->status_proc= inEndpoint->status_proc;

			// if we are uber, send our datagram port..
			if ( ((new_endpoint->connectionMode & _uber_connection) == _uber_connection) && 
					(!new_endpoint->netSprocketMode) )
			{
				// uber connection.
			// passive and active send the datagram port.
				send_datagram_socket(new_endpoint);
			}
		}

		// and fake the async call...
		// This calls back the original endpoints callback proc, which may or may not be what you are expecting.
		// (It wasn't what I was expecting the first time)

		// CRT, Sep 24, 2000
		// There are 2 async calls here:  (1)  The kNMAcceptComplete callback is to be called to the new endpoint,
		// and (2) the kNMHandoffComplete is called to the original endpoint, which is the listener...

		inCallback(new_endpoint, inContext, kNMAcceptComplete, err, inEndpoint);
		inEndpoint->callback(inEndpoint, inEndpoint->user_context, kNMHandoffComplete, err, new_endpoint);
	}
	else
		DEBUG_NETWORK_API("Create Endpoint (for Accept)", err);

	return err;
}

//----------------------------------------------------------------------------------------
// NMRejectConnection
//----------------------------------------------------------------------------------------

NMErr
NMRejectConnection(
	NMEndpointRef	inEndpoint, 
	void			*inCookie)
{
SOCKET		doomed_socket;
NMErr		err;
SOCKADDR_IN	remote_address;
int			remote_length = sizeof(remote_address);

	UNUSED_PARAMETER(inCookie)

	op_vassert_return((inEndpoint != NULL),"inEndpoint is NIL!",kNMParameterErr);
	op_vassert_return(inEndpoint->cookie==kModuleID, csprintf(sz_temporary, "cookie: 0x%x != 0x%x", inEndpoint->cookie, kModuleID),kNMParameterErr); 

	// accept and then immediately close?
	doomed_socket = accept(inEndpoint->socket[_stream_socket], (struct sockaddr *)&remote_address, &remote_length);

	if (doomed_socket != INVALID_SOCKET)
	{
		struct linger	aLinger;
		MSG				msg;
	
		err = WSAAsyncSelect(doomed_socket, inEndpoint->window, 0, FD_READ);
		DEBUG_NETWORK_API("setting doomed socket to synchronous", err);

		// clear any pending messages
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// set to be a hard close...
		aLinger.l_onoff= true;
		aLinger.l_linger= 0;
		
		err = setsockopt(doomed_socket, SOL_SOCKET, SO_LINGER, (LPSTR) &aLinger, sizeof (aLinger));
		DEBUG_NETWORK_API("setsockopt for hard close", err);
	
		err = shutdown(doomed_socket, 2);
		DEBUG_NETWORK_API("shutdown (for Reject)", err);

		err = closesocket(doomed_socket);
		DEBUG_NETWORK_API("closesocket (for Reject)", err);
	}
	else
	{
		err = WSAGetLastError();
		DEBUG_NETWORK_API("Accept (for Reject)", err);
	}
	
	return err;
}

//----------------------------------------------------------------------------------------
// NMIsAlive
//----------------------------------------------------------------------------------------

NMBoolean
NMIsAlive(NMEndpointRef inEndpoint)
{
	op_vassert_return((inEndpoint != NULL),"inEndpoint is NIL!",false);
	op_vassert_return(inEndpoint->cookie==kModuleID, csprintf(sz_temporary, "cookie: 0x%x != 0x%x", inEndpoint->cookie, kModuleID),false); 
	return inEndpoint->alive;
}

//----------------------------------------------------------------------------------------
// NMSetTimeout
//----------------------------------------------------------------------------------------

NMErr
NMSetTimeout(
	NMEndpointRef inEndpoint, 
	NMUInt32 inTimeout) // in milliseconds
{
	op_vassert_return((inEndpoint != NULL),"inEndpoint is NIL!",kNMParameterErr);
    op_vassert_return(inEndpoint->cookie==kModuleID, csprintf(sz_temporary, "cookie: 0x%x != 0x%x", inEndpoint->cookie, kModuleID),kNMParameterErr); 
	inEndpoint->timeout= inTimeout;
	
	return kNMNoError;
}

//----------------------------------------------------------------------------------------
// NMGetIdentifier
//----------------------------------------------------------------------------------------

NMErr
NMGetIdentifier(NMEndpointRef inEndpoint,  char * outIdStr, NMSInt16 inMaxLen)
{
    NMErr				err = kNMNoError;
    SOCKET		the_socket = inEndpoint->socket[_stream_socket];
    SOCKADDR_IN remote_address;
    int			remote_length = sizeof(remote_address);
	char result[256];
    IN_ADDR addr;
    
	op_vassert_return((inEndpoint != NULL),"Endpoint is NIL!",kNMParameterErr);
	op_vassert_return((outIdStr != NULL),"Identifier string ptr is NIL!",kNMParameterErr);
	op_vassert_return((inMaxLen > 0),"Max string length is less than one!",kNMParameterErr);


    getpeername(the_socket, (SOCKADDR*)&remote_address, &remote_length);
    
    addr = remote_address.sin_addr;
 
	sprintf(result, "%u.%u.%u.%u", addr.s_net, addr.s_host, addr.s_lh, addr.s_impno);
	
	strncpy(outIdStr, result, inMaxLen - 1);
    outIdStr[inMaxLen - 1] = 0;

	return (kNMNoError);
}


/* Back door functions */


//----------------------------------------------------------------------------------------
// NMIdle
//----------------------------------------------------------------------------------------

NMErr
NMIdle(NMEndpointRef inEndpoint)
{
	MSG	msg;

	//op_vassert_return((inEndpoint != NULL),"inEndpoint is NIL!",kNMParameterErr);
    //op_vassert_return(inEndpoint->cookie==kModuleID, csprintf(sz_temporary, "cookie: 0x%x != 0x%x", inEndpoint->cookie, kModuleID),kNMParameterErr); 
	UNUSED_PARAMETER(inEndpoint)

	// ECF as long as we're dependent on the windows messaging system, we need to make sure its running
	// if someone is running a command-line app or whatnot and ProtocolIdle() is all they use...
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return kNMNoError;
}

//----------------------------------------------------------------------------------------
// NMFunctionPassThrough
//----------------------------------------------------------------------------------------

NMErr
NMFunctionPassThrough(
	NMEndpointRef	inEndpoint, 
	NMUInt32	inSelector, 
	void			*inParamBlock)
{
NMErr	err = kNMNoError;

	op_vassert_return((inEndpoint != NULL),"inEndpoint is NIL!",kNMParameterErr);
    op_vassert_return(inEndpoint->cookie==kModuleID, csprintf(sz_temporary, "cookie: 0x%x != 0x%x", inEndpoint->cookie, kModuleID),kNMParameterErr); 
	
	if  (inSelector == _pass_through_set_debug_proc) {
		
		inEndpoint->status_proc= (status_proc_ptr) inParamBlock;
	}	
	else
		err= kNMUnknownPassThrough;

	return err;
}

/* Data functions */


//----------------------------------------------------------------------------------------
// NMSendDatagram
//----------------------------------------------------------------------------------------

NMErr
NMSendDatagram(
	NMEndpointRef	inEndpoint, 
	NMUInt8			*inData, 
	NMUInt32		inSize, 
	NMFlags			inFlags)
{
	NMErr result;

	// send_data checks the parameters

	//ECF 011102 i'm under the assumption that sending a datagram always sends the whole thing
	//and doesnt return the number of bytes sent
	inFlags |= kNMBlocking; //stay in the func till its all sent
	result = send_data(inEndpoint, _datagram_socket, inData, inSize, inFlags);
	if (result > 0)
		result = kNMNoError;
		
	return result;
}

//----------------------------------------------------------------------------------------
// NMReceiveDatagram
//----------------------------------------------------------------------------------------

NMErr
NMReceiveDatagram(
	NMEndpointRef	inEndpoint, 
	NMUInt8			*ioData, 
	NMUInt32		*ioSize, 
	NMFlags			*outFlags)
{
	// receive_data checks the parameters

	return receive_data(inEndpoint, _datagram_socket, ioData, ioSize, outFlags);
}

//----------------------------------------------------------------------------------------
// NMSend
//----------------------------------------------------------------------------------------

NMSInt32
NMSend(
	NMEndpointRef inEndpoint, 
	void		*inData, 
	NMUInt32	inSize, 
	NMFlags		inFlags)
{
	// send_data checks the parameters
	
	return send_data(inEndpoint, _stream_socket, inData, inSize, inFlags);
}

//----------------------------------------------------------------------------------------
// NMReceive
//----------------------------------------------------------------------------------------

NMErr
NMReceive(
	NMEndpointRef	inEndpoint, 
	void			*ioData, 
	NMUInt32		*ioSize, 
	NMFlags			*outFlags)
{
	// receive_data checks the parameters

	return receive_data(inEndpoint, _stream_socket, ioData, ioSize, outFlags);
}

//----------------------------------------------------------------------------------------
//	Calls the "Enter Notifier" function on the requested endpoint (stream or datagram).
//----------------------------------------------------------------------------------------

NMErr
NMEnterNotifier(NMEndpointRef inEndpoint, NMEndpointMode endpointMode)
{
	//currently we dont do anything...
	return kNMNoError;
}


//----------------------------------------------------------------------------------------
//	Calls the "Leave Notifier" function on the requested endpoint (stream or datagram).
//----------------------------------------------------------------------------------------

NMErr
NMLeaveNotifier(NMEndpointRef inEndpoint, NMEndpointMode endpointMode)
{
	//currently we dont do anything...
	return kNMNoError;
}

//----------------------------------------------------------------------------------------
// nm_getcstr
//----------------------------------------------------------------------------------------

char *
nm_getcstr(
	char	*buffer,
	NMSInt16	resource_number,
	NMSInt16	string_number,
	NMSInt16	max_length)
{
NMSInt16	length;
NMSInt16	string_id= ((resource_number-128)*MAX_STRINGS_PER_GROUP)+string_number;

	op_assert(gModuleGlobals.application_instance);
	length= LoadString(gModuleGlobals.application_instance, string_id,	buffer, max_length);
	op_vassert(length, csprintf(buffer, "Trying for string: %d %d, got nothing: 0x%x (id: %d) Error: %d", 
		resource_number, string_number, gModuleGlobals.application_instance,
		string_id, GetLastError()));
	
	return buffer;
}

//----------------------------------------------------------------------------------------
// NetCallbackWndProc
//----------------------------------------------------------------------------------------

LONG APIENTRY
NetCallbackWndProc(
	HWND	hWnd, 
	UINT	message, 
	UINT	wParam, 
	LONG	lParam)
{
NMEndpointRef	endpoint= (NMEndpointRef) GetWindowLong(hWnd, 0);
NMErr			wsaError, wsaEvent;
SOCKET			socket;
void			*cookie = NULL;
NMCallbackCode	code;
NMBoolean		callback = true;	
LONG			return_value;
NMSInt16		socket_type;
NMBoolean		handle_event = true;

	switch (message)
	{
		case WM_DATAGRAM_MESSAGE:
		case WM_STREAM_MESSAGE:
			wsaError= WSAGETSELECTERROR(lParam);
			wsaEvent= WSAGETSELECTEVENT(lParam);
			socket= (SOCKET) wParam;
			socket_type= (message==WM_STREAM_MESSAGE) ? _stream_socket : _datagram_socket;

			// handle errors the best we can...
			if (wsaError) 
			{
				switch (wsaError)
				{
					case WSAECONNABORTED:
					case WSAECONNRESET:
						// they killed us, or they died
						wsaEvent = FD_CLOSE;
						break;
						
					case WSAECONNREFUSED:
						// trying to connect, and we were refused.
						endpoint->opening_error = kNMOpenFailedErr;
						handle_event = false;
						break;
						
					default:
						DEBUG_NETWORK_API(csprintf(sz_temporary, "NetCallback: EP 0x%x ConnMode: 0x%x ValidEP: 0x%x msg: %d", endpoint, 
														endpoint->connectionMode, endpoint->valid_endpoints, message), 
											wsaError);
						handle_event = false;
						break;
				}
			}

			// the only time this should ever happen is on a passive connection, where the
			//  endpoint gets a message after the accept, but before I can clear it's async listener.
			//  This should only happen with connections that open and then immediately abort.
			if (endpoint->socket[socket_type] != socket)
			{
				if (endpoint->active)
				{
				char	*type = (socket_type == _stream_socket) ? "stream" : "datagram";

					DEBUG_PRINT("Got a %s message: %s Error: %d Socket: %d EP: %x Window: %x (endpoint->socket[socket_type]: 0x%x)", type, 
						network_message_to_string(wsaEvent), wsaError, socket, endpoint, endpoint->window,endpoint->socket[socket_type]);
				}

				handle_event = false;
			}
			
#if DEBUG
			//endpoint->status_proc= DEBUG_PRINT;
			if (endpoint->status_proc)
			{
			char	*type = (socket_type == _stream_socket) ? "stream" : "datagram";

				endpoint->status_proc("Got a %s message: %s Error: %d Socket: %d EP: %x Window: %x", type, 
					network_message_to_string(wsaEvent), wsaError, socket, endpoint, endpoint->window);

				DEBUG_NETWORK_API(csprintf(sz_temporary, "%s Message Err", type), wsaError);
			}
#endif
			if (handle_event)
			{
				switch (wsaEvent)
				{
					case FD_READ:
					case FD_OOB:
						if (internally_handle_read_data(endpoint, socket_type))
						{
							callback= false;
						}
						else
						{
						NMUInt32	amount_to_read;
						
							// error contains the amount of bytes that can be read
							ioctlsocket(socket, FIONREAD, &amount_to_read);

							if (!wsaError)
								wsaError = (WORD) amount_to_read;

							if (amount_to_read)
							{
								if (socket_type == _stream_socket)
								{
									code = kNMStreamData;
								}
								else
								{
									code = kNMDatagramData;
								}
							}
							else
							{
								// this happens in enumeration.  I don't know why- it looks like their bug.
								callback = false;
							}
						}
						break;

					case FD_CONNECT:
					 	// we are live!
					 	// this message goes to the person connecting
					 	op_assert(socket_type==_stream_socket);

						if ((endpoint->connectionMode & _uber_connection)==_uber_connection)
						{
							// uber connection.
							// passive and active send the datagram port.
							
							if (!endpoint->netSprocketMode)
							{
								send_datagram_socket(endpoint);
							}
							else	// In NetSprocket mode, mark both endpoints as valid immediately!
							{
						 		MARK_ENDPOINT_AS_VALID(endpoint, _stream_socket);
						 		MARK_ENDPOINT_AS_VALID(endpoint, _datagram_socket);
							}
						}
						else
						{
						 	MARK_ENDPOINT_AS_VALID(endpoint, socket_type);
						}
						
						callback = false;
						break;

					case FD_ACCEPT: // not on a datagram message
					 	op_assert(socket_type == _stream_socket);
						code = kNMConnectRequest;
						break;

					case FD_WRITE:
						code = kNMFlowClear;
						MARK_ENDPOINT_AS_VALID(endpoint, socket_type);
						callback = false;
						break;
						
					case FD_CLOSE:
						code = kNMEndpointDied;
						break;
						
					default:
						callback = false;
						break;
				}
				
				op_assert(endpoint);
				op_assert(endpoint->callback);

				if (callback)
				{
					endpoint->callback(endpoint, endpoint->user_context, code, wsaError, cookie);
				}
			}
			break;

		case WM_MY_DESTROY_MESSAGE:
			break;
			
		default:
			return_value = DefWindowProc(hWnd, message, wParam, lParam);
			break ;
	}

    return return_value;
}

//----------------------------------------------------------------------------------------
// create_endpoint
//----------------------------------------------------------------------------------------

static NMErr
create_endpoint(
	SOCKADDR_IN					*hostInfo,
	NMEndpointCallbackFunction	*inCallback, 
	void						*inContext, 
	NMEndpointRef				*outEndpoint, 
	NMBoolean					inActive,
	NMBoolean					create_sockets,
	NMSInt32					connectionMode,
	NMBoolean					netSprocketMode,
	NMUInt32				version,
	NMUInt32				gameID)
{
NMEndpointRef	new_endpoint = new_pointer(sizeof (NMEndpointPriv)); 
NMErr			err = kNMNoError;
NMUInt16 		required_port;

	if (new_endpoint)
	{
		NMSInt16	index;
		NMSInt32	uber_connection = ((1 << _stream_socket) | (1 << _datagram_socket));

		machine_mem_zero(new_endpoint, sizeof (NMEndpointPriv));
		new_endpoint->cookie= kModuleID;
		new_endpoint->alive= false;
		new_endpoint->connectionMode= connectionMode;
		new_endpoint->netSprocketMode= netSprocketMode;
		new_endpoint->timeout= DEFAULT_TIMEOUT;
		new_endpoint->callback= inCallback;
		new_endpoint->user_context= inContext;
		new_endpoint->dynamically_assign_remote_udp_port= ((connectionMode & _uber_connection)==_uber_connection);
		new_endpoint->thread_id= NONE;
		new_endpoint->valid_endpoints= 0l;
		new_endpoint->version= version;
		new_endpoint->gameID= gameID;
		new_endpoint->active= inActive;
		new_endpoint->opening_error= 0;

		for (index = 0; index < NUMBER_OF_SOCKETS; ++index)
		{
			new_endpoint->socket[index]= INVALID_SOCKET;
		}
		
		// create the endpoints thread and the callback window
		if (create_endpoint_thread_and_window(new_endpoint))
		{
			// let them have it (note that the callbacks could fire before we complete
			// this loop, so we must do this now...)
			*outEndpoint= new_endpoint;

			/* Create the sockets */
			if (create_sockets)
			{
			// sjb need to determine whether we really need this port or not
			// converting back to host format since it expects it below.
			
			// CRT, Sep 24, 2000  (Response to above comment.)
			// I don't think we want this port for an active connection, and we certainly don't want 
			// it for an active NetSprocket connection.  However, I only want to touch this code for 
			// NetSprocket mode.  So I test for NetSprocket mode and leave the code as it was if
			// the test fails.  I recommend that while integrating this into the OpenPlay baseline, you
			// consider testing with required_port = 0 regardless of whether NetSprocket mode is being used.
			//
			// To try this, replace the if/else block that follows with:
			//
			//		if (inActive)
			//			required_port = 0;	// If connecting, use any port, but use the same one for
			//		else					// both stream and datagram sockets.
			//			required_port = ntohs(hostInfo->sin_port);	// If listening, use the requested port.
			//
			// This seems to have solved the problem with same-machine connection under NetSprocket, so if
			// it works in general for OpenPlay, then great!
			
				/*if (!new_endpoint->netSprocketMode)
				{
					required_port = ntohs(hostInfo->sin_port);
				}
				else*/
				{
					if (inActive)
						required_port = 0;	// If connecting, use any port, but use the same one for
					else					// both stream and datagram sockets.
						required_port = ntohs(hostInfo->sin_port);	// If listening, use the requested port.
				}

				// Maybe this should break up creating and connecting, so that we have a fully formed connection
				// before any events start coming in?			
				for (index = 0; !err && index<NUMBER_OF_SOCKETS; ++index)
				{
					// create what we should based on compatibility
					if (new_endpoint->connectionMode & (1 << index))
					{
					SOCKADDR_IN	address;
					int			size = sizeof(address);
	
						err = create_socket(new_endpoint, index, hostInfo, inActive, required_port);

						if (!err)
						{
							err = getsockname(new_endpoint->socket[index], (LPSOCKADDR) &address, &size);

							if (!err)
							{
								required_port= ntohs(address.sin_port);
							}
							else
							{
								DEBUG_NETWORK_API("Getsockname", err);
								err= kNMInternalErr;
							} 
						}
					}
				}
			}
		}
		else
		{
			// unable to create the window and socket...
			err = kNMOutOfMemoryErr;
		}

		if (err)
		{
			NMClose(new_endpoint, false);
 			new_endpoint= NULL;
 			*outEndpoint= NULL;
		}
		else
		{
			new_endpoint->alive= true;
		}
	}
	else
	{
		err = kNMOutOfMemoryErr;
	}

	return err;
}

//----------------------------------------------------------------------------------------
// receive_data
//----------------------------------------------------------------------------------------

static NMErr
receive_data(
	NMEndpointRef	inEndpoint,
	NMSInt16		which_socket,
	unsigned char	*ioData,
	NMUInt32		*ioSize,
	NMFlags			*outFlags)
{
NMErr		err = kNMNoError;
NMSInt32	result;

	*outFlags = 0;
	op_vassert_return((inEndpoint != NULL),"inEndpoint is NIL!",kNMParameterErr);
    op_vassert_return(inEndpoint->cookie==kModuleID, csprintf(sz_temporary, "cookie: 0x%x != 0x%x", inEndpoint->cookie, kModuleID),kNMParameterErr); 
	op_assert(inEndpoint->socket[which_socket]!=INVALID_SOCKET);
	result = recv(inEndpoint->socket[which_socket], (char *)ioData, *ioSize, 0);

	if (result == 0) { // connection has been closed.
	
		err= kNMNoDataErr;
	} 
	else if (result == SOCKET_ERROR)
	{
		err= WSAGetLastError();

		if (err==WSAEWOULDBLOCK)
			err= kNMNoDataErr;
		else
			DEBUG_NETWORK_API("Receive", err);
	}
	else {				
		*ioSize = result;
	}

	return err;
}

//----------------------------------------------------------------------------------------
// send_data
//----------------------------------------------------------------------------------------

// returns the number of bytes that were actually sent....
static NMSInt32
send_data(
	NMEndpointRef	inEndpoint, 
	NMSInt16		which_socket,
	unsigned char	*inData, 
	NMUInt32		inSize, 
	NMFlags			inFlags)
{
NMErr			err = kNMNoError;
NMSInt32		result;
NMUInt32		bytes_to_send = inSize;
NMSInt32		offset = 0;
NMUInt32		total_bytes_sent = 0;
NMBoolean		done = false;

	op_vassert_return((inEndpoint != NULL),"inEndpoint is NIL!",kNMParameterErr);
    op_vassert_return(inEndpoint->cookie==kModuleID, csprintf(sz_temporary, "cookie: 0x%x != 0x%x", inEndpoint->cookie, kModuleID),kNMParameterErr); 
	op_vassert_return(inEndpoint->socket[which_socket]!=INVALID_SOCKET,"inEndpoint->socket[which_socket]!=INVALID_SOCKET",kNMParameterErr);

	do
	{
		done = true;
	
		// note that this may block (use flow control stuff....)
		result = send(inEndpoint->socket[which_socket], (const char *)inData+offset, bytes_to_send, 0);

		switch (result)
		{
			case SOCKET_ERROR:
				err = WSAGetLastError();
				done = true;

				switch (err)
				{
					case WSANOTINITIALISED: // didn't call WSAStartup()
					case WSAENETDOWN: // Networking subsytem went down
					case WSAEACCES: // trying to send to the broadcast address, but not flag not set.
					case WSAEFAULT: // buffer not valid
					case WSAENETRESET: // connection must be reset because windows dropped it
					case WSAENOTCONN: // not connected
					case WSAENOTSOCK: // not a socket
					case WSAEOPNOTSUPP: // out of band not supported
					case WSAESHUTDOWN: // socket has been shutdown- can't send on it.
					case WSAEINVAL: // socket has not been bound
					case WSAECONNRESET: // remote side reset the circuit
						err = kNMInternalErr;
						break;
						
					case WSAECONNABORTED: // timed out internally
						err = kNMTimeoutErr;
						break;
						
					case WSAEWOULDBLOCK: // send would block...
//						// block until it all goes down the pipe.
//						if ((inFlags & kNMBlocking) && total_bytes_sent!=inSize) done= false;
					
					//  Begin crt 9/3/2001, If not blocking, at least let the user know that there was a flow condition!
					//  Don't expect them to assume that a return value of "0" means that a flow error has occured, even
					//	though that may be the most logical assumption.	
						if (inFlags & kNMBlocking)
						{
							err= kNMNoError;	
							if (total_bytes_sent!=inSize)
								done = false;
						}
						else
						{
							err = kNMFlowErr;
						}					
					//  End crt 9/3/2001	

						break;
						
					case WSAEMSGSIZE: // too much data..
					case WSAENOBUFS: // buffer deadlock
						err = kNMTooMuchDataErr;
						break;
						
					case WSAEINTR: // blocking send was cancelled..
					case WSAEINPROGRESS: // blocking call in progress
						DEBUG_PRINT("What? got err: %d", err);
						err = kNMInternalErr;
						break;
				
					case 0:
						DEBUG_PRINT("send() returned INVALID_SOCKET, but there is no error...");
						err = kNMInternalErr;
						break;
				
					default:
						DEBUG_NETWORK_API("Unknown send error", err);
						err = kNMInternalErr;
						break;
				}
				break;

			default:
				op_assert(result>=0);
				
				// increment the offset and the bytes sent.
				offset += result;
				bytes_to_send -= result;
			
				// err is the number of bytes sent
				total_bytes_sent += result;
				err= total_bytes_sent;

				if ((inFlags & kNMBlocking) && total_bytes_sent!=inSize)
					done = false;

				break;		
		}
	} while (!done);
	
	return err;
}

//----------------------------------------------------------------------------------------
// network_message_to_string
//----------------------------------------------------------------------------------------

static char *
network_message_to_string(NMSInt32 event)
{
char	*name;
	
	switch (event)
	{
		case FD_READ:
			name = "FD_READ";
			break;
			
		case FD_OOB:
			name = "FD_OOB";
			break;

		case FD_CONNECT:
		 	// we are live!
			name = "FD_CONNECT";
			break;

		case FD_ACCEPT: // not on a datagram message
			name = "FD_ACCEPT";
			break;

		case FD_WRITE:
			name = "FD_WRITE";
			break;
			
		case FD_CLOSE:
			name = "FD_CLOSE";
			break;
			
		default:
			name = "< unknown >";
			break;
	}
	
	return name;
}

//----------------------------------------------------------------------------------------
// create_socket
//----------------------------------------------------------------------------------------

static NMErr
create_socket(
	NMEndpointRef	new_endpoint,
	NMSInt16		index,
	SOCKADDR_IN		*hostAddr,
	NMBoolean		inActive,
	NMUInt16			required_port)
{
NMSInt32	socket_types[] = { SOCK_DGRAM, SOCK_STREAM };
NMErr		err = kNMNoError;

	op_assert(socket_types[_datagram_socket]==SOCK_DGRAM);
	op_assert(socket_types[_stream_socket]==SOCK_STREAM);

	// sjb 19990330 either we aren't active meaning we don't need a remote address
	//              or we better be passed an address
	op_assert((inActive == 0) || (0 != hostAddr));

	new_endpoint->socket[index]= socket(PF_INET, socket_types[index], 0);

	if (new_endpoint->socket[index]!=INVALID_SOCKET)
	{
		SOCKADDR_IN	sin;
		BOOL	opt = true;
		
		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = INADDR_ANY;
		sin.sin_port = htons(required_port); // because we want our udp and tcp sockets to be the same

		// If it is nonzero, we want to make sure we can bind to it..
		if (required_port)
		{
			err = setsockopt(new_endpoint->socket[index], SOL_SOCKET, SO_REUSEADDR, (LPSTR) &opt, sizeof (opt));
			DEBUG_NETWORK_API("setsockopt for reuse address", err);
		}

		if (0 == inActive && index == _stream_socket)
		{
			err = setsockopt(new_endpoint->socket[index], IPPROTO_TCP, TCP_NODELAY, (LPSTR)&opt, sizeof (opt));
			DEBUG_NETWORK_API("setsockopt for no delay", err);
		}
		err = bind(new_endpoint->socket[index], (LPSOCKADDR) &sin, sizeof (sin));
		if (!err)
		{
			NMSInt32	bitmask;
			SOCKADDR_IN	address;
			int			size = sizeof(address);

			// get the host and port...
			getsockname(new_endpoint->socket[index], (LPSOCKADDR) &address, &size);
			new_endpoint->host= ntohl(address.sin_addr.s_addr);
			new_endpoint->port= ntohs(address.sin_port);

			if (index==_stream_socket)
			{
				if (inActive)
				{
					bitmask= FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE;
				}
				else
				{
					// this is done because I get FD_READ events on the accepted connection, before
					//  I get the connection all setup to receive them (thus the listening endpoint is
					//  actually getting the FD_READ, and ignoring it).  So I just disable those events....
					bitmask= FD_ACCEPT | FD_CONNECT | FD_CLOSE;
				}
			}
			else
			{			
				bitmask= FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE;
			}
		
			// created the window, set the callback function....
			// This sets it to non-blocking
			err = WSAAsyncSelect(new_endpoint->socket[index], new_endpoint->window, WM_NET_MESSAGE_START_FILTER+index, bitmask);

			if (! err)
			{
				// We be done?
				if (inActive)
				{
					// sjb 19990330 these fields are now in network byte order.
					//              so don't convert them
					// make the connect call...
					err = connect(new_endpoint->socket[index], (struct sockaddr *)hostAddr, sizeof (*hostAddr));

					if (err)
					{
						err = WSAGetLastError();
						
						if (err != WSAEWOULDBLOCK)
						{
							DEBUG_NETWORK_API("Connect", err);
							err = kNMOpenFailedErr;
						}
						else // this will call our callback...
							err = kNMNoError;
					}
				}
				else
				{
					if (index == _stream_socket)
					{
						// make the listen call (on the stream port)
						err = listen(new_endpoint->socket[index], MAXIMUM_OUTSTANDING_REQUESTS);
						DEBUG_NETWORK_API("Listen", err);
					}
				}
			}
			else
				DEBUG_NETWORK_API("WSAAsyncSelect", err);
		}
		else
		{
			DEBUG_NETWORK_API("Bind Socket", err);
			err = kNMOpenFailedErr;
		}
	}
	else
	{
		err = WSAGetLastError();
		DEBUG_NETWORK_API("Creating Socket",err);

		// standard error codes.
		err= kNMOpenFailedErr;
	}
	
	return err;
}

//----------------------------------------------------------------------------------------
// internally_handled_datagram
//----------------------------------------------------------------------------------------

static NMBoolean
internally_handled_datagram(NMEndpointRef endpoint)
{
NMBoolean	handled_internally = false;
NMSInt32	bytes_read;
NMUInt32	first_datagram_length;
NMErr		err = kNMNoError;

	// read just the first 4 bytes. (first..)
	// should be really fast..
	
	
	err = ioctlsocket(endpoint->socket[_datagram_socket], FIONREAD, &first_datagram_length);
	if (!err)
	{
		char	packet[kQuerySize];

		if (first_datagram_length == sizeof (packet))
		{
			bytes_read= recvfrom(endpoint->socket[_datagram_socket], packet, sizeof (packet), MSG_PEEK, NULL, NULL);

			if (bytes_read == sizeof (packet))
			{
				if (is_ip_request_packet(packet, bytes_read, endpoint->gameID))
				{
					SOCKADDR_IN	remote_address;
					char		response_packet[512];
					NMSInt32	bytes_to_send, result;
					int			remote_address_size = sizeof(remote_address);

					// read it out.
					bytes_read = recvfrom(endpoint->socket[_datagram_socket], (char *) &packet, sizeof (packet),
						0, (LPSOCKADDR) &remote_address, &remote_address_size);

					if (gAdvertisingGames)
					{
						// And respond to it.
						bytes_to_send= build_ip_enumeration_response_packet(response_packet, endpoint->gameID, 
							endpoint->version, endpoint->host, endpoint->port, endpoint->name, 0, NULL);
						op_assert(bytes_to_send<=sizeof (response_packet));

						byteswap_ip_enumeration_packet(response_packet);

						// send!
						result= sendto(endpoint->socket[_datagram_socket], response_packet, 
							bytes_to_send, 0, (LPSOCKADDR) &remote_address, sizeof (remote_address));

						switch (result)
						{
							case SOCKET_ERROR:
								DEBUG_NETWORK_API("Sendto on enum response", result);
								return false;
							default:
								op_assert(result==bytes_to_send);
								break;		
						}
					}
					// we handled it.
					handled_internally= true;
				}
			}
			else
			{
				err= WSAGetLastError();
				DEBUG_NETWORK_API("Reading the rest of it", err);
			}
		} 
	}
	else
	{
		DEBUG_NETWORK_API("ioctlsocket", err);
	}

	return handled_internally;
}

//----------------------------------------------------------------------------------------
// lookup_machine
//----------------------------------------------------------------------------------------

// returns in network order...
// sjb 19990330 This takes default_port in network order
//              What if the host name comes in in Unicode?
static NMBoolean
lookup_machine(
	char		*	machine,
	NMUInt16		default_port,
	SOCKADDR_IN	*	hostAddr)
{
NMBoolean		gotAddr = false;
char			trimmedHost[256];
char			*colonBlow;		// pointer to rightmost colon in name
NMUInt16		needPort;		// port we need
HOSTENT			*hostInfo;
NMUInt32		hostIPAddr;

	op_assert(hostAddr != 0);

	// sjb 19990330 stupidly, take port numbers on the end of host names/numbers
	// This does nothing in the context of OpenPlay, since we always specify the port
	// number separately. This will let you paste in a host name with a colon and
	// safely ignore it.
	needPort = default_port;

	strcpy(trimmedHost, machine);

	// find first colon
	colonBlow = strchr(trimmedHost,':');

	// everything after that is the port number	if we found it
	if (colonBlow)
	{
		*colonBlow++ = 0;		// strip the port spec and pass to next char
		needPort = htons((NMSInt16) atoi(colonBlow));
	}

	if (INADDR_NONE != (hostIPAddr = inet_addr(trimmedHost)))
	{
		gotAddr = 1;
	}
	//	sjb 19990326 This is NOT IPv6 compliant! This is evil. Use new resolver calls later.
	else if (0 != (hostInfo = gethostbyname(trimmedHost)))
	{
		// 	We successfully got an ip address by resolving a name
		//	have at it. We must have at least one address, take it
		hostIPAddr = *(NMUInt32 *) (hostInfo->h_addr_list[0]);

		gotAddr = 1;
	}

	if (gotAddr)
	{
		hostAddr->sin_family = AF_INET;
		hostAddr->sin_port = needPort;
		hostAddr->sin_addr.S_un.S_addr = hostIPAddr;
	}

	return gotAddr;
}

//----------------------------------------------------------------------------------------
// create_endpoint_thread_and_window
//----------------------------------------------------------------------------------------

// Thread proc...
static NMBoolean
create_endpoint_thread_and_window(NMEndpointRef endpoint)
{
NMBoolean	success = false;

	endpoint->window= CreateWindow(szNetCallbackWndClass,
		"", 
		WS_OVERLAPPED,
		0, 0, 0, 0,
		NULL, NULL, // no parent, no menu,
		gModuleGlobals.application_instance,
		NULL);

	// Set it's long..
	if (endpoint->window)
	{
		SetWindowLong(endpoint->window, 0, (NMSInt32) endpoint);
		success = true;
	}

	return success;
}

//----------------------------------------------------------------------------------------
// destroy_window_and_endpoint_thread
//----------------------------------------------------------------------------------------

static void
destroy_window_and_endpoint_thread(NMEndpointRef endpoint)
{
	if (endpoint->window)
	{
		DestroyWindow(endpoint->window);
		endpoint->window= NULL;
	}
}

//----------------------------------------------------------------------------------------
// internally_handle_read_data
//----------------------------------------------------------------------------------------

static NMBoolean
internally_handle_read_data(NMEndpointRef endpoint, NMSInt16 type)
{
NMBoolean	handled_internally = false;

	if (type == _stream_socket)
	{
		// if we are an uber endpoint ASSIGN_SOCKET 
		if (endpoint->dynamically_assign_remote_udp_port)
		{
			endpoint->dynamically_assign_remote_udp_port = false;
			
			if (endpoint->netSprocketMode)
			{
				// In NetSprocket mode, the first message is to be handled by the NetSprocket layer,
				// i.e. *not* internally by OpenPlay.  This is for backwaards compatibility with
				// previous versions of NetSprocket which did not use OpenPlay.
				
				handled_internally = false;
			}
			else
			{
				receive_udp_port(endpoint);
				handled_internally = true;
			}

		}
	}
	else
	{
		handled_internally= internally_handled_datagram(endpoint);
	}

	return handled_internally;
}

//----------------------------------------------------------------------------------------
// wait_for_open_complete
//----------------------------------------------------------------------------------------

NMErr
wait_for_open_complete(NMEndpointRef endpoint)
{
NMErr		err = kNMNoError;
NMUInt32	initial_ticks = machine_tick_count();
NMBoolean	done = false;
	
	// if we aren't an uber packet, we won't ever get the passive endpoint stream socket......
	// since we don't get confirmation on the passive endpoint stream until a connect..
	if (!endpoint->active && (endpoint->connectionMode & (1<<_stream_socket))) 
	{
		MARK_ENDPOINT_AS_VALID(endpoint, _stream_socket);
	}

	// Wait for connection...
	do
	{
		MSG	msg;

		// check for pending messages.		
		//** This should probably only look for messages in my message queue, and not everyones, but I don't have access
		//** to a list of windows right here (OpenPlay does though)
		// Changed 'while' to 'if' for PeekMessage(), so we stop as soon as we're open-complete.
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	
		if ((endpoint->connectionMode & endpoint->valid_endpoints)==endpoint->connectionMode)
		{
			// both are open.
			done= true;
		}
	} while(!done && !endpoint->opening_error && machine_tick_count()-initial_ticks<30*MACHINE_TICKS_PER_SECOND);

	if (!done)
	{
		// error...
		err = kNMOpenFailedErr;

		// and close it down...
		endpoint->alive= false;
		NMClose(endpoint, false);
	}
	
	return err;
}

//----------------------------------------------------------------------------------------
// send_datagram_socket
//----------------------------------------------------------------------------------------

static void
send_datagram_socket(NMEndpointRef endpoint)
{
SOCKADDR_IN	address;
int			size = sizeof(address);
NMErr		err;

	err = getsockname(endpoint->socket[_datagram_socket], (LPSOCKADDR) &address, &size);

	if (! err)
	{
		udp_port_struct_from_server	port_data;
		
		port_data.port= address.sin_port;

		// in network endianness.
		err= send_data(endpoint, _stream_socket, (unsigned char *) &port_data, sizeof (port_data), 0);
	}
	else
		DEBUG_NETWORK_API("Getting UDP Port", err);
}

//----------------------------------------------------------------------------------------
// receive_udp_port
//----------------------------------------------------------------------------------------

static void
receive_udp_port(NMEndpointRef endpoint)
{
udp_port_struct_from_server	port_data;
NMSInt32	err;
NMUInt32	size= sizeof(port_data);
NMUInt32	flags;

	// Read the port...
	err= receive_data(endpoint, _stream_socket, (unsigned char *)&port_data, &size, &flags);
	if (!err && size==sizeof (port_data))
	{
		NMSInt16 	old_port;
		SOCKADDR_IN address;
		int			address_size= sizeof (address);

		op_assert(size==sizeof (port_data)); // or else
		
		// And change our local port to match theirs..
		getpeername(endpoint->socket[_datagram_socket], (LPSOCKADDR) &address, &address_size);
		old_port= address.sin_port;
		address.sin_port= port_data.port; // alrady in network order

		// make the connect call.. (sets the udp destination)
		err= connect(endpoint->socket[_datagram_socket], (LPSOCKADDR) &address, sizeof (address));
		if (!err)
		{
			// and mark the connection as having been completed...
			MARK_ENDPOINT_AS_VALID(endpoint, _stream_socket);
		}
		else
			DEBUG_NETWORK_API("dynamic port- didn't connect", err);
	}
	else
		DEBUG_NETWORK_API("dynamic port- receive data", err);
}
