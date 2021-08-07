/* 
 *-------------------------------------------------------------
 * Description:
 *   Functions which handle communication
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

#include <signal.h>
#include <fcntl.h>

#include "Openplay.h"
#include "OPUtils.h"
#include "configuration.h"
#include "configfields.h"
#include "ip_enumeration.h"

#ifndef __NETMODULE__
#include 			"NetModule.h"
#endif
#include "tcp_module.h"

#ifdef OP_API_NETWORK_SOCKETS
	#include <sys/time.h>
	#include <pthread.h>

	// this hacks in minimal support for EAGAIN for asynchronous communications
	#define HACKY_EAGAIN
#endif

// -------------------------------  Private Definitions

//since we are multithreaded, we have to use a mutual-exclusion locks for certain items

//for locking callbacks -
#define TRY_ENTER_NOTIFIER() machine_acquire_lock(notifierLock)
#define ENTER_NOTIFIER() while (machine_acquire_lock(notifierLock) == false) {}
#define LEAVE_NOTIFIER() machine_clear_lock(notifierLock)

//locks the endpoint list - you must lock this when adding or removing endpoints, and increment endpointListState whenever you change it (while locked of course)
#define TRY_LOCK_ENDPOINT_LIST() machine_acquire_lock(endpointListLock)
#define TRY_LOCK_ENDPOINT_WAITING_LIST() machine_acquire_lock(endpointWaitingListLock)
#define LOCK_ENDPOINT_LIST() {while (machine_acquire_lock(endpointListLock) == false) {}}
#define LOCK_ENDPOINT_WAITING_LIST() {while (machine_acquire_lock(endpointWaitingListLock) == false) {}}
#define UNLOCK_ENDPOINT_LIST() {machine_clear_lock(endpointListLock);}
#define UNLOCK_ENDPOINT_WAITING_LIST() {machine_clear_lock(endpointWaitingListLock);}

#define MARK_ENDPOINT_AS_VALID(e, t) ((e)->valid_endpoints |= (1<<(t)))

//	------------------------------	Private Functions
static NMBoolean 	internally_handle_read_data(NMEndpointRef endpoint, NMSInt16 type);
NMBoolean processEndpoints(NMBoolean block);
void processEndPointSocket(NMEndpointPriv *theEndPoint, long socketType, fd_set *input_set, fd_set *output_set, fd_set *exc_set);
void receive_udp_port(NMEndpointRef endpoint);
static NMBoolean internally_handled_datagram(NMEndpointRef endpoint);
static long socketReadResult(NMEndpointRef endpoint,int socketType);


//  ------------------------------  Private Variables

#if (OP_API_NETWORK_WINSOCK)
	NMBoolean	winSockRunning = false;
#endif

static int wakeSocket;
static int wakeHostSocket;

//for notifier locks
static long notifierLockCount = 0;

/*stuff for our worker thread*/
#if (USE_WORKER_THREAD)
	NMBoolean workerThreadAlive = false;
	NMBoolean dieWorkerThread = false;
	
	#ifdef OP_API_NETWORK_SOCKETS
		pthread_t	worker_thread;
	#elif defined(OP_API_NETWORK_WINSOCK)
		DWORD	worker_thread;
		HANDLE	worker_thread_handle;
	#endif
#endif

/* 
 * Static Function: _lookup_machine
 *--------------------------------------------------------------------
 * Parameters:
 *  [IN]  machine = name of machine to resolve
 *  [IN]  default_port = port number to include in final address
 *  [IN/OUT]  hostAddr = structure to contain resolved information  
 *
 * Returns:
 *   0 = failed to get IP address for name
 *   1 = successfully got IP address for name
 *
 * Description:
 *   Function to lookup the IP address of a specified machine name.
 *   Expects and returns in network order.
 *
 *--------------------------------------------------------------------
 */

static int _lookup_machine(char *machine, unsigned short default_port, struct sockaddr_in *hostAddr)
{
  int  gotAddr = 0;
  char trimmedHost[256];
  char *colon_pos;              /* pointer to rightmost colon in name */
  unsigned short  needPort;     /* port we need */
  struct hostent  *hostInfo;
  unsigned long	  hostIPAddr;

	DEBUG_ENTRY_EXIT("_lookup_machine");


  if (!hostAddr)
    return(0);


  /* Take port numbers on the end of host names/numbers
   * This does nothing in the context of OpenPlay, since we always specify the port
   * number separately. This will let you paste in a host name with a colon and
   * safely ignore it.
   */
  needPort = default_port;

  strcpy(trimmedHost, machine);

  /* find first colon */
  colon_pos = strchr(trimmedHost, ':');

  /* everything after that is the port number, if we found it */
  if (colon_pos)
  {
    *colon_pos++ = 0;  /* strip the port spec and pass to next char */
    needPort = htons((short) atoi(colon_pos));
  }
  
  hostInfo = gethostbyname(trimmedHost);

  if (hostInfo)
  {
    /* Got an ip address by resolving name */
    hostIPAddr = *(unsigned long *) (hostInfo->h_addr_list[0]);

    gotAddr = 1;
  }
  else if (INADDR_NONE != (hostIPAddr = inet_addr(trimmedHost)))
    gotAddr = 1;

  if (gotAddr)
  {
    hostAddr->sin_family = AF_INET;
    hostAddr->sin_port = needPort;
    hostAddr->sin_addr.s_addr = hostIPAddr;
  }

  return gotAddr;
} /* _lookup_machine */

//creates a datagram and stream socket on the same port - retries if necessary until success is achieved
static NMErr _create_sockets(int *sockets, word required_port, NMBoolean active)
{
	NMErr status = kNMNoError;
	struct sockaddr_in address;
	posix_size_type size = sizeof(address);
	int opt = true;

	DEBUG_ENTRY_EXIT("_create_sockets");
	
	int datagramSockets[64]; //if we dont get it in 64 tries, we dont deserve to get it...
	int streamSocket;
	long counter = 0;
	
	while (true)
	{
		//make a datagram socket, bind it to any port, and try to make a stream socket on that port.
		//if we don't get the same port, leave it bound for now and move to the next (otherwise we might get same one repeatedly)
		datagramSockets[counter] = socket(PF_INET,SOCK_DGRAM,0);
		op_assert(datagramSockets[counter]);
		
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = INADDR_ANY;
		address.sin_port = htons(required_port);
		
		//so we can bind to it even if its just been used
		if (required_port)
		{
			status = setsockopt(datagramSockets[counter], SOL_SOCKET, SO_REUSEADDR,(char*)&opt, sizeof(opt));
			DEBUG_NETWORK_API("setsockopt for reuse address", status);
		}
		
		status = bind(datagramSockets[counter],(sockaddr*)&address,sizeof(address));
		if (!status)
		{	
			status = getsockname(datagramSockets[counter], (sockaddr*) &address, &size);
			if (!status)
			{	
				DEBUG_PRINT("bound a datagram socket to port %d; trying stream",ntohs(address.sin_port));
				//weve got a bound datagram socket and its port - now try making a stream socket on the same one
				streamSocket = socket(PF_INET,SOCK_STREAM,0);
				op_assert(streamSocket);
				
				//so we can bind to it even if its just been used
				status = setsockopt(streamSocket, SOL_SOCKET, SO_REUSEADDR,(char*)&opt, sizeof(opt));
				DEBUG_NETWORK_API("setsockopt for reuse address", status);
				
				//why is this only done when not active? just curious...
				if (!active)
				{
					/* set no delay */
					status = setsockopt(streamSocket, IPPROTO_TCP, TCP_NODELAY,(char*)&opt, sizeof(opt));
					DEBUG_NETWORK_API("setsockopt for no delay", status);
				}
				
				//port is already correct from our getsockname call
				address.sin_family = AF_INET;
				address.sin_addr.s_addr = INADDR_ANY;
				
				status = bind(streamSocket,(sockaddr*)&address,sizeof(address));
				if (!status)
				{
					DEBUG_PRINT("bind worked for stream");
					//success! return these two sockets
					sockets[_datagram_socket] = datagramSockets[counter];
					sockets[_stream_socket] = streamSocket;
					//decrement our counter so this most recent one isnt destroyed
					counter--;
					break;
				}
				//if bind failed, kill the stream socket. we only use one of those at once
				else
				{
					DEBUG_NETWORK_API("bind for stream endpoint",status);
					close(streamSocket);
				}
			}			
		}
		//if we required a certain port and didnt get it, we fail
		if (required_port)
			break;

		if (counter > 63)
			break; //cry uncle if it goes this far
			
		//well, that one didnt work. increment the counter and try again
		counter++;
	}
	
	//cleanup time - go backwards through our array, closing down all the sockets we made
	while (counter > 0)
	{
		close(datagramSockets[counter]);
		counter--;
	}
	
	if (status == -1)
		status = kNMOpenFailedErr;
	return status;
}
/* 
 * Static Function: _setup_socket
 *--------------------------------------------------------------------
 * Parameters:
 *  [IN] Endpoint = 
 *  [IN] socket_index = 
 *  [IN] Data = 
 *  [IN] Size = 
 *  [IN] Flags =   
 *
 * Returns:
 *  
 *
 * Description:
 *   Function 
 *
 *--------------------------------------------------------------------
 */

