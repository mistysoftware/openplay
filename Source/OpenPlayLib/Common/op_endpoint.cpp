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

#ifndef __OPENPLAY__
#include			"OpenPlay.h"
#endif
#ifndef __NETMODULE__
#include 			"NetModule.h"
#endif

#include			"portable_files.h"

#include			"OPUtils.h"
#include 			"NetModulePrivate.h"

#include 			"op_definitions.h"
#include 			"op_globals.h"
#include 			"module_management.h"
#include 			"OPDLLUtils.h"

/* ------------ local code */

static void net_module_callback_function(NMEndpointRef inEndpoint, 
	                   void* inContext, NMCallbackCode inCode, 
                           NMErr inError, void* inCookie);

static PEndpointRef create_endpoint_and_bind_to_protocol_library(NMType type, 
                      FileDesc *library_file, NMErr *err);

static Endpoint *create_endpoint_for_accept(PEndpointRef endpoint, NMErr *err, NMBoolean *from_cache);
static void clean_up_endpoint(PEndpointRef endpoint, NMBoolean return_to_cache);

#if (DEBUG)
	char* GetOPErrorName(NMErr err);
#endif 


/* ---------- opening/closing */

//doxygen instruction:
/** @addtogroup EndpointManagement
 * @{
 */


//----------------------------------------------------------------------------------------
// ProtocolOpenEndpoint
//----------------------------------------------------------------------------------------
/**
	Create a new endpoint.
	@brief Creates a new endpoint.
	@param inConfig The \ref PConfigRef configuration to create end endpoint from.
	@param inCallback The callback function OpenPlay will call for events involving this endpoint.
	@param inContext A custom context that will be passed to the endpoint's callback function. 
	@param outEndpoint Pass a pointer to a \ref PEndpointRef which will receive the new endpoint.
	@param flags Flags - passing \ref kOpenNone will create a passive(host) endpoint, while passing \ref kOpenActive will create an active(client) endpoint.
	@return \ref kNMNoError if the function succeeds.\n
	Otherwise, an error code.
	\n\n\n\n
 */

/*
	Takes the config data created by ProtocolConfig, and builds an endpoint out of it.
	active, passive
	listening & spawn (ie multiple endpoints)
	Callback for multiples.
*/

/*
	Binds to the protocol layer, fills in the function pointers, stays bound until
	CloseEndpoint is called, and calls ProtocolOpenEndpoint
*/
NMErr ProtocolOpenEndpoint(
	PConfigRef inConfig, 
	PEndpointCallbackFunction inCallback,
	void *inContext,
	PEndpointRef *outEndpoint, 
	NMOpenFlags flags)
{
	NMErr err= kNMNoError;
	Endpoint *ep;

	/* call as often as possible (anything that is synchronous) */
	op_idle_synchronous(); 

	ep= create_endpoint_and_bind_to_protocol_library(inConfig->type, &inConfig->file, &err);
	if(!err)
	{
		op_assert(ep);
		
		ep->openFlags = flags;
		
		/* So we know where we came from.. */
		machine_copy_data(&inConfig->file, &ep->library_file, sizeof(FileDesc));
		/* Now call it's initialization function */
		if(ep->NMOpen)
		{
			/* setup the callback data */
			ep->callback.user_callback= inCallback;
			ep->callback.user_context= inContext;

			/* And open.. */			
			err= ep->NMOpen((NMProtocolConfigPriv*)inConfig->configuration_data, net_module_callback_function, (void *)ep, &ep->module, 
                                        (flags & kOpenActive) ? true : false);
			if(!err)
			{
				op_assert(ep->module);
				*outEndpoint= ep;
				DEBUG_PRINT("New endpoint created (0X%X) of type %c%c%c%c (0X%X to netmodule)",ep,inConfig->type >> 24, (inConfig->type >> 16) & 0xFF,(inConfig->type>>8)&0xFF,inConfig->type&0xFF,ep->module);
			} else {
				DEBUG_PRINT("Open failed creating endpoint #%d of type %c%c%c%c",count_endpoints_of_type(inConfig->type),inConfig->type>>24,(inConfig->type>>16)&0xFF,(inConfig->type>>8)&0xFF,inConfig->type&0xFF); 
				/* Close up everything... */
				clean_up_endpoint(ep, false);
			}
		} else {
			err= kNMFunctionNotBoundErr;
		}
	}

#if (DEBUG)
	if( err )
		DEBUG_PRINT("ProtocolOpenEndpoint returning %s (%d)\n",GetOPErrorName(err),err);
#endif //DEBUG
	
	return err;
}

//----------------------------------------------------------------------------------------
// ProtocolCloseEndpoint
//----------------------------------------------------------------------------------------
/**
	Close an endpoint.  Endpoints must always be closed,
	even if they have received a \ref kNMEndpointDied message.
	After closing an endpoint, one should wait for its \ref kNMCloseComplete message to arrive,
	or else wait until \ref ProtocolIsAlive() returns false for the endpoint.
	Only then can one be certain that the endpoint's callback will no longer be triggered with new messages.
	@brief Close an endpoint.
	@param endpoint The endpoint to close.
	@param inOrderly Whether to attempt an "orderly" disconnect.  This is not yet well-defined...
	@return \ref kNMNoError if the function succeeds.\n
	Otherwise, an error code.
	\n\n\n\n
 */