static NMErr _setup_socket(	NMEndpointRef 			new_endpoint, 
							int						index,
                            struct sockaddr_in *	hostAddr, 
                            NMBoolean 				Active, 
                            int *					preparedSockets)
{
	int socket_types[] = {SOCK_DGRAM, SOCK_STREAM};
	int status = kNMNoError;
		
	DEBUG_ENTRY_EXIT("_setup_socket");


	// sjb 19990330 either we aren't active meaning we don't need a remote address
	//              or we better be passed an address
	op_assert((Active == 0) || (0 != hostAddr));

	//use a prepared socket, or make our own
	if (preparedSockets)
		new_endpoint->sockets[index] = preparedSockets[index];
	else
		new_endpoint->sockets[index] = socket(PF_INET, socket_types[index], 0);

	if (new_endpoint->sockets[index] != INVALID_SOCKET)
	{
		//if it wasnt prepared, set up the socket and bind it
		if (preparedSockets == NULL)
		{
			struct sockaddr_in sin;
			int opt = true;
		
			sin.sin_family = AF_INET;
			sin.sin_addr.s_addr = INADDR_ANY;
			sin.sin_port = htons(0);
			
			if ((Active == 0) && (index == _stream_socket))
			{
				/* set no delay */
				status = setsockopt(new_endpoint->sockets[index], IPPROTO_TCP, TCP_NODELAY,(char*)&opt, sizeof(opt));
				DEBUG_NETWORK_API("setsockopt for no delay", status);
			}

			if (index == _datagram_socket)
				DEBUG_PRINT("binding datagram socket to port %d",ntohs(sin.sin_port));
			else
				DEBUG_PRINT("binding stream socket to port %d",ntohs(sin.sin_port));
			status = bind(new_endpoint->sockets[index], (sockaddr*)&sin, sizeof(sin));
		}
		if (!status)
		{
			sockaddr_in address;
			posix_size_type size = sizeof(address);
					
			// get the host and port...
			status = getsockname(new_endpoint->sockets[index], (sockaddr*) &address, &size);
			if (!status)
			{
				new_endpoint->host= ntohl(address.sin_addr.s_addr);
				new_endpoint->port= ntohs(address.sin_port);
				
				//*required_port = new_endpoint->port;
				if (index == _datagram_socket)
					DEBUG_PRINT("have a legal datagram socket on port %d",new_endpoint->port);
				else
					DEBUG_PRINT("have a legal stream socket on port %d",new_endpoint->port);
			
				//set the endpoint to be non-blocking
				SetNonBlockingMode(new_endpoint->sockets[index]);
			}
			else
				DEBUG_NETWORK_API("getsockname",status);
									
			if (!status)
			{
				//we dont want the datagram socket to connect() in netsprocket mode because
				//it won't be sending and receiving to the same port (and connect locks it to one)
				//(we send data to the port we connect to, but they use a random port to send us data)
				if ((Active) && ((new_endpoint->netSprocketMode == false) || (index == _stream_socket)))
				{
					status = connect(new_endpoint->sockets[index], (sockaddr*)hostAddr, sizeof(*hostAddr));

					//if we get EINPROGRESS, it just means the socket can't be created immediately, but that
					//we can select() for completion by selecting the socket for writing...
					if (status)
					{
						if ((op_errno == EINPROGRESS) || (op_errno == EWOULDBLOCK))
						{
							if (index == _datagram_socket)
								DEBUG_PRINT("got EINPROGRESS from connect() on datagram socket - will need to select() for completion");
							else
								DEBUG_PRINT("got EINPROGRESS from connect() on stream socket - will need to select() for completion");
							status = 0;							
						}
						else
							DEBUG_NETWORK_API("connect()",status);
					}
					else //we can go ahead and mark this socket as valid
					{
						MARK_ENDPOINT_AS_VALID(new_endpoint,index);
						new_endpoint->flowBlocked[index] = false;
					}
				}
				else if (index == _stream_socket)
				{
					status = listen(new_endpoint->sockets[index], MAXIMUM_OUTSTANDING_REQUESTS);  
					if (status != 0)
						DEBUG_NETWORK_API("listen()",status);
				}
			}
		}
		else
		{
			DEBUG_NETWORK_API("Bind Socket", status);
			status = kNMOpenFailedErr;
		}
	}
	else
	{
		DEBUG_NETWORK_API("Creating Socket",status);
		
		//standard error codes
		status = kNMOpenFailedErr;
	}
	
    return(status);
}


/* 
 * Static Function: _create_endpoint
 *--------------------------------------------------------------------
 * Parameters:
 *  [IN] hostInfo = 
 *  [IN] Callback = 
 *  [IN] Context = 
 *  [OUT] Endpoint = 
 *  [IN] Active =
 *  [IN] create_sockets = 
 *  [IN] connectionMode = 
 *  [IN] version = 
 *  [IN] gameID = 
 *
 * Returns:
 *  
 *
 * Description:
 *   Function 
 *
 *--------------------------------------------------------------------
 */

static NMErr 
_create_endpoint(
	sockaddr_in *hostInfo, 
	NMEndpointCallbackFunction *Callback,
	void *Context,
	NMEndpointRef *Endpoint,
	NMBoolean Active,
	NMBoolean create_sockets,
	long connectionMode,
	NMBoolean netSprocketMode,
	unsigned long version,
	unsigned long gameID)
{
	NMEndpointRef new_endpoint;
	int index;
	NMErr err = 0;

	DEBUG_ENTRY_EXIT("_create_endpoint");

		
	new_endpoint = (NMEndpointRef)calloc(1, sizeof(struct NMEndpointPriv));


	if (!new_endpoint)
		return(kNMOutOfMemoryErr);

	if (create_sockets == false)
		DEBUG_PRINT("\n\nCREATING JOINER ENDPOINT (0X%X)\n\n",new_endpoint);
	else if (Active)
		DEBUG_PRINT("\n\nCREATING ACTIVE ENDPOINT (0X%X)\n\n",new_endpoint);
	else
		DEBUG_PRINT("\n\nCREATING PASSIVE ENDPOINT (0X%X)\n\n",new_endpoint);

	machine_mem_zero(new_endpoint, sizeof(NMEndpointPriv));
	
	//both joiner and passive endpoints are passed with active == false, so we distinguish here	
	new_endpoint->listener = false;
	if (!Active)
	{
		if (create_sockets == true)
			new_endpoint->listener = true;
	}
	new_endpoint->parent = NULL;
	new_endpoint->alive = false;
	new_endpoint->cookie = kModuleID;
	new_endpoint->dying = false;
	new_endpoint->needToDie = false;
	new_endpoint->connectionMode = connectionMode;
	new_endpoint->netSprocketMode= netSprocketMode;
	new_endpoint->advertising = false;
	new_endpoint->timeout  = DEFAULT_TIMEOUT;
	new_endpoint->callback = Callback;
	new_endpoint->user_context = Context;
	if ((new_endpoint->netSprocketMode) || (new_endpoint->listener == true))
	//if ((new_endpoint->netSprocketMode) || (Active == false))
		new_endpoint->dynamically_assign_remote_udp_port = false;
	else
		new_endpoint->dynamically_assign_remote_udp_port = ((connectionMode & _uber_connection) == _uber_connection);
	new_endpoint->valid_endpoints = 0;
	new_endpoint->version = version;
	new_endpoint->gameID = gameID;
	new_endpoint->active = Active;
	new_endpoint->opening_error = 0;

	for (index = 0; index < NUMBER_OF_SOCKETS; ++index)
	{
		new_endpoint->sockets[index] = INVALID_SOCKET;
		new_endpoint->flowBlocked[index] = true; //gotta wait till it says we can write
		new_endpoint->newDataCallbackSent[index] = false;
	}
	*Endpoint = new_endpoint;

	if (create_sockets)
 	{ 
 		int preparedSockets[NUMBER_OF_SOCKETS];
		int *preparedSocketsPtr;
 		
		//sometimes we need datagram and stream sockets to be on the same port
		//netsprocket mode does because we don't send our our port number to the connected machine so it has to be predictable
		//we also need it when hosting, because enumeration requests are sent to the datagram socket on that port
		//therefore, we prepare the sockets first-off, since we might not be able to get two of a kind on the first try
		if ((new_endpoint->netSprocketMode) || (new_endpoint->listener))
		{
			word required_port;
			if (Active)
				required_port = 0; //we need two of the same, but they can be any port
			else
				required_port = ntohs(hostInfo->sin_port);//gotta get two of a specific port
				
			err = _create_sockets(preparedSockets,required_port,Active);
			preparedSocketsPtr = preparedSockets;
		}
		else
		{
			preparedSocketsPtr = NULL;
		}

		if (!err)
		{
			for (index = 0; !err && index < NUMBER_OF_SOCKETS; ++index)
			{
				if (new_endpoint->connectionMode & (1 << index))
				{
					struct sockaddr_in  address;
					posix_size_type		addr_size = sizeof(address);

					if (!err)
						err = _setup_socket(new_endpoint, index, hostInfo, Active, preparedSocketsPtr);
				}
			}
		}
	}

	if (err)
	{
		NMClose(new_endpoint, false);
		new_endpoint = NULL;
		*Endpoint = NULL;

		return(err);
	}

	// now we add ourself to our list of live endpoints.
	// the worker thread might be in the middle of a long select() call,
	// so we hop on the waiting list to keep the worker thread from re-acquiring the lock,
	// then attempt to "wake up" out of the select() call to get it to relenquish its current lock,
	// and hopefully grab the lock quickly ourselves

	LOCK_ENDPOINT_WAITING_LIST();
	sendWakeMessage();
	LOCK_ENDPOINT_LIST();
	new_endpoint->next = endpointList;
	endpointList = new_endpoint;
	endpointListState++;
	UNLOCK_ENDPOINT_LIST();
	UNLOCK_ENDPOINT_WAITING_LIST();
	return(kNMNoError);
}

/* 
 * Static Function: _send_data
 *--------------------------------------------------------------------
 * Parameters:
 *  [IN] Endpoint = 
 *  [IN] socket_index = 
 *  [IN] Data = 
 *  [IN] Size = 
 *  [IN] Flags =   
 *
 * Returns:
 *  
 *
 * Description:
 *   Function 
 *
 *--------------------------------------------------------------------
 */

static NMErr _send_data(	NMEndpointRef 		Endpoint, 
							int					socket_index,
                        	void *				Data, 
                        	unsigned long 		Size, 
                        	NMFlags 			Flags)
{
	int result;
	int done = 0;
	int offset = 0;
	unsigned long bytes_to_send = Size;
	unsigned long total_bytes_sent = 0;

	DEBUG_ENTRY_EXIT("_send_data");


	if (Endpoint->sockets[socket_index] == INVALID_SOCKET)
	return(kNMParameterErr);

	do
	{
		//if we're in netsprocket mode and active, we have to supply the address for each send, since
		//we're not associated with an address via connect().  Doing that would not allow us to receive datagrams from
		//the host, since they dont come from that same port.
		if ((socket_index == _datagram_socket) && (Endpoint->netSprocketMode) && (Endpoint->active))
			result = sendto(Endpoint->sockets[socket_index], ((char*)Data) + offset, bytes_to_send,0,(sockaddr*)&Endpoint->remoteAddress,sizeof(Endpoint->remoteAddress));
		else
			result = send(Endpoint->sockets[socket_index], ((char*)Data) + offset, bytes_to_send, 0);

		/* FIXME - check all result codes */

		if (result == -1)
		{
#ifdef HACKY_EAGAIN
			if ( errno == EAGAIN )
			{
				result = 0;
			}
			else
			{
				DEBUG_NETWORK_API("send",result);
				return(kNMInternalErr);
			}
#else
			DEBUG_NETWORK_API("send",result);
			return(kNMInternalErr);
#endif // HACKY_EAGAIN
		}

		offset += result;
		bytes_to_send -= result;
		total_bytes_sent += result;

    if ((Flags & kNMBlocking) && (total_bytes_sent != Size))
		done = 0;
	else
 		done = 1;

	} while (!done);

	return(total_bytes_sent);

} /* _send_data */


/* 
 * Static Function: _receive_data
 *--------------------------------------------------------------------
 * Parameters:
 *  [IN] Endpoint = 
 *  [IN] the_socket = 
 *  [IN] Data = 
 *  [IN] Size = 
 *  [IN] Flags =   
 *
 * Returns:
 *  
 *
 * Description:
 *   Function 
 *
 *--------------------------------------------------------------------
 */

static NMErr _receive_data(NMEndpointRef inEndpoint, int which_socket, 
                           void *ioData, unsigned long *ioSize, NMFlags *outFlags)
{
	NMErr		err = kNMNoError;
	NMSInt32	result;

	*outFlags = 0;
	op_vassert_return((inEndpoint != NULL),"inEndpoint is NIL!",kNMParameterErr);
    op_vassert_return(inEndpoint->cookie==kModuleID, csprintf(sz_temporary, "cookie: 0x%x != 0x%x", inEndpoint->cookie, kModuleID),kNMParameterErr); 
	op_assert(inEndpoint->sockets[which_socket]!=INVALID_SOCKET);

	if (inEndpoint->needToDie == true)
		return kNMNoDataErr;
#ifdef HACKY_EAGAIN
	SetNonBlockingMode(inEndpoint->sockets[which_socket]);
#endif // HACKY_EAGAIN
	result = recv(inEndpoint->sockets[which_socket], (char *)ioData, *ioSize, 0);

	if (result == 0)
	{
		inEndpoint->needToDie = true;
		return kNMNoDataErr;
	}
	
	//should be more detailed here
	if (result == -1)
	{
		if (op_errno != EWOULDBLOCK)
			DEBUG_NETWORK_API("recv",result);
		err= kNMNoDataErr;
	}
	else {				
		*ioSize = result;
	}

	return err;
} /* _receive_data */


/* 
 * Function: _send_datagram_socket
 *--------------------------------------------------------------------
 * Parameters:
 *  [IN]  Endpoint = 
 *
 * Returns:
 *  
 *
 * Description:
 *   Function 
 *
 *--------------------------------------------------------------------
 */

static void _send_datagram_socket(NMEndpointRef endpoint)
{
	struct sockaddr_in  address;
	int    status;
	NMErr  err;
	posix_size_type size;
	
	DEBUG_ENTRY_EXIT("_send_datagram_socket");
	
	size = sizeof(address);
	
	status = getsockname(endpoint->sockets[_datagram_socket], (sockaddr*)&address, &size);
	
	if (status == 0)
	{
		struct udp_port_struct_from_server  port_data;
		
		port_data.port = address.sin_port;
		DEBUG_PRINT("sending udp port: %d",ntohs(port_data.port));
		
		err = _send_data(endpoint, _stream_socket, (void *)&port_data, sizeof(port_data), 0);
	}
}

//----------------------------------------------------------------------------------------
// internally_handle_read_data
//----------------------------------------------------------------------------------------

static NMBoolean
internally_handle_read_data(NMEndpointRef endpoint, NMSInt16 type)
{
NMBoolean	handled_internally = false;

	DEBUG_ENTRY_EXIT("internally_handle_read_data");

	//if we're dying, dont bother - but return true so its not passed on
	if (endpoint->needToDie)
		return true;
		
	if (type == _stream_socket)
	{
		// if we are an uber endpoint ASSIGN_SOCKET 
		if (endpoint->dynamically_assign_remote_udp_port)
		{
			endpoint->dynamically_assign_remote_udp_port = false;
			
			receive_udp_port(endpoint);
			handled_internally = true;

		}
	}
	else
	{
		
		handled_internally= internally_handled_datagram(endpoint);
	}

	return handled_internally;
}

//----------------------------------------------------------------------------------------
// internally_handled_datagram
//----------------------------------------------------------------------------------------

static NMBoolean
internally_handled_datagram(NMEndpointRef endpoint)
{
	NMBoolean	handled_internally = false;
	NMSInt32	bytes_read;
	NMErr		err = kNMNoError;
	char		packet[kQuerySize + 4]; //add a bit so we know if it goes over the size we're looking for

	bytes_read= recvfrom(endpoint->sockets[_datagram_socket], packet, sizeof (packet), MSG_PEEK, NULL, NULL);

	if (bytes_read == kQuerySize)
	{
		
		if (is_ip_request_packet(packet, bytes_read, endpoint->gameID))
		{
			sockaddr	remote_address;
			char		response_packet[512];
			NMSInt32	bytes_to_send, result;
			posix_size_type   remote_address_size  = sizeof(remote_address);

			DEBUG_PRINT("got an enumeraion-request object");

			// its ours - read it for real and see who its from
			bytes_read = recvfrom(endpoint->sockets[_datagram_socket], (char *) &packet, sizeof (packet),
				0, (sockaddr*) &remote_address, &remote_address_size);

			if (endpoint->advertising)
			{
				DEBUG_PRINT("responding");
				// And respond to it.
				bytes_to_send= build_ip_enumeration_response_packet(response_packet, endpoint->gameID, 
					endpoint->version, endpoint->host, endpoint->port, endpoint->name, 0, NULL);
				op_assert(bytes_to_send<=sizeof (response_packet));

				byteswap_ip_enumeration_packet(response_packet);

				// send!
				result= sendto(endpoint->sockets[_datagram_socket], response_packet, 
					bytes_to_send, 0, (sockaddr*) &remote_address, sizeof (remote_address));

				if (result > 0)
					op_assert(result==bytes_to_send);
				else if (result < 0)
					DEBUG_NETWORK_API("Sendto on enum response",result);
			}
			// we handled it.
			handled_internally= true;
		}

	} 

	return handled_internally;
}

/* 
 * Function: _wait_for_open_complete
 *--------------------------------------------------------------------
 * Parameters:
 *  [IN]  Endpoint = 
 *
 * Returns:
 *  
 *
 * Description:
 *   Function 
 *
 *--------------------------------------------------------------------
 */

static NMErr _wait_for_open_complete(NMEndpointRef Endpoint)
{
	int done = 0;
	long entry_time;
	long elapsed_time = 0;
	long max_wait_time = 10 * MACHINE_TICKS_PER_SECOND;

	DEBUG_ENTRY_EXIT("_wait_for_open_complete");

	// both types of non-active endpoints have stream sockets that are already valid
	// listener endpoints dont need to wait, and a new handed-off socket is already valid too
	if ( !Endpoint->active && (Endpoint->connectionMode & (1 << _stream_socket)) )
		MARK_ENDPOINT_AS_VALID(Endpoint,_stream_socket);
		
	entry_time = machine_tick_count();

	// wait here for stream and datagram socket confirmations - and also the remote udp port if we're not in nsp mode
	while (!done && !Endpoint->opening_error && (elapsed_time < max_wait_time))
	{
		if (((Endpoint->connectionMode & Endpoint->valid_endpoints) == Endpoint->connectionMode)
			&& (Endpoint->dynamically_assign_remote_udp_port == false))
			done = 1;
		
		//if we're running without a worker thread we need to idle ourself
		#if (!USE_WORKER_THREAD)
			NMIdle(Endpoint);
		#endif

		elapsed_time = machine_tick_count() - entry_time;
	} 
	if (done)
		DEBUG_PRINT("_wait_for_open_complete: endpoint successfully constructed");
		
	if (Endpoint->opening_error)
		DEBUG_PRINT("_wait_for_open_complete: opening_error %d",Endpoint->opening_error);
	if (!done)
	{
		NMErr returnValue;
		if (elapsed_time >= max_wait_time)
			DEBUG_PRINT("timed-out waiting for open complete");
		
		Endpoint->alive = false;

		//this gets set if they close before we get a udp port
		if (Endpoint->opening_error)
			returnValue =  Endpoint->opening_error;
		else
			returnValue =  kNMOpenFailedErr;

		NMClose(Endpoint, false);
		
		return returnValue;
	}
	else
		return Endpoint->opening_error;

	return(kNMNoError);
} /* _wait_for_open_complete */


/* 
 * Function: NMOpen
 *--------------------------------------------------------------------
 * Parameters:
 *  [IN]  Config = 
 *  [IN]  Callback = 
 *  [IN]  Context = 
 *  [OUT] Endpoint = 
 *  [IN]  Active =   
 *
 * Returns:
 *  
 *
 * Description:
 *   Function 
 *
 *--------------------------------------------------------------------
 */