/*
	closes the endpoint
	puts NULL in all the function pointers,
	unbinds from the shared library,
*/
NMErr ProtocolCloseEndpoint(
	PEndpointRef endpoint,
	NMBoolean inOrderly)
{
	NMErr err= kNMNoError;

	/* call as often as possible (anything that is synchronous) */

	DEBUG_PRINT("ProtocolCloseEndpoint called for endpoint (0X%X)",endpoint);
	
	if(valid_endpoint(endpoint) && endpoint->state != _state_closing)
	{
		op_assert(endpoint->cookie==PENDPOINT_COOKIE);
		if(endpoint->NMClose)
		{
			/* don't try to close it more than once... */
			if(endpoint->state==_state_unknown)
			{
				endpoint->state= _state_closing;
				
				//make sure we're not working with an incomplete endpoint
				op_assert(endpoint->module != NULL);
				err= endpoint->NMClose(endpoint->module, inOrderly);

			} else {
				op_vwarn(endpoint->state==_state_closing, csprintf(sz_temporary, 
					"endpoint state was %d should have been _state_unknown (%d) or _state_closing (%d) in ProtocolCloseEndpoint", 
					endpoint->state, _state_unknown, _state_closing));
			}
		} else {
			err= kNMFunctionNotBoundErr;
		}
		/* note we clean up everything on the response from the NMClose... */
	} else {
		if (valid_endpoint(endpoint) == false)
			DEBUG_PRINT("invalid endpoint(0X%X) passed to ProtocolCloseEndpoint()",endpoint);
		else if (endpoint->state == _state_closing)
			DEBUG_PRINT("endpoint (0X%X) passed to ProtocolCloseEndpoint() in state _state_closing",endpoint);
		err= kNMParameterErr;
	}
	
#if (DEBUG)
	if( err )
		DEBUG_PRINT("ProtocolCloseEndpoint returning %s (%d) for endpoint (0X%X)",GetOPErrorName(err),err,endpoint);	
#endif //DEBUG
	return err;
}

//----------------------------------------------------------------------------------------
// clean_up_endpoint
//----------------------------------------------------------------------------------------

static void clean_up_endpoint(
	PEndpointRef endpoint,
	NMBoolean return_to_cache)
{
  UNUSED_PARAMETER(return_to_cache)

  op_vassert(valid_endpoint(endpoint), csprintf(sz_temporary, "clean up: ep 0x%x is not valid (orderly: %d)", endpoint, return_to_cache));
 
  /* Free up everything.. */
  remove_endpoint_from_loaded_list(endpoint);

  /* done after the above to make sure everything's okay.. */
  endpoint->cookie = PENDPOINT_BAD_COOKIE;
  endpoint->state = _state_closed;

  unbind_from_protocol(endpoint->type, true);
	
#ifdef OP_API_NETWORK_WINSOCK
	dispose_pointer(endpoint);
#elif defined(OP_API_NETWORK_OT)
	if (return_to_cache)
    {
      if(gOp_globals.cache_size+1 <= MAXIMUM_CACHED_ENDPOINTS)
	{
	  gOp_globals.endpoint_cache[gOp_globals.cache_size++] = endpoint;
	} else {
	  // we may be at interrupt time here, so we have to add this to a list of things that we free later... 
	  op_assert(gOp_globals.doomed_endpoint_count+1 < MAXIMUM_DOOMED_ENDPOINTS);
	  gOp_globals.doomed_endpoints[gOp_globals.doomed_endpoint_count++] = endpoint;
	}
    } else {
      dispose_pointer(endpoint);
    }
#else
  /* do something for ports here ?? */
#endif
}

//----------------------------------------------------------------------------------------
// ProtocolAcceptConnection
//----------------------------------------------------------------------------------------
/**
	Accept a pending client connection, creating a new \ref PEndpointRef for the client connection.
	@brief Accept a pending client connection.
	@param endpoint The endpoint that received the \ref kNMConnectRequest message.
	@param inCookie The cookie value which accompanied the \ref kNMConnectRequest message to the host endpoint's callback.
	@param inNewCallback The callback function for the new endpoint to be created for the client.
	@param inNewContext The custom context to be assigned to the new endpoint.
	@return \ref kNMNoError if the function succeeds.\n
	Otherwise, an error code.
	\n\n\n\n
 */

/* -------------- connections */
NMErr 
ProtocolAcceptConnection(
	PEndpointRef endpoint,
	void *inCookie,
	PEndpointCallbackFunction inNewCallback,
	void *inNewContext)
{
	NMErr err= kNMNoError;

	op_vassert(valid_endpoint(endpoint), csprintf(sz_temporary, "ProtocolAcceptConnection: ep 0x%x is not valid", endpoint));
	if(endpoint)
	{
		op_assert(endpoint->cookie==PENDPOINT_COOKIE);
		if(endpoint->NMAcceptConnection)
		{
			Endpoint *new_endpoint;
			NMBoolean from_cache;
			
			new_endpoint= create_endpoint_for_accept(endpoint, &err, &from_cache);
			if(!err)
			{
				op_assert(new_endpoint);
				
				new_endpoint->callback.user_callback= inNewCallback;
				new_endpoint->callback.user_context= inNewContext;

/* DEBUG_PRINT("Calling accept connection on endpoint: 0x%x ep->module: 0x%x new_endpoint: 0x%x", endpoint, endpoint->module, new_endpoint); */
				err= endpoint->NMAcceptConnection(endpoint->module, inCookie, net_module_callback_function, new_endpoint);
				if(!err)
				{
				} else {
					/* error occured.  Clean up... */
					DEBUG_PRINT("Open failed creating endpoint #%d of type 0x%x for accept Error: %d", count_endpoints_of_type(endpoint->type), endpoint->type, err);
					clean_up_endpoint(new_endpoint, from_cache); /* don't return to the cache..*/
				}
			}
		} else {
			err= kNMFunctionNotBoundErr;
		}
	} else {
		err= kNMParameterErr;
	}
	
#if (DEBUG)
	if( err )
		DEBUG_PRINT("ProtocolAcceptConnection returning %s (%d)",GetOPErrorName(err),err);	
#endif //DEBUG
	return err;
}

//----------------------------------------------------------------------------------------
// ProtocolRejectConnection
//----------------------------------------------------------------------------------------
/**
	Reject a pending client connection.
	@brief Reject a pending client connection.
	@param endpoint The endpoint that received the \ref kNMConnectRequest message.
	@param inCookie the cookie value which accompanied the \ref kNMConnectRequest message to the host endpoint's callback.
	@return \ref kNMNoError if the function succeeds.\n
	Otherwise, returns an error code.
	@warning Some NetModules/configurations may be unable to do a "graceful" rejection, and the client side will appear to have the connection
	accepted and then immediately receive a \ref kNMEndpointDied message.
	The safest way to avoid this is to always accept a connection initially, and then send it a custom-defined approval or denial message, disconnecting it then if necessary.
	\n\n\n\n
 */