NMErr NMOpen(NMConfigRef Config,
             NMEndpointCallbackFunction *Callback, void* Context, 
             NMEndpointRef *Endpoint, NMBoolean Active)
{
	NMErr  err;
	int    status;

	DEBUG_ENTRY_EXIT("NMOpen");

	if (module_inited < 1)
		return kNMInternalErr;

	if (!Config || !Callback || !Endpoint)
		return(kNMParameterErr);

	if (Config->cookie != config_cookie)
		return(kNMInvalidConfigErr);

	//make sure we've inited winsock (doing this duringDllMain causes problems)
	#ifdef OP_API_NETWORK_WINSOCK
		initWinsock();
	#endif

	//make sure a worker-thread is running if need-be (doing this in _init can cause problems)
	#if (USE_WORKER_THREAD)	
		createWorkerThread();
	#endif

	*Endpoint = NULL;

	if (Active)
	{
		if (Config->host_name[0])
		{
			struct sockaddr_in host_info;

			status = _lookup_machine(Config->host_name, Config->hostAddr.sin_port, &host_info);

			if (status)
				memcpy(&(Config->hostAddr), &host_info, sizeof(struct sockaddr_in));
			else
				return(kNMAddressNotFound);
		}
	}

	err = _create_endpoint(&(Config->hostAddr), Callback, Context, Endpoint,
		Active, true, Config->connectionMode, Config->netSprocketMode,
		Config->version, Config->gameID);

	if (!err)
	{
		/* copy the name */
		strcpy((*Endpoint)->name, Config->name);

		err = _wait_for_open_complete(*Endpoint);
	}

	/* FIXME : check that Endpoint can be set NULL here after other calls */
	if (err)
	{
		*Endpoint = NULL;
		return(err);
	}

	//unleash the dogs.  this lets messages start hitting the callback
	DEBUG_PRINT("endpoint 0x%x is now alive",*Endpoint);
	(*Endpoint)->alive = true;

	return(kNMNoError);
} /* NMOpen */


/* 
 * Function: NMClose
 *--------------------------------------------------------------------
 * Parameters:
 *  [IN/OUT] Endpoint = 
 *  [IN] Orderly  =  
 *
 * Returns:
 *  
 *
 * Description:
 *   Function 
 *
 *--------------------------------------------------------------------
 */

NMErr NMClose(NMEndpointRef Endpoint, NMBoolean Orderly)
{
	int index;
	int status;
	struct linger linger_option;

	DEBUG_ENTRY_EXIT("NMClose");

	if (module_inited < 1)
	{
		DEBUG_PRINT("Module not inited in NMClose");
		return kNMInternalErr;
	}

	if (!Endpoint)
	{
		DEBUG_PRINT("NULL Endpoint in NMClose");
		return(kNMParameterErr);
	}

	if (Endpoint->cookie != kModuleID)
	{
		DEBUG_PRINT("Invalid endpoint cookie detected in NMClose");
		return(kNMInternalErr);
	}
	linger_option.l_onoff = 1;
	linger_option.l_linger = 0;

	DEBUG_PRINT("Searching for theEndpoint in NMClose");

	//search for this endpoint on the list, and if its there, remove it
	{
		NMBoolean found = false;
		NMEndpointPriv *theEndpoint;

		theEndpoint = endpointList;

		if (theEndpoint == Endpoint) //we're first on the list
		{
			found = true;
			endpointList = Endpoint->next;
		}
		else while (theEndpoint != NULL)
		{
			if (theEndpoint->next == Endpoint) //its us
			{
				found = true;
				theEndpoint->next = Endpoint->next;
				break;
			}
			theEndpoint = theEndpoint->next;
		}
		if (found)
			endpointListState++;
	}

    	DEBUG_PRINT("Done searching for theEndpoint in NMClose");

	for (index = 0; index < NUMBER_OF_SOCKETS; ++index)
	{
		if (Endpoint->sockets[index] != INVALID_SOCKET)
		{
			if (!Orderly && index != _datagram_socket)
			{
				/* cause socket to discard remaining data and reset on close */
				status = setsockopt(Endpoint->sockets[index], SOL_SOCKET, SO_LINGER,
					(char*)&linger_option, sizeof(struct linger));
			}

    		DEBUG_PRINT("Shutting down a socket in NMClose...");
			/* shutdown the socket, not allowing any further send/receives */
			status = shutdown(Endpoint->sockets[index], 2);

    		DEBUG_PRINT("Closing a socket in NMClose...");
			status = close(Endpoint->sockets[index]);
		} 
	} /* for (index) */

	// notify that it is closed, if necessary
	if (Endpoint->alive)
	{
		Endpoint->alive = false;
    	DEBUG_PRINT("Notifying about closure in NMClose...");
		Endpoint->callback(Endpoint, Endpoint->user_context, kNMCloseComplete, 0, NULL);
	}

	Endpoint->cookie = PENDPOINT_BAD_COOKIE;


	/* FIX ME - why free the endpoint pointer here ? */
	DEBUG_PRINT("Freeing the Endpoint in NMClose...");
	free(Endpoint);

	return(kNMNoError);
} /* NMClose */


/* 
 * Function: NMAcceptConnection
 *--------------------------------------------------------------------
 * Parameters:
 *  [IN] Endpoint
 *  [IN] Cookie
 *  [IN] Callback
 *  [IN] Context
 *
 * Returns:
 *  
 *
 * Description:
 *   Function 
 *
 *--------------------------------------------------------------------
 */

NMErr
NMAcceptConnection(
	NMEndpointRef				inEndpoint, 
	void						*inCookie, 
	NMEndpointCallbackFunction	*inCallback, 
	void						*inContext)
{
	DEBUG_ENTRY_EXIT("NMAcceptConnection");
	NMErr			err;
	NMEndpointRef	new_endpoint;

	UNUSED_PARAMETER(inCookie)

	if (module_inited < 1)
		return kNMInternalErr;

	op_vassert_return((inCallback != NULL),"Callback is NULL!",kNMParameterErr);
	op_vassert_return((inEndpoint != NULL),"inEndpoint is NIL!",kNMParameterErr);
    op_vassert_return(inEndpoint->cookie==kModuleID, csprintf(sz_temporary, "cookie: 0x%x != 0x%x", inEndpoint->cookie, kModuleID),kNMParameterErr); 

    // create all the data...
    
    err = _create_endpoint(0, inCallback, inContext, &new_endpoint, false, false, inEndpoint->connectionMode,
			  inEndpoint->netSprocketMode, inEndpoint->version, inEndpoint->gameID);

	if (! err)
	{
		new_endpoint->parent = inEndpoint;
		
   		// create the sockets..
		sockaddr_in	remote_address;
		posix_size_type	remote_length  = sizeof(remote_address);

 		// make the accept call...
		DEBUG_PRINT("calling accept()");
		new_endpoint->sockets[_stream_socket]= accept(inEndpoint->sockets[_stream_socket], (sockaddr*) &remote_address, 
													   &remote_length);

		DEBUG_PRINT("the remote address is %d",ntohs(remote_address.sin_port));
		if (new_endpoint->sockets[_stream_socket] != INVALID_SOCKET)
		{
			sockaddr_in	address;
			NMUInt16	required_port;
			posix_size_type     size  = sizeof(address);

			err = getpeername(new_endpoint->sockets[_stream_socket], (sockaddr*) &address, &size);

	  		if (!err)
			{
		 		required_port = 0;

				     // if we need a datagram socket
		      	if (inEndpoint->connectionMode & 1)
		 		{
					// create the datagram socket with the default port and host set..
					err= _setup_socket(new_endpoint, _datagram_socket, &address, true,NULL);
		     	}
			}
	  		else
	      		DEBUG_NETWORK_API("Getsockname", err);
		}
		else
			DEBUG_NETWORK_API("Accept (for Accept)", err);

		if (err)
		{
			//we're not really alive - we dont wanna send a close complete
			new_endpoint->alive = false;
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
				_send_datagram_socket(new_endpoint);
			}
		}

		// and the async calls...
		// This calls back the original endpoints callback proc, which may or may not be what you are expecting.
		// (It wasn't what I was expecting the first time)

		// CRT, Sep 24, 2000
		// There are 2 async calls here:  (1)  The kNMAcceptComplete callback is to be called to the new endpoint,
		// and (2) the kNMHandoffComplete is called to the original endpoint, which is the listener...

		
		
	}
	else
		DEBUG_NETWORK_API("Create Endpoint (for Accept)", err);

	return err;

} // NMAcceptConnection


/* 
 * Function: NMRejectConnection
 *--------------------------------------------------------------------
 * Parameters:
 *  [IN] Endpoint =
 *  [IN] Cookie   =  
 *
 * Returns:
 *  
 *
 * Description:
 *   Function 
 *
 *--------------------------------------------------------------------
 */

NMErr NMRejectConnection(NMEndpointRef Endpoint, void *Cookie)
{
	int    closing_socket;
	struct sockaddr_in remote_address;
	posix_size_type remote_length = sizeof(remote_address);
	
	DEBUG_ENTRY_EXIT("NMRejectConnection");
	
	UNUSED_PARAMETER(Cookie);
	
	if (module_inited < 1)
		return kNMInternalErr;
	
	
	if (!Endpoint)
		return(kNMParameterErr);
	
	if (Endpoint->cookie != kModuleID)
		return(kNMInternalErr);
	
	DEBUG_PRINT("calling accept()");
	closing_socket = accept(Endpoint->sockets[_stream_socket], 
		(sockaddr*)&remote_address, &remote_length);
	
	if (closing_socket != INVALID_SOCKET)
	{
		struct linger linger_option;
		int status;
		
		linger_option.l_onoff = 1;
		linger_option.l_linger = 0;
		
		status = setsockopt(closing_socket, SOL_SOCKET, SO_LINGER,
			(char*)&linger_option, sizeof(struct linger)); 
		
		if (status != 0)
			return(op_errno);
		
		status = shutdown(closing_socket, 2);
		
		if (status != 0)
			return(op_errno);
		
		status = close(closing_socket);
		
		if (status != 0)
			return(op_errno);
	}
	
	return(kNMNoError);
} /* NMRejectConnection */


/* 
 * Function: NMIsAlive
 *--------------------------------------------------------------------
 * Parameters:
 *  [IN] Endpoint =  
 *
 * Returns:
 *   true  = connection is alive
 *   false = connection is not alive
 *
 * Description:
 *   Function to return alive status.
 *
 *--------------------------------------------------------------------
 */

NMBoolean NMIsAlive(NMEndpointRef Endpoint)
{

	if (module_inited < 1)
		return false;

	if (!Endpoint || Endpoint->cookie != kModuleID)
		return(false);

	return(Endpoint->alive);
} /* NMIsAlive */


/* 
 * Function: NMSetTimeout
 *--------------------------------------------------------------------
 * Parameters:
 *  [IN] Endpoint = 
 *  [IN] Timeout =  
 *
 * Returns:
 *  
 *
 * Description:
 *   Function to set timeout in milliseconds
 *
 *--------------------------------------------------------------------
 */

NMErr NMSetTimeout(NMEndpointRef Endpoint, unsigned long Timeout)
{

	DEBUG_ENTRY_EXIT("NMSetTimeout");

	if (module_inited < 1)
		return kNMInternalErr;

	DEBUG_PRINT("setting timeout to %d",Timeout);
	
	if (!Endpoint)
		return(kNMParameterErr);
	
	if (Endpoint->cookie != kModuleID)
		return(kNMInternalErr);
	
	Endpoint->timeout = Timeout;
	
	return(kNMNoError);
} /* NMSetTimeout */


/* 
 * Function: NMGetIdentifier
 *--------------------------------------------------------------------
 * Parameters:
 *  [IN] Endpoint = 
 *  [OUT] IdStr =  
 *  [IN] MaxLen = 
 *
 * Returns:
 *  identifier for remote endpoint (dotted IP address) in outIdStr
 *
 *--------------------------------------------------------------------
 */

NMErr
NMGetIdentifier(NMEndpointRef inEndpoint,  char * outIdStr, NMSInt16 inMaxLen)
{
	int the_socket;

	DEBUG_ENTRY_EXIT("NMGetIdentifier");

	if (module_inited == false)
		return kNMInternalErr;
	
	if (!inEndpoint)
		return(kNMParameterErr);
	
	if (!outIdStr)
		return(kNMParameterErr);

	if (inMaxLen < 1)
		return(kNMParameterErr);

	if (inEndpoint->cookie != kModuleID)
		return(kNMInternalErr);
	
    the_socket = inEndpoint->sockets[_stream_socket];
    sockaddr_in   remote_address;
    int			remote_length = sizeof(remote_address);
    char result[256];
    
    getpeername(the_socket, (sockaddr*)&remote_address, &remote_length);
    
    in_addr addr = remote_address.sin_addr;

    sprintf(result, "%s", inet_ntoa(addr));
	
    strncpy(outIdStr, result, inMaxLen - 1);
    outIdStr[inMaxLen - 1] = 0;

	return (kNMNoError);
}

/* 
 * Function: NMIdle
 *--------------------------------------------------------------------
 * Parameters:
 *  [IN] Endpoint = 
 *
 * Returns:
 *  
 *
 * Description:
 *   Function which does nothing.
 *
 *--------------------------------------------------------------------
 */

NMErr NMIdle(NMEndpointRef Endpoint)
{

	//DEBUG_ENTRY_EXIT("NMIdle");

	if (module_inited < 1)
		return kNMInternalErr;

	//we call this passing NULL sometimes....  should be up to OP to keep NULL endpoints out
	if (Endpoint)
	{
		if (Endpoint->cookie != kModuleID)
			return(kNMInternalErr);
	}

	//if we're not using a worker thread, here is where we
	//process messages
	#if (!USE_WORKER_THREAD)
	
		//while something new is coming in, keep chugging.
		//stop after 10, however, so we dont get stuck in a loop if something
		//goes wrong at least..
		long counter = 0;
		while ((processEndpoints(false) == true) && (counter < 10)) { counter++; }
	#endif
		
  	return(kNMNoError);
} /* NMIdle */


/* 
 * Function: NMFunctionPassThrough
 *--------------------------------------------------------------------
 * Parameters:
 *  [IN] Endpoint = 
 *  [IN] Selector = 
 *  [IN] ParamBlock
 *
 * Returns:
 *  
 *
 * Description:
 *   Function 
 *
 *--------------------------------------------------------------------
 */

NMErr NMFunctionPassThrough(NMEndpointRef Endpoint, unsigned long Selector, void *ParamBlock)
{

	DEBUG_ENTRY_EXIT("NMFunctionPassThrough");

	if (module_inited < 1)
		return kNMInternalErr;

	if (!Endpoint || !ParamBlock)
		return(kNMParameterErr);

	if (Endpoint->cookie != kModuleID)
		return(kNMInternalErr);

	switch(Selector)
	{
		case _pass_through_set_debug_proc:
			Endpoint->status_proc = (status_proc_ptr) ParamBlock;
			break;

		default:
			return(kNMUnknownPassThrough);
			break;
	}

	return(kNMNoError);
} /* NMFunctionPassThrough */


/* 
 * Function: NMSendDatagram
 *--------------------------------------------------------------------
 * Parameters:
 *  [IN] Endpoint = 
 *  [IN] Data = 
 *  [IN] Size = 
 *  [IN] Flags = 
 *
 * Returns:
 *   See _send_data().
 *
 * Description:
 *   Function to send a datagram.
 *
 *--------------------------------------------------------------------
 */

NMErr NMSendDatagram(NMEndpointRef Endpoint, NMUInt8 *Data, unsigned long Size, NMFlags Flags)
{
	DEBUG_ENTRY_EXIT("NMSendDatagram");

	if (module_inited < 1)
		return kNMInternalErr;

	long result;

	if (!Endpoint || !Data)
		return(kNMParameterErr);

	if (Endpoint->cookie != kModuleID)
		return(kNMInternalErr);

	result = _send_data(Endpoint, _datagram_socket, Data, Size, Flags);

	//if its less than 0 its an error. we don't return size for datagrams.
	//it its size is not what we sent, however, we return a flow control error
	if (result < 0)
		return(result);
	else
	{
		if (result != Size)
		{
			Endpoint->flowBlocked[_datagram_socket] = true; //let em know when they can go again
			return kNMFlowErr;
		}
		return 0;
	}
} /* NMSendDatagram */


/* 
 * Function: NMReceiveDatagram
 *--------------------------------------------------------------------
 * Parameters:
 *  [IN] Endpoint = 
 *  [IN/OUT] Data = 
 *  [IN/OUT] Size = 
 *  [IN/OUT] Flags = 
 *
 * Returns:
 *    See _receive_data().
 *
 * Description:
 *   Function to receive a datagram.
 *
 *--------------------------------------------------------------------
 */

NMErr NMReceiveDatagram(NMEndpointRef Endpoint, NMUInt8 *Data, unsigned long *Size, NMFlags *Flags)
{

	DEBUG_ENTRY_EXIT("NMReceiveDatagram");

	if (module_inited < 1)
		return kNMInternalErr;

	Endpoint->newDataCallbackSent[_datagram_socket] = false; //we should start telling them of incoming data again

	NMErr err;

	if (!Endpoint || !Data || !Size || !Flags)
		return(kNMParameterErr);

	if (Endpoint->cookie != kModuleID)
		return(kNMInternalErr);

	err = _receive_data(Endpoint, _datagram_socket, Data, Size, Flags);

	return(err);
} /* NMReceiveDatagram */


/* 
 * Function: NMSend
 *--------------------------------------------------------------------
 * Parameters:
 *  [IN] Endpoint = 
 *  [IN/OUT] Data = 
 *  [IN/OUT] Size = 
 *  [IN/OUT] Flags = 
 *
 * Returns:
 *   See _send_data(). 
 *
 * Description:
 *   Function to send a stream packet.
 *
 *--------------------------------------------------------------------
 */

NMErr NMSend(NMEndpointRef Endpoint, void *Data, unsigned long Size, NMFlags Flags)
{
	DEBUG_ENTRY_EXIT("NMSend");

	if (module_inited < 1)
		return kNMInternalErr;


	long result;

	if (!Endpoint || !Data)
		return(kNMParameterErr);

	if (Endpoint->cookie != kModuleID)
		return(kNMInternalErr);

	result = _send_data(Endpoint, _stream_socket, Data, Size, Flags);

	//if its a positive return value (not an error) and not the same as they requested,
	//we're flow blocked - start looking for a flow clear
	if ((result != Size) && (result > 0))
		Endpoint->flowBlocked[_stream_socket] = true; //let em know when they can go again

	return(result);
} /* NMSend */


/* 
 * Function: NMReceive
 *--------------------------------------------------------------------
 * Parameters:
 *  [IN] Endpoint = 
 *  [IN/OUT] Data = 
 *  [IN/OUT] Size = 
 *  [IN/OUT] Flags = 
 *
 * Returns:
 *   See _receive_data().
 *
 * Description:
 *   Function to receive a stream packet.
 *
 *--------------------------------------------------------------------
 */

NMErr NMReceive(NMEndpointRef Endpoint, void *Data, unsigned long *Size, NMFlags *Flags)
{

	DEBUG_ENTRY_EXIT("NMReceive");

	if (module_inited < 1)
		return kNMInternalErr;

	Endpoint->newDataCallbackSent[_stream_socket] = false; //we should start telling them of incoming data again
	
	NMErr err;

	if (!Endpoint || !Data || !Size || !Flags)
		return(kNMParameterErr);

	if (Endpoint->cookie != kModuleID)
		return(kNMInternalErr);

	err = _receive_data(Endpoint, _stream_socket, Data, Size, Flags);

	return(err);
} /* NMReceive */