NMErr ProtocolRejectConnection(
	PEndpointRef endpoint, 
	void *inCookie)
{
	NMErr err= kNMNoError;
	
	op_vassert(valid_endpoint(endpoint), csprintf(sz_temporary, "ProtocolRejectConnection: ep 0x%x is not valid", endpoint));
	if(endpoint)
	{
		op_assert(endpoint->cookie==PENDPOINT_COOKIE);

		if(endpoint->NMRejectConnection)
		{
			op_assert(endpoint->module);
			err= endpoint->NMRejectConnection(endpoint->module, inCookie);
		} else {
			err= kNMFunctionNotBoundErr;
		}
	} else {
		err= kNMParameterErr;
	}
	
#if (DEBUG)
	if( err )
		DEBUG_PRINT("ProtocolRejectConnection returning %s (%d)",GetOPErrorName(err),err);		
#endif //DEBUG
	return err;
}

//----------------------------------------------------------------------------------------
// ProtocolSetTimeout
//----------------------------------------------------------------------------------------
/**
	Set the default timeout for OpenPlay operations.
	@brief Set the default timeout for OpenPlay operations.
	@param endpoint The endpoint for which the timeout is to be set.
	@param timeout The timeout value, in 60ths of a second.
	@return \ref kNMNoError if the function succeeds.\n
	Otherwise, an error code.
	\n\n\n\n
 */

/* -------------- options */
/*
	set timeouts 
	 (in 60ths of a second)
*/
NMErr ProtocolSetTimeout(
	PEndpointRef endpoint, 
	long timeout)
{
	NMErr err= kNMNoError;

	/* call as often as possible (anything that is synchronous) */
	op_idle_synchronous(); 
	
	op_assert(valid_endpoint(endpoint));
	if(endpoint)
	{
		op_assert(endpoint->cookie==PENDPOINT_COOKIE);

		/* keep a local copy. */
		if(endpoint->NMSetTimeout)
		{
			op_assert(endpoint->module);
			err= endpoint->NMSetTimeout(endpoint->module, timeout);
		} else {
			err= kNMFunctionNotBoundErr;
		}
	} else {
		err= kNMParameterErr;
	}

	return err;
}

//----------------------------------------------------------------------------------------
// ProtocolIsAlive
//----------------------------------------------------------------------------------------
/**
	Determine if an endpoint exists and is able to trigger a callback.
	Once this function returns true for an endpoint, the endpoint can be assumed truly "dead" and its associated contexts
	disposed of without fear of the endpoint's callback being triggered any more.
	@brief Determine if an endpoint exists and is able to trigger a callback.
	@param endpoint the endpoint to check for life.
	@return true if the endpoint exists and has not yet sent its \ref kNMCloseComplete message.\n
	false otherwise.
	\n\n\n\n
 */

NMBoolean ProtocolIsAlive(
	PEndpointRef endpoint)
{
	NMBoolean state = false;
	
	if(valid_endpoint(endpoint) == false)
		return false;
		
	if(endpoint)
	{
		op_assert(endpoint->cookie==PENDPOINT_COOKIE);
		/* keep a local copy. */
		if(endpoint->NMIsAlive)
		{
			op_assert(endpoint->module);
			state= endpoint->NMIsAlive(endpoint->module);
		}
	}

	return state;
}

//----------------------------------------------------------------------------------------
// ProtocolIdle
//----------------------------------------------------------------------------------------
/**
	Provide processing time to an endpoint if necessary. \ref ProtocolGetEndpointInfo() can be used to determine if the endpoint requires idle time.
	@brief Provide processing time to an endpoint if necessary.
	@param endpoint The endpoint to provide processing time to.
	@return \ref kNMNoError if the function succeeds.\n
	Otherwise, an error code.
	\n\n\n\n
 */

NMErr ProtocolIdle(
	PEndpointRef endpoint)
{
	NMErr err= kNMNoError;

	/* call as often as possible (anything that is synchronous) */
	op_idle_synchronous(); 

	//ECF011115 it would seem that a protocol requiring idle time might still want it
	//while closing
	//if(valid_endpoint(endpoint) && endpoint->state != _state_closing)
	if(valid_endpoint(endpoint))
	{
		op_assert(endpoint->cookie==PENDPOINT_COOKIE);
		if(endpoint->NMIdle)
		{
			op_assert(endpoint->module);
			err= endpoint->NMIdle(endpoint->module);
		} else {
			err= kNMFunctionNotBoundErr;
		}
	} else {
		err= kNMParameterErr;
	}

	return err;
}

//----------------------------------------------------------------------------------------
// ProtocolFunctionPassThrough
//----------------------------------------------------------------------------------------
/**
	Access NetModule-specific functionality for a \ref PEndpointRef.
	@brief Access NetModule-specific functionality for a \ref PEndpointRef.
	@param endpoint The \ref PEndpointRef on which the supplementary function acts.
	@param inSelector An index that tells the NetModule which custom operation is to be done.
	@param inParamBlock data to pass to the NetModule for the function.
	@return \ref kNMNoError if the function succeeds.\n
	\ref kNMUnknownPassThrough if the NetModule does not recognize the value of \e inSelector. \n
	Otherwise, an error code.
	\n\n\n\n
 */

NMErr ProtocolFunctionPassThrough(
	PEndpointRef endpoint, 
	NMUInt32 inSelector, 
	void *inParamBlock)
{
	NMErr err= kNMNoError;
	
	op_assert(valid_endpoint(endpoint));
	if(endpoint)
	{
		op_assert(endpoint->cookie==PENDPOINT_COOKIE);

		/* keep a local copy. */
		if(endpoint->NMFunctionPassThrough)
		{
			op_assert(endpoint->module);
			err= endpoint->NMFunctionPassThrough(endpoint->module, inSelector, inParamBlock);
		} else {
			err= kNMFunctionNotBoundErr;
		}
	} else {
		err= kNMParameterErr;
	}

	return err;
}

//----------------------------------------------------------------------------------------
// ProtocolSetEndpointContext
//----------------------------------------------------------------------------------------
/**
	Set an endpoint's associated context to a new value.
	@brief Set an endpoint's associated context to a new value.
	@param endpoint The endpoint whose context is to be set.
	@param newContext The new context for the endpoint.
	@return \ref kNMNoError if the function succeeds.\n
	Otherwise, an error code.
	\n\n\n\n
 */

/* >>> [Edmark/PBE] 11/10/99 added ProtocolSetEndpointContext() 
  "... allows us to set an endpoint's context after it's created. Adding this 
   function saved us hundreds of lines of messy code elsewhere." */

NMErr ProtocolSetEndpointContext(PEndpointRef endpoint, void* newContext)
{
	NMErr err= kNMNoError;

	op_assert(valid_endpoint(endpoint));
	op_assert(endpoint->cookie == PENDPOINT_COOKIE);

	if (endpoint && (endpoint->cookie == PENDPOINT_COOKIE))
	{
		endpoint->callback.user_context = newContext;
	}
	else
	{
		err = kNMParameterErr;
	}

	return err;
}


//----------------------------------------------------------------------------------------
// ProtocolStartAdvertising
//----------------------------------------------------------------------------------------
/**
	Enable a host endpoint to be seen on the network.
	@brief Enable a host endpoint to be seen on the network.
	@param endpoint The passive(host) endpoint to enable advertising for.
	@return No return value.
	\n\n\n\n
 */

/* >>>> Moved ProtocolStart/StopAdvertising() from op_module_mgmt.c */
void ProtocolStartAdvertising(PEndpointRef endpoint)	/* [Edmark/PBE] 11/8/99 changed parameter from PConfigRef to PEndpointRef */
{
	endpoint->NMStartAdvertising(endpoint->module);
}
//----------------------------------------------------------------------------------------
// ProtocolStopAdvertising
//----------------------------------------------------------------------------------------
/**
	Prevent a host endpoint from being seen on the network.
	@brief Prevent a host endpoint from being seen on the network.
	@param endpoint The passive(host) endpoint to disable advertising for.
	@return No return value.
	\n\n\n\n
 */

void ProtocolStopAdvertising(PEndpointRef endpoint)	/* [Edmark/PBE] 11/8/99 changed parameter from PConfigRef to PEndpointRef */
{
	endpoint->NMStopAdvertising(endpoint->module);
}

/** @}*/

 //doxygen instruction:
/** @addtogroup DataTransfer
 * @{
 */

//----------------------------------------------------------------------------------------
// ProtocolSendPacket
//----------------------------------------------------------------------------------------
/**
	Send a packet via an endpoint's unreliable(datagram) connection.  Packets are generally faster
	arriving than stream data, but are not guaranteed to arrive in order, if at all.  The ones that do arrive,
	however, arrive as packets of the same size as they were sent, unlike stream data, which "flows" in as one long
	stream of data with no boundaries.  
	@brief Send a packet via an endpoint's unreliable(datagram) connection.
	@param endpoint The endpoint to send the data to.
	@param inData Pointer to the data to be sent.
	@param inLength Size of the data to be sent.
	@param inFlags Flags.  None used currently.
	@return \ref kNMNoError if the function succeeds.\n
	\ref kNMFlowErr if no data can currently be sent due to flow control (network is saturated).
	In this case, the endpoint will be sent a \ref kNMFlowClear message when data can be sent again.\n
	Otherwise, an error code.
	\n\n\n\n
 */
/* -------------- sending/receiving */	
/*
	Send the packet to the given endpoint
*/
NMErr ProtocolSendPacket(
	PEndpointRef endpoint, 
	void *inData, 
	NMUInt32 inLength, 
	NMFlags inFlags)
{
	NMErr err= kNMNoError;

	/* Some of this error checking could be ignored.. */	
	op_assert(valid_endpoint(endpoint));
	if(endpoint)
	{
		op_assert(endpoint->cookie==PENDPOINT_COOKIE);
		if(endpoint->NMSendDatagram)
		{
			op_assert(endpoint->module);
			err= endpoint->NMSendDatagram(endpoint->module, (unsigned char*)inData, inLength, inFlags);
		} else {
			err= kNMFunctionNotBoundErr;
		}
	} else {
		err= kNMParameterErr;
	}

	return err;
}

//----------------------------------------------------------------------------------------
// ProtocolReceivePacket
//----------------------------------------------------------------------------------------
/**
	Receive a packet from an endpoint's unreliable(datagram) connection.
	@brief Receive a packet from an endpoint's unreliable(datagram) connection.
	@param endpoint The endpoint to receive the data from.
	@param outData Pointer to a buffer to receive a datagram.
	@param outLength In: size of \e outData buffer. Out: size of datagram received.
	@param outFlags Flags. Pass \ref kNMBlocking to block until receival of a datagram.
	@return \ref kNMNoError if the function succeeds.\n
	\ref kNMNoDataErr if there are no more waiting datagrams.\n
	Otherwise, an error code.
	\n\n\n\n
 */
/*
	Tries to receive the endpoint.  Does a copy here.
*/	
NMErr ProtocolReceivePacket(
	PEndpointRef endpoint, 
	void *outData, 
	NMUInt32 *outLength, 
	NMFlags *outFlags)
{
	NMErr err= kNMNoError;

	/* Some of this error checking could be ignored.. */	
	op_assert(valid_endpoint(endpoint));
	
	if(endpoint)
	{
		op_assert(endpoint->cookie==PENDPOINT_COOKIE);
		if(endpoint->NMReceiveDatagram)
		{
			op_assert(endpoint->module);
			err= endpoint->NMReceiveDatagram(endpoint->module, (unsigned char*)outData, outLength, outFlags);
		} else {
			err= kNMFunctionNotBoundErr;
		}
	} else {
		err= kNMParameterErr;
	}
	
	return err;
}