//----------------------------------------------------------------------------------------
//	Calls the "Enter Notifier" function on the requested endpoint (stream or datagram).
//----------------------------------------------------------------------------------------

NMErr
NMEnterNotifier(NMEndpointRef inEndpoint, NMEndpointMode endpointMode)
{
	DEBUG_ENTRY_EXIT("NMEnterNotifier");

	UNUSED_PARAMETER(inEndpoint);
	UNUSED_PARAMETER(endpointMode);
	
	if (module_inited < 1)
		return kNMInternalErr;

	//we're a wee bit sloppy here - whenever anyone calls this, we halt any callbacks.
	if (notifierLockCount == 0)
	ENTER_NOTIFIER();
	notifierLockCount++;
	return kNMNoError;
}


//----------------------------------------------------------------------------------------
//	Calls the "Leave Notifier" function on the requested endpoint (stream or datagram).
//----------------------------------------------------------------------------------------

NMErr
NMLeaveNotifier(NMEndpointRef inEndpoint, NMEndpointMode endpointMode)
{
	DEBUG_ENTRY_EXIT("NMLeaveNotifier");

	UNUSED_PARAMETER(inEndpoint);
	UNUSED_PARAMETER(endpointMode);

	op_assert(notifierLockCount > 0);

	if (module_inited < 1)
		return kNMInternalErr;

	if (notifierLockCount == 1)
		LEAVE_NOTIFIER();
	notifierLockCount--;	
	
	return kNMNoError;
}


/* 
 * Function: NMStartAdvertising
 *--------------------------------------------------------------------
 * Parameters:
 *  [IN] Endpoint = 
 *
 * Returns:
 *   true  = if set advertising succeeded
 *   false = on any error condition
 *
 * Description:
 *   Function to start advertising games.
 *
 *--------------------------------------------------------------------
 */

NMBoolean NMStartAdvertising(NMEndpointRef Endpoint)
{
	DEBUG_ENTRY_EXIT("NMStartAdvertising");

	if (module_inited < 1)
		return(false);   //kNMInternalErr;

	if (!Endpoint)
		return(false);

	Endpoint->advertising = true;

	return(true);

} /* NMStartAdvertising */


/* 
 * Function: NMStopAdvertising
 *--------------------------------------------------------------------
 * Parameters:
 *  [IN] Endpoint = 
 *
 * Returns:
 *   true  = if stop succeeded
 *   false = on any error condition
 *
 * Description:
 *   Function to stop advertising games. 
 *
 *--------------------------------------------------------------------
 */

NMBoolean NMStopAdvertising(NMEndpointRef Endpoint)
{
	DEBUG_ENTRY_EXIT("NMStopAdvertising");

	if (module_inited < 1)
		return(false);  //kNMInternalErr;

	if (!Endpoint)
		return(false);

	Endpoint->advertising = false;

	return(true);

} /* NMStopAdvertising */

int createWakeSocket(void)
{
	struct sockaddr Server_Address;
	int result;
	posix_size_type nameLen = sizeof(Server_Address);
	
		
	wakeSocket = 0;
	wakeHostSocket = 0;
	
	machine_mem_zero((char *) &Server_Address, sizeof(Server_Address));	
	((sockaddr_in *) &Server_Address)->sin_family = AF_INET;
	((sockaddr_in *) &Server_Address)->sin_addr.s_addr = INADDR_ANY;
	((sockaddr_in *) &Server_Address)->sin_port = 0;
	
	wakeHostSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (!wakeHostSocket)	return false;
	result = bind(wakeHostSocket, (struct sockaddr *)&Server_Address, sizeof(Server_Address));
	if (result == -1)	return false;
	wakeSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (!wakeSocket)	return false;
	result = bind(wakeSocket, (struct sockaddr *)&Server_Address, sizeof(Server_Address));
	if (result == -1)	return false;
	
	result = getsockname(wakeHostSocket, (struct sockaddr *)&Server_Address, &nameLen);
	if (result == -1)	return false;
	result = connect(wakeSocket,(sockaddr*)&Server_Address,sizeof(Server_Address));
	if (result == -1)	return false;
	
//	result = send(wakeSocket,buffer,1,0);
//	if (result == -1)	return false;
		
//	long amountIn = recv(wakeHostSocket,buffer,sizeof(buffer),0);
//	printf("in buffer: %d\n",amountIn);
	return true;
}

void disposeWakeSocket(void)
{
	if (wakeSocket)
		close(wakeSocket);
	if (wakeHostSocket)
		close(wakeHostSocket);
}


//sends a small datagram to our "wake" socket to try and break out of a select() call
void sendWakeMessage(void)
{
	char buffer[10];
	//DEBUG_PRINT("sending wake message");
	int result = send(wakeSocket,buffer,1,0);
	//DEBUG_PRINT("sendWakeMessage result: %d",result);
}


void SetNonBlockingMode(int fd)
{

	DEBUG_ENTRY_EXIT("SetNonBlockingMode");

#ifdef OP_API_NETWORK_SOCKETS
	int val = fcntl(fd, F_GETFL,0); //get current file descriptor flags
	if (val < 0)
	{
		DEBUG_PRINT("error: fcntl() failed to get flags for fd %d: err %d",fd,op_errno);
		return;
	}

	val |= O_NONBLOCK; //turn non-blocking on
	if (fcntl(fd, F_SETFL, val) < 0)
	{
		DEBUG_PRINT("error: fcntl() failed to set flags for fd %d: err %d",fd,op_errno);
	}
#elif defined(OP_API_NETWORK_WINSOCK)
	unsigned long DataVal = 1;
	long result = ioctlsocket(fd,FIONBIO,&DataVal);
	op_assert(result == 0);
#endif
}


//process any events that have happened with the endpoints
NMBoolean processEndpoints(NMBoolean block)
{
	struct timeval timeout;
	long nfds = 0;
	long numEvents;
	NMBoolean gotEvent = false;
	NMEndpointPriv *theEndPoint;
	fd_set input_set, output_set, exc_set; //create input,output,and error sets for the select function
	NMUInt32 listStartState;
			
	// if block is true, we wait one second - otherwise not at all
	// we wait a max of one second because we have to check every once in a while to see if we need to terminate our thread
	// or add new endpoints to watch
	// ECF - changed this to 10 seconds in the debug version so as to identify unnecessary delays
	// (sufficient machinery is in place now so we never should need to spin waiting for the duration of a select() call)
	if (block)
	{
		#if (DEBUG)
			timeout.tv_sec = 10;
		#else
			timeout.tv_sec = 1;
		#endif
		timeout.tv_usec = 0;
	}
	else
	{
		timeout.tv_sec = 0;
		timeout.tv_usec = 0;
	}
	
	//set up the sets
	FD_ZERO(&input_set);
	FD_ZERO(&output_set);
	FD_ZERO(&exc_set);

	//so we can abort if the list changes while we're using it
	listStartState = endpointListState;
	
	//dont hog the proc if we cant get the list right now
	if (TRY_LOCK_ENDPOINT_WAITING_LIST() == false)
	{
		if (!block)
			return false;
		usleep(10000); //sleep for 10 millisecs
		return false;
	}

	if (TRY_LOCK_ENDPOINT_LIST() == false)
	{
		UNLOCK_ENDPOINT_WAITING_LIST();
		if (!block)
			return false;
		usleep(10000); //sleep for 10 millisecs
		return false;
	}
	else
		UNLOCK_ENDPOINT_WAITING_LIST();
	
	//if we have a wake endpoint, add it
	if (wakeHostSocket)
		FD_SET(wakeHostSocket,&input_set);
	
	//add all endpoints to our lists to check
	theEndPoint = endpointList;
			
	while (theEndPoint != NULL)
	{
		//first off, if the endpoint needs to die, start it dying.  
		if ((theEndPoint->needToDie == true) && (theEndPoint->dying == false))
		{
			theEndPoint->dying = true;

			//only inform them of its demise if its been fully formed
			if (theEndPoint->alive == true)
			{
				UNLOCK_ENDPOINT_LIST(); //they'll probably kill the endpoint (and thus modify the list) as a result
				//tell the player the endpoint died and make sure we don't do anything else with it
				theEndPoint->callback(theEndPoint,theEndPoint->user_context,kNMEndpointDied,0,NULL);
				LOCK_ENDPOINT_LIST();
			}
		}
		
		//if the list has changed, we need to abort and do it next time
		if (listStartState != endpointListState)
		{
			UNLOCK_ENDPOINT_LIST();
			return true;
		}
		
		//we always check for waiting input and errors
		//we only look for output ability if our flow is blocked and we therefore need to see when we can send again
		if (theEndPoint->connectionMode & (1 << _datagram_socket)){
			FD_SET(theEndPoint->sockets[_datagram_socket],&input_set);
			if (theEndPoint->flowBlocked[_datagram_socket])
				FD_SET(theEndPoint->sockets[_datagram_socket],&output_set);
			FD_SET(theEndPoint->sockets[_datagram_socket],&exc_set);
			if (theEndPoint->sockets[_datagram_socket] + 1 > nfds)
    			nfds = theEndPoint->sockets[_datagram_socket] + 1;			
		}
		if (theEndPoint->connectionMode & (1 << _stream_socket)){
			FD_SET(theEndPoint->sockets[_stream_socket],&input_set);
			//only look for output if our flow is blocked
			if (theEndPoint->flowBlocked[_stream_socket])
				FD_SET(theEndPoint->sockets[_stream_socket],&output_set);
			FD_SET(theEndPoint->sockets[_stream_socket],&exc_set);
			if (theEndPoint->sockets[_stream_socket] + 1 > nfds)
    			nfds = theEndPoint->sockets[_stream_socket] + 1;						
		}
		theEndPoint = theEndPoint->next;
	}
	
	//check for events
	if (endpointList)
		numEvents = select(nfds,&input_set,&output_set,&exc_set,&timeout);
	else
		numEvents = 0; //save ourselves some time with good ol' common sense!

	// Look Through all the endpoints to see which ones have news for us
	if (numEvents > 0)
    {
    	gotEvent = true; //something came in
		theEndPoint = endpointList;
		
		//first check our wake socket
		if (wakeHostSocket){
			char buffer[64];
			if (FD_ISSET(wakeHostSocket,&input_set)){
				long amountIn = recv(wakeHostSocket,buffer,sizeof(buffer),0);
				DEBUG_PRINT("wakeHostSocket amountIn: %d",amountIn);
			}
		}
		while (theEndPoint)
		{
			//this endpoint has a datagram socket
			if (theEndPoint->connectionMode & (1 << _datagram_socket))
				processEndPointSocket(theEndPoint,_datagram_socket,&input_set,&output_set,&exc_set);			

			//abort if the list has been changed since theEndPoint may no longer exist for all we know...
			if (endpointListState != listStartState)
			{
				break;
			}
				
			//if it has a stream socket
			if (theEndPoint->connectionMode & (1 << _stream_socket))
				processEndPointSocket(theEndPoint,_stream_socket,&input_set,&output_set,&exc_set);
			
			//abort if the list has been changed since theEndPoint may no longer exist for all we know...
			if (endpointListState != listStartState)
				break;

			theEndPoint = theEndPoint->next;
		}
	}
	
	UNLOCK_ENDPOINT_LIST();
	
	//if we arent getting events, we dont wanna sit in a spin-lock, hogging the endpoint list
	if (gotEvent == false)
	{
		usleep(10000); //sleep for 10 millisecs
	}
	return gotEvent;
}