//----------------------------------------------------------------------------------------
// ProtocolSend
//----------------------------------------------------------------------------------------
/**
	Send data via an endpoint's reliable(stream) connection.  Stream data is guaranteed to arrive in the order it was sent
	and it generally faster for large amounts of data, but does not arrive in "packets" of any specific size.  Applications must
	be prepared to break the stream back into packets if necessary.
	@brief Send data via an endpoint's reliable(stream) connection.
	@param endpoint The endpoint to send the data to.
	@param inData Pointer to the data to be sent.
	@param inSize Size of the data to be sent.
	@param inFlags Flags.
	@return If positive, the number of bytes actually sent.\n If negative, an error code.\n
	If the number of bytes sent is less than the number requested, the endpoint will receive a \ref kNMFlowClear message once
	more data can be sent.
	\n\n\n\n
*/
NMSInt32 ProtocolSend(
	PEndpointRef endpoint, 
	void *inData, 
	NMUInt32 inSize, 
	NMFlags inFlags)
{
NMSInt32 result= 0;

	/* Some of this error checking could be ignored.. */	
	op_warn(valid_endpoint(endpoint));
	if(endpoint)
	{
		op_assert(endpoint->cookie==PENDPOINT_COOKIE);
		if(endpoint->NMSend)
		{
			op_assert(endpoint->module);
			result= endpoint->NMSend(endpoint->module, inData, inSize, inFlags);
		} else {
			result= kNMFunctionNotBoundErr;
		}
	} else {
		result= kNMParameterErr;
	}
	
	return result; // is < 0 incase of error...
}

//----------------------------------------------------------------------------------------
// ProtocolReceive
//----------------------------------------------------------------------------------------
/**
	Receive data from an endpoint's reliable(stream) connection.
	@brief Receive data from an endpoint's reliable(stream) connection.
	@param endpoint The endpoint to receive the data from.
	@param outData Pointer to a buffer to receive the stream data.
	@param ioSize In: amount of data requested. Out: amount of data received.
	@param outFlags Flags. Pass \ref kNMBlocking to block until receival of \e ioSize bytes of data.
	@return \ref kNMNoError if the function succeeds.\n
	\ref kNMNoDataErr if there is no more data to be read.\n
	Otherwise, an error code.
	\n\n\n\n
 */
NMErr ProtocolReceive(
	PEndpointRef endpoint, 
	void *outData, 
	NMUInt32 *ioSize, 
	NMFlags *outFlags)
{
	NMErr err= kNMNoError;

	/* Some of this error checking could be ignored.. */	
	op_assert(valid_endpoint(endpoint));
	if(endpoint)
	{
		op_vassert(endpoint->cookie==PENDPOINT_COOKIE, csprintf(sz_temporary, "Endpoint cookie not right: ep: 0x%x cookie: 0x%x",endpoint, endpoint->cookie));
		if(endpoint->NMReceive)
		{
			op_assert(endpoint->module);
			err= endpoint->NMReceive(endpoint->module, outData, ioSize, outFlags);
		} else {
			err= kNMFunctionNotBoundErr;
		}
	} else {
		err= kNMParameterErr;
	}
	
	return err;
}

//----------------------------------------------------------------------------------------
// ProtocolEnterNotifier
//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------
/**
	Temporarily disable callbacks for an endpoint.
	This should be used only for small blocks of code where it is important that an endpoint's callback not be called.
	Be sure to call \ref ProtocolLeaveNotifier() when done.
	@brief Temporarily disable callbacks for an endpoint.
	@param endpoint The endpoint to disable callbacks for.
	@param endpointMode The endpoint mode/s to disable callbacks for.
	@return \ref kNMNoError if the function succeeds.\n
	Otherwise, an error code.
	\n\n\n\n
 */

NMErr ProtocolEnterNotifier(
	PEndpointRef endpoint,
	NMEndpointMode endpointMode)
{
	NMErr err= 0;

	if(endpoint)
	{
		op_vpause("ProtocolEnterNotifier - Checking for endpoint->NMEnterNotifier.");
		if(endpoint->NMEnterNotifier)
		{
			op_vpause("ProtocolEnterNotifier - Calling endpoint->NMEnterNotifier...");
			err= endpoint->NMEnterNotifier(endpoint->module, endpointMode);
		} else {
			op_vpause("ProtocolEnterNotifier - endpoint->NMEnterNotifier not bound.");
			err= kNMFunctionNotBoundErr;
		}
	} else {
		op_vpause("ProtocolEnterNotifier - Called with NULL endpoint.");
		err= kNMParameterErr;
	}

	return err;
}

//----------------------------------------------------------------------------------------
// ProtocolLeaveNotifier
//----------------------------------------------------------------------------------------
/**
	Re-enable callbacks for an endpoint.
	@brief Re-enable callbacks for an endpoint.
	@param endpoint The endpoint to enable callbacks for.
	@param endpointMode The endpoint mode/s to enable callbacks for.
	@return \ref kNMNoError if the function succeeds.\n
	Otherwise, an error code.
	\n\n\n\n
 */

NMErr ProtocolLeaveNotifier(
	PEndpointRef endpoint,
	NMEndpointMode endpointMode)
{
	NMErr err= 0;

	if(endpoint)
	{
		if(endpoint->NMLeaveNotifier)
		{
			err= endpoint->NMLeaveNotifier(endpoint->module, endpointMode);
		} else {
			err= kNMFunctionNotBoundErr;
		}
	} else {
		err= kNMParameterErr;
	}

	return err;
}


//----------------------------------------------------------------------------------------
// ProtocolGetEndpointInfo
//----------------------------------------------------------------------------------------
/**
	Returns extended info about an endpoint.
	@brief Returns extended info about an endpoint.
	@param endpoint The endpoint to obtain info for.
	@param info Pointer to the \ref NMModuleInfoStruct to be filled in.
	@return \ref kNMNoError if the function succeeds.\n
	Otherwise, an error code.
	\n\n\n\n
 */

/* ----------- information functions */
NMErr ProtocolGetEndpointInfo(
	PEndpointRef endpoint, 
	NMModuleInfo *info)
{
	NMErr err= kNMNoError;

	/* Some of this error checking could be ignored.. */	
	op_assert(valid_endpoint(endpoint));
	op_assert(endpoint->cookie==PENDPOINT_COOKIE);
	if(endpoint)
	{
		if(endpoint->NMGetModuleInfo)
		{
			err= endpoint->NMGetModuleInfo(info);
		} else {
			err= kNMFunctionNotBoundErr;
		}
	} else {
		err= kNMParameterErr;
	}
	
	return err;
}

//----------------------------------------------------------------------------------------
// ProtocolFreeEndpointAddress
//----------------------------------------------------------------------------------------
/*
	Frees the address of the endpoint in the type specified.
*/
NMErr ProtocolFreeEndpointAddress(
	PEndpointRef endpoint,
	void **outAddress)
{
	NMErr err= kNMNoError;

	/* call as often as possible (anything that is synchronous) */
	op_idle_synchronous(); 
	
	// CRT 4/15/04 Don't die here just because you couldn't get an IP address!
	// Log a warning, fill in default values, and return an error!
	
	op_warn(valid_endpoint(endpoint));
	
	if(valid_endpoint(endpoint))
	{
		op_assert(endpoint->cookie==PENDPOINT_COOKIE);

		/* keep a local copy. */
		if(endpoint->NMFreeAddress)
		{
			op_assert(endpoint->module);
			err= endpoint->NMFreeAddress(endpoint->module, outAddress);
		} else {
			err= kNMFunctionNotBoundErr;
		}
	} else {
		err= kNMParameterErr;
	}

	return( err );
}

//----------------------------------------------------------------------------------------
// ProtocolGetEndpointAddress
//----------------------------------------------------------------------------------------
/*
	Returns the address of the endpoint in the type specified.
*/
NMErr ProtocolGetEndpointAddress(
	PEndpointRef endpoint,
	NMAddressType addressType,
	void **outAddress)
{
	NMErr err= kNMNoError;

	/* call as often as possible (anything that is synchronous) */
	op_idle_synchronous(); 
	
	// CRT 4/15/04 Don't die here just because you couldn't get an IP address!
	// Log a warning, fill in default values, and return an error!
	
	op_warn(valid_endpoint(endpoint));
	
	if(valid_endpoint(endpoint))
	{
		op_assert(endpoint->cookie==PENDPOINT_COOKIE);

		/* keep a local copy. */
		if(endpoint->NMGetAddress)
		{
			op_assert(endpoint->module);
			err= endpoint->NMGetAddress(endpoint->module, addressType, outAddress);
		} else {
			err= kNMFunctionNotBoundErr;
		}
	} else {
		err= kNMParameterErr;
	}

	return err;
}

//----------------------------------------------------------------------------------------
// ProtocolGetEndpointIdentifier
//----------------------------------------------------------------------------------------

/* ----------- information functions */
NMErr ProtocolGetEndpointIdentifier (   
    PEndpointRef endpoint,
    char *identifier_string, 
    NMSInt16 max_length)
{
	NMErr err= kNMNoError;

	/* Some of this error checking could be ignored.. */	
	op_assert(valid_endpoint(endpoint));
	op_assert(endpoint->cookie==PENDPOINT_COOKIE);
	if(endpoint)
	{
		if(endpoint->NMGetIdentifier)
		{
			err= endpoint->NMGetIdentifier(endpoint->module, identifier_string, max_length);
		} else {
			err= kNMFunctionNotBoundErr;
		}
	} else {
		err= kNMParameterErr;
	}
	
	return err;
}

//----------------------------------------------------------------------------------------
// net_module_callback_function
//----------------------------------------------------------------------------------------

/* ------------ static code */
static void net_module_callback_function(
	NMEndpointRef inEndpoint, 
	void* inContext,
	NMCallbackCode inCode, 
	NMErr inError, 
	void* inCookie)
{
	PEndpointRef ep= (PEndpointRef) inContext;
	NMBoolean callback= true;
	
	op_assert(ep);
	op_assert(ep->callback.user_callback);
	op_vassert(ep->cookie==PENDPOINT_COOKIE, csprintf(sz_temporary, "inCookie wasn't our PEndpointRef? 0x%x Cookie: 0x%x", ep, ep->cookie));

	/* must always reset it, because it may not be set yet. (trust me) */
	op_assert(valid_endpoint(ep));
	ep->module= inEndpoint;
	switch(inCode)
	{
		case kNMAcceptComplete: /* new endpoint, cookie is the parent endpoint. (easy) */
			/* generate the kNMHandoffComplete code.. */
			op_assert(ep->parent);
			if(ep->parent->callback.user_callback)
			{
				ep->parent->callback.user_callback(ep->parent, ep->parent->callback.user_context, kNMHandoffComplete, 
					inError, ep);
			}
			
			/* now setup for the kNMAcceptComplete */
			inCookie= ep->parent;
			op_assert(inCookie);
			break;
			
		case kNMHandoffComplete:
			/* eat it. */
			callback= false;
			break;
		default:
		break;
	}

	if(callback && ep->callback.user_callback)
		ep->callback.user_callback(ep, ep->callback.user_context, inCode, inError, inCookie);
	if(inCode==kNMCloseComplete)
	{
		op_vwarn(ep->state==_state_closing, csprintf(sz_temporary, 
			"endpoint state was %d should have been _state_closing (%d) for kNMCloseComplete", 
			ep->state, _state_closing));
		clean_up_endpoint(ep, true); /* Must try to return to the cache, because we can't deallocate memory at interrupt time. */
	}
}

//----------------------------------------------------------------------------------------
// create_endpoint_and_bind_to_protocol_library
//----------------------------------------------------------------------------------------