//this function is always called with access to a locked endpoint-list, remember. And it should always return a locked list.
void processEndPointSocket(NMEndpointPriv *theEndPoint, long socketType, fd_set *input_set, fd_set *output_set, fd_set *exc_set)
{
	//cant do nothing if they've called ProtocolEnterNotifier
	if (TRY_ENTER_NOTIFIER() == false)
		return;

	if (FD_ISSET(theEndPoint->sockets[socketType],exc_set)) // There was an Error?
	{
		NMUInt32 listStartState = endpointListState;
		//if we're already dying, theres nothing to be done...
		if (theEndPoint->dying == false)
		{
			DEBUG_PRINT("got an exception for EP 0x%x",theEndPoint);
			if (theEndPoint->alive == true)
			{
				//todo: is there something that would end up here that's not fatal?
			
				//tell the player the endpoint died and make sure we don't do anything else with it
				theEndPoint->needToDie = true;
				theEndPoint->dying = true;
				UNLOCK_ENDPOINT_LIST();
				theEndPoint->callback(theEndPoint,theEndPoint->user_context,kNMEndpointDied,0,NULL);
				LOCK_ENDPOINT_LIST();
				
				//if an endpoint was added or removed, we can't go on (it might have been us)
				if (listStartState != endpointListState){
					LEAVE_NOTIFIER();
					return;
				}
			}
			else//if the endpoint is just being created, set it up to return an error.
			{
				theEndPoint->opening_error = kNMOpenFailedErr;
				theEndPoint->needToDie = true;
				LEAVE_NOTIFIER();
				return;
			}
		}
	}

	if ((FD_ISSET(theEndPoint->sockets[socketType],input_set))) // Data has arrived
	{
		NMUInt32 listStartState = endpointListState;
			
		//if the endpoint is dying, we don't care....
		//(we might, though, in the future, if we're doing "graceful" disconnects or something)
		if (theEndPoint->dying == false)
		{
			NMCallbackCode code;
			
			//if this is a listener-endpoint, we need to deal with the new connection
			//ask the user to accept or reject
			if ((socketType == _stream_socket) && (theEndPoint->listener == true))
			{
				//only dish this out if we're alive
				if (theEndPoint->alive == true)
				{
					DEBUG_PRINT("sending user a connect request");
					UNLOCK_ENDPOINT_LIST();
					theEndPoint->callback(theEndPoint,theEndPoint->user_context,kNMConnectRequest,0,NULL);
					LOCK_ENDPOINT_LIST();
					//if an endpoint was added or removed, we can't go on (it might have been us)
					if (listStartState != endpointListState){
						LEAVE_NOTIFIER();
						return;
					}
				}
			}
			//looks like its data coming in on a normal endpoint. see if we need to handle it internally
			//(non-nspmode connections use a "confirmation" packet at the beginning of a connection)
			else if (internally_handle_read_data(theEndPoint,socketType) == false)
			{
				//only dish out if we're alive
				if (theEndPoint->alive)
				{
					//do a test-read without actually pulling data out
					long readResult = socketReadResult(theEndPoint,socketType);
					
					//first off we check the data to see if its of zero length - this implies
					//that the endpoint has died, in which case we deliver that news instead of the new-data
					if ( readResult <= 0)
					{
						if (theEndPoint->dying == false) //if we havn't delivered the news, do it
						{
							//tell the player the endpoint died and make sure we don't do anything else with it
							theEndPoint->needToDie = true;
							theEndPoint->dying = true;
							UNLOCK_ENDPOINT_LIST();
							DEBUG_PRINT("sending kNMEndpointDied for ep 0x%x",theEndPoint);
							theEndPoint->callback(theEndPoint,theEndPoint->user_context,kNMEndpointDied,0,NULL);
							LOCK_ENDPOINT_LIST();
							
							LEAVE_NOTIFIER();
							return;
						}					
					}
					
					//we only tell them of new data once until they retrieve it
					if (theEndPoint->newDataCallbackSent[socketType] == false)
					{
						theEndPoint->newDataCallbackSent[socketType] = true;
					
						//tell the user that data has arrived
						if (socketType == _stream_socket)
						{
							DEBUG_PRINT("sending kNMStreamData callback to 0x%x",theEndPoint);
							code = kNMStreamData;
						}
						else
						{
							DEBUG_PRINT("sending kNMDatagramData callback to 0x%x",theEndPoint);
							code = kNMDatagramData;
						}
							
						//hand it off to the user to read or whatever
						UNLOCK_ENDPOINT_LIST();
						theEndPoint->callback(theEndPoint,theEndPoint->user_context,code,0,NULL);
						LOCK_ENDPOINT_LIST();
						
						//if an endpoint was added or removed, we can't go on (it might have been us)
						if (listStartState != endpointListState){
							LEAVE_NOTIFIER();
							return;
						}
					}
				}
				//if we're not yet alive, we at least see if the endpoint has died so we can fail in opening
				//otherwise we'd wind up spinning until the open timed out
				else
				{
					//if they killed us off before we could finish opening, just assume it was a rejection
					if (socketReadResult(theEndPoint,_stream_socket) <= 0)
					{
						theEndPoint->opening_error = kNMAcceptFailedErr;
						theEndPoint->needToDie = true;
						LEAVE_NOTIFIER();
						return;
					}
				}
			}
		}
	}
	
	if (FD_ISSET(theEndPoint->sockets[socketType],output_set)) // Socket is ready to write
	{
		NMUInt32 listStartState = endpointListState;
	
		//if its dying, we dont care
		if (theEndPoint->dying == false)
		{
			//if this socket hasnt been declared valid yet, we do so
			//if it already was, there must have been a flow control problem and we need to send
			//out a flow clear message (unless its a listener, which doesnt send)
			theEndPoint->flowBlocked[socketType] = false;
			UNLOCK_ENDPOINT_LIST();
			if (theEndPoint->valid_endpoints & (1 << socketType)) //this endpoint's been marked valid already - this must be flow clear
			{
				DEBUG_PRINT("sending 0x%x a flowclear for socketType %d.",theEndPoint,socketType);
				DEBUG_PRINT("datagram blocked: %d",theEndPoint->flowBlocked[_datagram_socket]);
				DEBUG_PRINT("datagram valid: %d",theEndPoint->valid_endpoints & (1 << _datagram_socket));
				theEndPoint->callback(theEndPoint,theEndPoint->user_context,kNMFlowClear,0,NULL);
			}
			else
			{
				NMBoolean setValid = true;

				//if we're active and in netsprocket mode, we set our remote datagram address based on the stream socket.  We have to use it every time we
				//send a datagram, since we can't just connect() to it (because we send and receive to different ports and that wouldnt work)
				//if we're an active, non-netsprocket-mode stream socket coming online, we send our datagram address
				if ((socketType == _stream_socket) && (theEndPoint->active == true))
				{
					{
						NMErr err;
						posix_size_type	size  = sizeof(theEndPoint->remoteAddress);

						DEBUG_PRINT("setting remote datagram address");
						err = getpeername(theEndPoint->sockets[_stream_socket], (sockaddr*) &theEndPoint->remoteAddress, &size);
						
						//if we got a not-connected error, it means they dumped us real quick. we die.
						if (err)
						{
							setValid = false;
							DEBUG_NETWORK_API("getpeername",err);
						}
						//if there was no error and we're not in nspmode, ship off our datagram port
						else if (!theEndPoint->netSprocketMode)
						{
							_send_datagram_socket(theEndPoint);
						}
					}
				}
				
				//if its a remote-client connection and the stream-socket is ready to go, we connect our datagram socket and
				//send off our "ready" callbacks.  If we're in NSpMode, we use this opportunity to assign our datagram port too
				if ((socketType == _stream_socket) && (theEndPoint->active == false) && (theEndPoint->listener == false))
				{
					op_assert(theEndPoint->parent != NULL);
					
					//this signals that messages can start flowin' in
					theEndPoint->alive = true;		
			
					// And aim our datagrams at their port
					if (theEndPoint->netSprocketMode)
					{
						NMErr result;
						sockaddr_in address;
						posix_size_type	address_size= sizeof (address);
					
						getpeername(theEndPoint->sockets[_stream_socket], (sockaddr*) &address, &address_size);
						DEBUG_PRINT("connecting datagram socket to port %d",ntohs(address.sin_port));						
						result = connect(theEndPoint->sockets[_datagram_socket],(sockaddr*)&address,sizeof(address));
						DEBUG_NETWORK_API("connect",result);
					}
					//send a handoff-complete to its parent and an accept-complete to it
					theEndPoint->callback(theEndPoint, theEndPoint->user_context, kNMAcceptComplete, 0, theEndPoint->parent);
					theEndPoint->parent->callback(theEndPoint->parent, theEndPoint->parent->user_context, kNMHandoffComplete, 0, theEndPoint);
				}
				
				if (setValid)
				{
					if (socketType == _datagram_socket)
						DEBUG_PRINT("datagram socket ready to write for 0x%x",theEndPoint);
					else	
						DEBUG_PRINT("stream socket ready to write for 0x%x",theEndPoint);
											
					MARK_ENDPOINT_AS_VALID(theEndPoint,socketType);
				}
				else
				{
					if (socketType == _datagram_socket)
						DEBUG_PRINT("datagram socket completion failed for 0x%x",theEndPoint);
					else	
						DEBUG_PRINT("stream socket completion failed for 0x%x",theEndPoint);				
				}
			}
			LOCK_ENDPOINT_LIST();

			//if an endpoint was added or removed, we can't go on (it might have been us)
			if (listStartState != endpointListState){
				LEAVE_NOTIFIER();
				return;
			}
		}
	}
	LEAVE_NOTIFIER();
	return;
}