static PEndpointRef create_endpoint_and_bind_to_protocol_library(
	NMType type,
	FileDesc *library_file,
	NMErr *err)
{
	Endpoint *ep = NULL;
	
	ep= (Endpoint *) new_pointer(sizeof(Endpoint));
	if(ep)
	{
		machine_mem_zero(ep, sizeof(Endpoint));
		*err= bind_to_protocol(type, library_file, &ep->connection);
		if(!(*err))
		{
			/* Load in the endpoint information.. */
			ep->cookie= PENDPOINT_COOKIE;
			ep->type= type;
			ep->state= _state_unknown;
			ep->library_file= *library_file;
			ep->parent= NULL;

			/* Lock and load the pointers.. */
			ep->NMGetModuleInfo= (NMGetModuleInfoPtr) load_proc(ep->connection, kNMGetModuleInfo);

			ep->NMGetConfig= (NMGetConfigPtr) load_proc(ep->connection, kNMGetConfig);
			ep->NMGetConfigLen= (NMGetConfigLenPtr) load_proc(ep->connection, kNMGetConfigLen);

			ep->NMOpen= (NMOpenPtr) load_proc(ep->connection, kNMOpen);
			ep->NMClose= (NMClosePtr) load_proc(ep->connection, kNMClose);

			ep->NMAcceptConnection= (NMAcceptConnectionPtr) load_proc(ep->connection, kNMAcceptConnection);
			ep->NMRejectConnection= (NMRejectConnectionPtr) load_proc(ep->connection, kNMRejectConnection);

			ep->NMSendDatagram= (NMSendDatagramPtr) load_proc(ep->connection, kNMSendDatagram);
			ep->NMReceiveDatagram= (NMReceiveDatagramPtr) load_proc(ep->connection, kNMReceiveDatagram);

			ep->NMSend= (NMSendPtr) load_proc(ep->connection, kNMSend);
			ep->NMReceive= (NMReceivePtr) load_proc(ep->connection, kNMReceive);

			ep->NMSetTimeout= (NMSetTimeoutPtr) load_proc(ep->connection, kNMSetTimeout);
			ep->NMIsAlive= (NMIsAlivePtr) load_proc(ep->connection, kNMIsAlive);
			ep->NMFreeAddress= (NMFreeAddressPtr) load_proc(ep->connection, kNMFreeAddress);
			ep->NMGetAddress= (NMGetAddressPtr) load_proc(ep->connection, kNMGetAddress);
			ep->NMFunctionPassThrough= (NMFunctionPassThroughPtr) load_proc(ep->connection, kNMFunctionPassThrough);

			ep->NMIdle= (NMIdlePtr) load_proc(ep->connection, kNMIdle);

			ep->NMStopAdvertising  = (NMStopAdvertisingPtr)  load_proc(ep->connection, kNMStopAdvertising);
			ep->NMStartAdvertising = (NMStartAdvertisingPtr) load_proc(ep->connection, kNMStartAdvertising);

			ep->NMEnterNotifier  = (NMEnterNotifierPtr)  load_proc(ep->connection, kNMEnterNotifier);
			ep->NMLeaveNotifier  = (NMLeaveNotifierPtr)  load_proc(ep->connection, kNMLeaveNotifier);

			ep->NMGetIdentifier = (NMGetIdentifierPtr) load_proc(ep->connection, kNMGetIdentifier);

			/* [Edmark/PBE] 11/8/99 moved NMStart/StopAdvertising from ProtocolConfig to Endpoint */

			ep->next= NULL;

			/* remember this... */
			add_endpoint_to_loaded_list(ep);
		}
		
		if(*err)
		{
			/* clear! */
			dispose_pointer(ep);
			ep= NULL;
		}
	} else {
		*err= kNMOutOfMemoryErr;
	}

	return ep;
}

//----------------------------------------------------------------------------------------
// create_endpoint_for_accept
//----------------------------------------------------------------------------------------

static Endpoint *create_endpoint_for_accept(
	PEndpointRef endpoint,
	NMErr *err,
	NMBoolean *from_cache)
{
	Endpoint *new_endpoint= NULL;

#ifdef OP_API_NETWORK_WINSOCK
	/* We must create the wrapper for it.. */
	new_endpoint= create_endpoint_and_bind_to_protocol_library(endpoint->type, &endpoint->library_file, err);
	*from_cache= false;
#elif defined(OP_API_NETWORK_OT)
	if(gOp_globals.cache_size>0)
	{
		new_endpoint= gOp_globals.endpoint_cache[--gOp_globals.cache_size];
	}
	*from_cache= true;

	if(new_endpoint) 
	{
		*err= bind_to_protocol(endpoint->type, &endpoint->library_file, &new_endpoint->connection);
		if(!(*err))
		{
			/* Load in the endpoint information.. */
			new_endpoint->cookie= PENDPOINT_COOKIE;
			new_endpoint->type= endpoint->type;
			new_endpoint->state= _state_unknown;
			new_endpoint->parent= NULL;

			new_endpoint->library_file= endpoint->library_file;
			new_endpoint->NMGetModuleInfo= endpoint->NMGetModuleInfo;
			new_endpoint->NMGetConfig= endpoint->NMGetConfig;
			new_endpoint->NMGetConfigLen= endpoint->NMGetConfigLen;
			new_endpoint->NMOpen= endpoint->NMOpen;
			new_endpoint->NMClose= endpoint->NMClose;
			new_endpoint->NMAcceptConnection= endpoint->NMAcceptConnection;
			new_endpoint->NMRejectConnection= endpoint->NMRejectConnection;
			new_endpoint->NMSendDatagram= endpoint->NMSendDatagram;
			new_endpoint->NMReceiveDatagram= endpoint->NMReceiveDatagram;
			new_endpoint->NMSend= endpoint->NMSend;
			new_endpoint->NMReceive= endpoint->NMReceive;
			new_endpoint->NMSetTimeout= endpoint->NMSetTimeout;
			new_endpoint->NMIsAlive= endpoint->NMIsAlive;
			new_endpoint->NMFreeAddress= endpoint->NMFreeAddress;
			new_endpoint->NMGetAddress= endpoint->NMGetAddress;
			new_endpoint->NMFunctionPassThrough= endpoint->NMFunctionPassThrough;
			new_endpoint->NMIdle= endpoint->NMIdle;

			new_endpoint->NMStopAdvertising  = endpoint->NMStopAdvertising;
			new_endpoint->NMStartAdvertising = endpoint->NMStartAdvertising;

			new_endpoint->NMEnterNotifier  = endpoint->NMEnterNotifier;
			new_endpoint->NMLeaveNotifier  = endpoint->NMLeaveNotifier;
			
			new_endpoint->NMGetIdentifier = endpoint->NMGetIdentifier;

			new_endpoint->next= NULL;

			add_endpoint_to_loaded_list(new_endpoint);
		}
		else
		{
			DEBUG_PRINT("Err %d binding to protocol", *err);

			op_assert(gOp_globals.cache_size+1<=MAXIMUM_CACHED_ENDPOINTS);
			gOp_globals.endpoint_cache[gOp_globals.cache_size++]= new_endpoint;
			new_endpoint= NULL;
		}
	} else {
		*err= kNMOutOfMemoryErr;
	}
#elif defined(OP_API_NETWORK_SOCKETS)
	new_endpoint = (Endpoint*)new_pointer(sizeof(Endpoint));

	if(new_endpoint) 
	{
		*err= bind_to_protocol(endpoint->type, &endpoint->library_file, &new_endpoint->connection);
		if(!(*err))
		{
			/* Load in the endpoint information.. */
			new_endpoint->cookie= PENDPOINT_COOKIE;
			new_endpoint->type= endpoint->type;
			new_endpoint->state= _state_unknown;
			new_endpoint->parent= NULL;

			new_endpoint->library_file= endpoint->library_file;
			new_endpoint->NMGetModuleInfo= endpoint->NMGetModuleInfo;
			new_endpoint->NMGetConfig= endpoint->NMGetConfig;
			new_endpoint->NMGetConfigLen= endpoint->NMGetConfigLen;
			new_endpoint->NMOpen= endpoint->NMOpen;
			new_endpoint->NMClose= endpoint->NMClose;
			new_endpoint->NMAcceptConnection= endpoint->NMAcceptConnection;
			new_endpoint->NMRejectConnection= endpoint->NMRejectConnection;
			new_endpoint->NMSendDatagram= endpoint->NMSendDatagram;
			new_endpoint->NMReceiveDatagram= endpoint->NMReceiveDatagram;
			new_endpoint->NMSend= endpoint->NMSend;
			new_endpoint->NMReceive= endpoint->NMReceive;
			new_endpoint->NMSetTimeout= endpoint->NMSetTimeout;
			new_endpoint->NMIsAlive= endpoint->NMIsAlive;
			new_endpoint->NMFreeAddress= endpoint->NMFreeAddress;
			new_endpoint->NMGetAddress= endpoint->NMGetAddress;
			new_endpoint->NMFunctionPassThrough= endpoint->NMFunctionPassThrough;
			new_endpoint->NMIdle= endpoint->NMIdle;

			new_endpoint->NMStopAdvertising  = endpoint->NMStopAdvertising;
			new_endpoint->NMStartAdvertising = endpoint->NMStartAdvertising;

			new_endpoint->NMEnterNotifier  = endpoint->NMEnterNotifier;
			new_endpoint->NMLeaveNotifier  = endpoint->NMLeaveNotifier;

			new_endpoint->NMGetIdentifier = endpoint->NMGetIdentifier;

			new_endpoint->next= NULL;

			add_endpoint_to_loaded_list(new_endpoint);
		}
		else
		{
			DEBUG_PRINT("Err %d binding to protocol", *err);

			new_endpoint= NULL;
		}
	} else {
		*err= kNMOutOfMemoryErr;
	}
#else
	#error create_endpoint_for_accept not defined for this platform
#endif

	if(new_endpoint)
	{
		new_endpoint->parent= endpoint;
	}

	return new_endpoint;
}

#if (DEBUG)
#define DO_CASE(a) case a: return #a; break

char* GetOPErrorName(NMErr err)
{
	switch (err){
		DO_CASE(kNMOutOfMemoryErr);
		DO_CASE(kNMParameterErr);
		DO_CASE(kNMTimeoutErr);
		DO_CASE(kNMInvalidConfigErr);
		DO_CASE(kNMNewerVersionErr);
		DO_CASE(kNMOpenFailedErr);
		DO_CASE(kNMAcceptFailedErr);
		DO_CASE(kNMConfigStringTooSmallErr);
		DO_CASE(kNMResourceErr);
		DO_CASE(kNMAlreadyEnumeratingErr);
		DO_CASE(kNMNotEnumeratingErr);
		DO_CASE(kNMEnumerationFailedErr);
		DO_CASE(kNMNoDataErr);
		DO_CASE(kNMProtocolInitFailedErr);
		DO_CASE(kNMInternalErr);
		DO_CASE(kNMMoreDataErr);
		DO_CASE(kNMUnknownPassThrough);
		DO_CASE(kNMAddressNotFound);
		DO_CASE(kNMAddressNotValidYet);
		DO_CASE(kNMWrongModeErr);
		DO_CASE(kNMBadStateErr);
		DO_CASE(kNMTooMuchDataErr);
		DO_CASE(kNMCantBlockErr);
		DO_CASE(kNMFlowErr);
		DO_CASE(kNMProtocolErr);
		DO_CASE(kNMModuleInfoTooSmall);
		DO_CASE(kNMClassicOnlyModuleErr);
		DO_CASE(kNMNoError);
		DO_CASE(kNMBadPacketDefinitionErr);
		DO_CASE(kNMBadShortAlignmentErr);
		DO_CASE(kNMBadLongAlignmentErr);
		DO_CASE(kNMBadPacketDefinitionSizeErr);
		DO_CASE(kNMBadVersionErr);
		DO_CASE(kNMNoMoreNetModulesErr);
		DO_CASE(kNMModuleNotFoundErr);
		DO_CASE(kNMFunctionNotBoundErr);
		default: return "unknown OP error"; break;
	}
}
#endif //DEBUG