//This function gets the IP address of the remote peer on the connection.

NMErr NMGetAddress(NMEndpointRef inEndpoint, NMAddressType addressType, void **outAddress)
{
	NMErr status = kNMNoError;

	switch( addressType )
	{
		case kNMIPAddressType:	//¥ IP address (string of format "127.0.0.1:80")
		
		    // First, get the socket address for the remote connection...
		    // (Note, I tried using inEndpoint->remoteAddress.sin_addr, but the value
		    // was nil.  Isn't that structure element supposed to contain the remote
		    // machine's address?  Does this mean that datagrams will be screwed?
		    // I'll have to look into this when I have more time, but for now, I don't
		    // need datagrams, and I can get the remote address directly using "getpeername()"...)
		    
		    posix_size_type size;
		    unsigned char *byte;
		    sockaddr_in	socket_address;

		    size  = sizeof(sockaddr_in);
		    status = getpeername(inEndpoint->sockets[_stream_socket], (sockaddr *) &socket_address, &size);
		
		    // Now, convert the UInt32 value in socket_address.sin_addr into a dotted decimal
		    // format string...
		    byte = (unsigned char *) &socket_address.sin_addr;
		    *outAddress = (void *) new char[16];
		    #if big_endian
		    sprintf((char *) *outAddress, "%u.%u.%u.%u",byte[0],byte[1],byte[2],byte[3]);
		    #else
		    sprintf((char *) *outAddress, "%u.%u.%u.%u",byte[3],byte[2],byte[1],byte[0]);		    
		    #endif
		break;

		default:	// This module returns no other type of address.
			status = kNMParameterErr;
		break;
	}

	return status;
}

//This function frees the memory allocated by NMGetAddress().

NMErr NMFreeAddress(NMEndpointRef inEndpoint, void **outAddress)
{
	// (OP_PLATFORM_MAC_MACHO)

	NMErr	err = kNMNoError;

	#pragma unused (inEndpoint)
	
	op_vassert_return((outAddress != NULL),"OutAddress is NIL!",kNMParameterErr);
	op_vassert_return((*outAddress != NULL),"*OutAddress is NIL!",kNMParameterErr);

	delete (char *) *outAddress;

	return( err );
}


#ifdef USE_WORKER_THREAD
void createWorkerThread(void)
{
	dieWorkerThread = false;
	
	//if we've already got a worker thread...
	if (workerThreadAlive)
		return;
		
	#ifdef OP_API_NETWORK_SOCKETS
		long pThreadResult = pthread_create(&worker_thread,NULL,worker_thread_func,NULL);
		op_assert(pThreadResult == 0);
	#elif defined(OP_API_NETWORK_WINSOCK)
		worker_thread_handle = CreateThread((_SECURITY_ATTRIBUTES*)NULL,
								0,
								worker_thread_func,
								0, //context
								0,
								&worker_thread);
		op_assert(worker_thread_handle != NULL);
	#endif
	
	workerThreadAlive = true;
}

void killWorkerThread(void)
{
	if (workerThreadAlive == false)
		return;
	
	DEBUG_PRINT("terminating worker-thread...");	
	
	// on windows, we simply annihilate the thread in its tracks,
	// as it seems unable to execute, and thus shut itself down, during DllMain().
	// this is a rather unelegant way to do it...
	#ifdef OP_API_NETWORK_WINSOCK
		BOOL result = 	TerminateThread(worker_thread_handle,0);
		op_assert(result != 0);
		dieWorkerThread = true;
		workerThreadAlive = false;
		return;
	#endif
			
	dieWorkerThread = true;
	
	//snap the worker thread out of its select() call
	sendWakeMessage();
	
	//wait while it dies
	while (workerThreadAlive == true)
	{
		usleep(10000); //sleep for 10 millisecs
	}
	DEBUG_PRINT("...worker thread terminated.");			
}

// the main function for our worker thread, which just sits and waits for socket events to happen
#ifdef OP_API_NETWORK_SOCKETS
	void* worker_thread_func(void *arg)
#elif defined(OP_API_NETWORK_WINSOCK)
	DWORD WINAPI worker_thread_func(LPVOID arg)
#endif
{
	UNUSED_PARAMETER(arg);

	NMBoolean done = false;

	DEBUG_PRINT("worker_thread is now running");
	workerThreadAlive = true;
	
	//sit in a loop blocking until new stuff happens
	while (!done)
	{
		processEndpoints(true);
		if (dieWorkerThread)
			done = true;
	}
	
	DEBUG_PRINT("worker-thread shutting down");	
	workerThreadAlive = false;
    pthread_exit(0);
	return NULL;
}
#endif //worker-thread

//----------------------------------------------------------------------------------------
// receive_udp_port
//----------------------------------------------------------------------------------------

void
receive_udp_port(NMEndpointRef endpoint)
{
	udp_port_struct_from_server	port_data;
	NMSInt32	err;
	NMUInt32	size= sizeof(port_data);
	NMUInt32	flags;

	DEBUG_ENTRY_EXIT("receive_udp_port");
	
	//first off, theres a chance that they disconnected before sending
	if (socketReadResult(endpoint,_stream_socket) <= 0)
	{
		endpoint->opening_error = kNMAcceptFailedErr;
		endpoint->needToDie = true;
		return;
	}
	// Read the port...
	err = _receive_data(endpoint, _stream_socket, (unsigned char *)&port_data, &size, &flags);
	if (!err && size==sizeof (port_data))
	{
		NMSInt16 	old_port;
		sockaddr_in address;
		posix_size_type	address_size= sizeof (address);

		op_assert(size==sizeof (port_data)); // or else
		
		// And change our local port to match theirs..
		getpeername(endpoint->sockets[_datagram_socket], (sockaddr*) &address, &address_size);
		old_port= address.sin_port;
		address.sin_port= port_data.port; // already in network order
		{
			long port = ntohs(address.sin_port);
			DEBUG_PRINT("received remote udp port: %d",port);
		}
		
		// make the connect call.. (sets the udp destination)
		err= connect(endpoint->sockets[_datagram_socket], (sockaddr*) &address, sizeof (address));
		if (!err)
		{
			// and mark the connection as having been completed...
			MARK_ENDPOINT_AS_VALID(endpoint, _datagram_socket);
		}
		else
			DEBUG_NETWORK_API("connect()", err);
	}
	//if we got a read of size 0, they disconnected before sending it (most likely a rejection)
	else
	{
		DEBUG_PRINT("error receiving udp port");	
		endpoint->opening_error = kNMAcceptFailedErr;
		endpoint->needToDie = true;
	}
}

long socketReadResult(NMEndpointRef endpoint, int socketType)
{
	int socket = endpoint->sockets[socketType];
	char buffer[4];
	long result;
	
	//we read the first bit of data in the socket (in peek mode) to determine if the socket has died
	result = recvfrom(socket,(char*)buffer,sizeof(buffer),MSG_PEEK,NULL,NULL);
	
	//if its an error, go ahead and read it for real
	if (result == -1)
	{
		//windows complains if the buffer isnt big enough to fit the datagram. i dont think any unixes do.
		//this of course is not a problem
#ifdef HACKY_EAGAIN
		if ( (op_errno == EMSGSIZE) || (op_errno == EAGAIN) )
#else
		if (op_errno == EMSGSIZE)
#endif // HACKY_EAGAIN
			return 4; //pretend we read it and go home happy
		DEBUG_NETWORK_API("socketReadResult",result);
		return recvfrom(socket,(char*)buffer,sizeof(buffer),0,NULL,NULL);
	}
	return result;
}

#ifdef OP_API_NETWORK_WINSOCK
void initWinsock(void)
{
	DEBUG_ENTRY_EXIT("initWinsock");
	NMErr err;
	// Here we make connection with the Winsock library for the windows build
	// this can cause problems if we do it when loading the library
	if (winSockRunning == false)
	{
		WSADATA		aWsaData;
		NMSInt16	aVersionRequested= MAKEWORD(1, 1);
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
			else
				winSockRunning = true;
		}
		else
		{
			DEBUG_PRINT("Unable to startup winsock!");
		}
	}
}

void shutdownWinsock(void)
{
	DEBUG_ENTRY_EXIT("shutdownWinsock");

	if (winSockRunning)
	{
		winSockRunning = false;
		WSACleanup();
	}
}
#endif


