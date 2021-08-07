/* 
 *-------------------------------------------------------------
 * Description:
 *   Functions which handle enumeration
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

#include "OPUtils.h"
#include "configuration.h"
#include "configfields.h"
#include "ip_enumeration.h"

#include "NetModule.h"
#include "tcp_module.h"



/* 
 * Static Function: _handle_game_enumeration_packet
 *--------------------------------------------------------------------
 * Parameters:
 *   [IN] Config 
 *
 * Returns:
 *  
 *
 * Description:
 *   Function 
 *
 *--------------------------------------------------------------------
 */

static void _handle_game_enumeration_packet(NMConfigRef Config, 
                                            IPEnumerationResponsePacket *packet,
                                            long host, unsigned short port)
{
	DEBUG_ENTRY_EXIT("_handle_game_enumeration_packet");
	if(Config && Config->enumerating)
	{
		/* Byteswap the packet.. */
		packet->host = host; /* in network byte order. */
		packet->port = port;
		byteswap_ip_enumeration_packet((char *) packet);

		if ((Config->gameID == packet->gameID) && 
			(Config->new_game_count < MAXIMUM_GAMES_ALLOWED_BETWEEN_IDLE))
		{
			struct available_game_data *new_game;

			new_game = &Config->new_games[Config->new_game_count++];

			memset(new_game, 0, sizeof(struct available_game_data));

			new_game->host  = packet->host;
			new_game->port  = packet->port;
			new_game->flags = _new_game_flag;
			new_game->ticks_at_last_response= machine_tick_count();

			strncpy(new_game->name, packet->name, kMaxGameNameLen);

			new_game->name[kMaxGameNameLen]= '\0';
		}
	}	

	return;
} /* _handle_game_enumeration_packet  */


/* 
 * Static Function: _handle_packets
 *--------------------------------------------------------------------
 * Parameters:
 *   [IN] Config 
 *
 * Returns:
 *  
 *
 * Description:
 *   Function 
 *
 *--------------------------------------------------------------------
 */

static NMErr _handle_packets(NMConfigRef Config)
{

	DEBUG_ENTRY_EXIT("_handle_packets");

	struct sockaddr_in source_address;
	int bytes_read;
	int done = 0;
	posix_size_type source_address_len = sizeof(source_address);

	if (!Config->enumerating || (Config->enumeration_socket == INVALID_SOCKET))
	return(kNMNotEnumeratingErr);

	while (!done)
	{
		//op_errno = 0;
		bytes_read = recvfrom(Config->enumeration_socket, Config->buffer, (unsigned long)MAXIMUM_CONFIG_LENGTH,
		0, (sockaddr*)&source_address, &source_address_len);

		if (bytes_read >= 0)
		{
			DEBUG_PRINT("read %d bytes",bytes_read);
			
			IPEnumerationResponsePacket *packet = (IPEnumerationResponsePacket *) Config->buffer;

			if (packet->responseCode == htonl(kReplyFlag))
			{
				_handle_game_enumeration_packet(Config, packet, source_address.sin_addr.s_addr, source_address.sin_port);
			} 
			else
			DEBUG_PRINT("Got a response packet with a size of %d but a response code of: 0x%x", bytes_read, packet->responseCode);
		} 
		else
		{
			if (op_errno != EWOULDBLOCK) 
				DEBUG_PRINT("Error in _handle_packets: recvfrom returned %d", op_errno);
			done = 1;
		}
	}

	return(kNMNoError);
} /* _handle_packets */


/* 
 * Static Function: _send_game_request_packet
 *--------------------------------------------------------------------
 * Parameters:
 *   [IN] Config 
 *
 * Returns:
 *  
 *
 * Description:
 *   Function 
 *
 *--------------------------------------------------------------------
 */

static void _send_game_request_packet(NMConfigRef Config)
{
	DEBUG_ENTRY_EXIT("_send_game_request_packet");
	//DEBUG_PRINT("numm:%d",machine_tick_count());
	//DEBUG_PRINT("last: %d this: %d (%d)",Config->ticks_at_last_enumeration_request,machine_tick_count(),MACHINE_TICKS_PER_SECOND);
	if (machine_tick_count() - Config->ticks_at_last_enumeration_request > TICKS_BETWEEN_ENUMERATION_REQUESTS)
	{
		Config->ticks_at_last_enumeration_request = machine_tick_count();

		struct sockaddr_in dest_address;
		char request_packet[kQuerySize];
		int bytes_sent;
		short packet_length;

		/* Doing this from memory... */
		packet_length = build_ip_enumeration_request_packet(request_packet);

		op_assert(packet_length <= sizeof(request_packet));

		/* Build the address. */
		dest_address.sin_family = AF_INET;
		dest_address.sin_addr.s_addr = INADDR_BROADCAST;
		dest_address.sin_port = Config->hostAddr.sin_port; //already in network order
		DEBUG_PRINT("broadcasting to port %d",ntohs(dest_address.sin_port));

		/* Send the request. */
		if (Config->enumeration_socket == INVALID_SOCKET)
		{
			DEBUG_PRINT("invalid socket for enumeration");
			return;
		}
		
		bytes_sent = sendto(Config->enumeration_socket, request_packet, packet_length, 0,
		(sockaddr*)&dest_address, sizeof(dest_address));
		if (bytes_sent == -1)
		{
			DEBUG_NETWORK_API("sendto()",bytes_sent);
		} 
		else{
			DEBUG_PRINT("bytes sent: %d",bytes_sent);
		#ifdef DEBUG
			if (bytes_sent != packet_length)
				DEBUG_PRINT("Error in  _send_game_request_packet: sendto only delivered %d bytes of %d", bytes_sent, packet_length);
			#endif
		}
	}

	return;
} /* _send_game_request_packet */


/* 
 * Function: NMBindEnumerationtoConfig
 *--------------------------------------------------------------------
 * Parameters:
 *   [IN] Config 
 *   [IN] Host_ID
 *
 * Returns:
 *   kNMNoError = success
 *
 * Description:
 *   Function 
 *
 *--------------------------------------------------------------------
 */

NMErr NMBindEnumerationItemToConfig(NMConfigRef inConfig, NMHostID inID)
{
	DEBUG_ENTRY_EXIT("NMBindEnumerationtoConfig");

	NMErr 		err= kNMNoError;

	op_assert(inConfig->cookie==config_cookie);
	if(inConfig->enumerating)
	{
		int	index;

		for(index= 0; index<inConfig->game_count; ++index)
		{
			if((NMHostID)(inConfig->games[index].host)==inID)
	    	{
				inConfig->hostAddr.sin_family = AF_INET;
				inConfig->hostAddr.sin_addr.s_addr = htonl(inConfig->games[index].host);
				inConfig->hostAddr.sin_port = htons(inConfig->games[index].port);
				strcpy(inConfig->host_name, inet_ntoa(inConfig->hostAddr.sin_addr));
				break;
		    }
		}
		
		if	(index==inConfig->game_count)
	  		err= kNMInvalidConfigErr;

	} else
		err= kNMNotEnumeratingErr;

	return err;	

} /* NMBindEnumerationtoConfig */


/* 
 * Function: NMStartEnumeration
 *--------------------------------------------------------------------
 * Parameters:
 *   [IN] Config 
 *   [IN] Callback
 *   [IN] Context
 *   [IN] Active
 *
 * Returns:
 *  
 *
 * Description:
 *   Function 
 *
 *--------------------------------------------------------------------
 */

NMErr NMStartEnumeration(NMConfigRef Config, NMEnumerationCallbackPtr Callback, void *Context, NMBoolean Active)
{

	DEBUG_ENTRY_EXIT("NMStartEnumeration");

	if (module_inited < 1)
		return kNMInternalErr;

	int status;


	if (!Config || !Callback)
		return(kNMParameterErr);

    op_assert(Config->cookie==config_cookie);
	if (Config->cookie != config_cookie)
		return(kNMInvalidConfigErr); 

	//	If they don't want us to actively get the enumeration, there is nothing to do
	Config->activeEnumeration = Active;
	if (! Active)
		return kNMNoError;

	if(!Config->enumerating)
	{  
		op_assert(!Config->callback);
		op_assert(!Config->games);
		op_assert(!Config->game_count);
		op_assert(Config);
		op_assert(Config->enumeration_socket==INVALID_SOCKET);
  
		if (Config->callback || Config->games || Config->game_count || 
			(Config->enumeration_socket != INVALID_SOCKET))
		{
			DEBUG_PRINT("return b");
			return(kNMInvalidConfigErr);
		}
	    
	    Config->enumeration_socket = socket(AF_INET, SOCK_DGRAM, 0);

		if (Config->enumeration_socket != INVALID_SOCKET)
		{
			struct sockaddr_in  sock_addr;
			
			sock_addr.sin_family = AF_INET;
			sock_addr.sin_addr.s_addr = INADDR_ANY;
			sock_addr.sin_port = 0;

			status = bind(Config->enumeration_socket, (sockaddr*)&sock_addr, sizeof(sock_addr));


			if (status == 0)
			{
				int option_val = 1;

				/* enable broadcast on socket */
				status = setsockopt(Config->enumeration_socket, SOL_SOCKET, SO_BROADCAST, (char*)&option_val, sizeof(int));

				/* set socket non-blocking */
				SetNonBlockingMode(Config->enumeration_socket);

				Config->callback     = Callback;
				Config->user_context = Context;
				Config->enumerating  = true;
				Config->ticks_at_last_enumeration_request = 0;

				/* clear the enumeration list */
				Config->callback(Config->user_context, kNMEnumClear, NULL);
			}
			else
			{
				DEBUG_NETWORK_API("bind()",status);
				return(kNMEnumerationFailedErr);
			}
	    }
	    else
	    {
	    	status = INVALID_SOCKET;
			DEBUG_NETWORK_API("socket()",status);
			return(kNMEnumerationFailedErr);
		}
	}
	else
	{
		DEBUG_PRINT("start enumeration failed: config already enumerating");
		return(kNMEnumerationFailedErr);
	}

	return(kNMNoError);

} /* NMStartEnumeration */


/* 
 * Function: NMIdleEnumeration
 *--------------------------------------------------------------------
 * Parameters:
 *  
 *
 * Returns:
 *  
 *
 * Description:
 *   Function 
 *
 *--------------------------------------------------------------------
 */

NMErr NMIdleEnumeration(NMConfigRef Config)
{

	//DEBUG_ENTRY_EXIT("NMIdleEnumeration");

	if (module_inited < 1)
		return kNMInternalErr;

	NMErr  err;


	if (!Config)
	return(kNMParameterErr);

	if (Config->cookie != config_cookie)
	return(kNMInvalidConfigErr);

	//we do nothing for inactive enumeration
	if (Config->activeEnumeration == false)
		return kNMNoError;

	if (Config->enumerating)
	{

		err = _handle_packets(Config);

		if ((err == kNMNoError) && (Config->new_game_count))  /* add game to list */
		{
			int index;
			int added_game_count = 0;
			struct available_game_data *new_games;

			for (index = 0; index < Config->new_game_count; ++index)
			{
				int compare_index;

				for (compare_index = 0; compare_index < Config->game_count; ++compare_index)
				{
					if ((Config->new_games[index].host == Config->games[compare_index].host) &&
					(Config->new_games[index].port == Config->games[compare_index].port))
					{
						Config->new_games[index].flags |= _duplicate_game_flag;
						Config->games[compare_index].ticks_at_last_response = machine_tick_count();
						break;
					}
				} /* for(compare_index) */

				if (compare_index == Config->game_count)
				added_game_count++;
			} /* for(index) */

			/* Now allocate and add... */
			if (added_game_count)
			{
				new_games = (struct available_game_data *) calloc(1, (Config->game_count + added_game_count) * sizeof(struct available_game_data));

				if (new_games)
				{
					memcpy(new_games, Config->games, Config->game_count * sizeof(struct available_game_data));

					// bump the count.
					for (index = 0; index < Config->new_game_count; ++index)
					{
						if ( !(Config->new_games[index].flags & _duplicate_game_flag) )
						{
							struct available_game_data *new_game = &new_games[Config->game_count++];

							memcpy(new_game, &Config->new_games[index], sizeof(struct available_game_data));
						}
					} // for(index)

					if (Config->games) 
						free(Config->games);

					Config->games = new_games;
				} // if (new_games)
			}

			// reset the new game count, since it is now in our global...
			Config->new_game_count = 0;

			// Give their callback the items... 
			index = 0;

			while (index < Config->game_count)
			{
				NMEnumerationItem item;

				if (Config->games[index].flags & _new_game_flag)
				{
					Config->games[index].flags &= ~_new_game_flag;

					item.id = Config->games[index].host;
					item.name = Config->games[index].name;

					// mb_printf("giving callbacks item %d (Host: 0x%x port: %d) as new game", index, 
					//Config->games[index].host, Config->games[index].port)  

					Config->callback(Config->user_context, kNMEnumAdd, &item);

					// next!
					index++;
				} 
				else 
				{
					// Drop if necessary
					if (machine_tick_count() - Config->games[index].ticks_at_last_response > TICKS_BEFORE_GAME_DROPPED)
					{
						// time to drop this one..
						item.id = Config->games[index].host;
						item.name = Config->games[index].name;

						// mb_printf("giving callbacks item %d as delete game", index);

						Config->callback(Config->user_context, kNMEnumDelete, &item);

						memmove(&Config->games[index], &Config->games[index+1],
						(Config->game_count-index - 1) * sizeof(struct available_game_data));

						Config->game_count -= 1;
					} 
					else
					index++; // go to next 
				}
			}

		}
	}
	else
		err = kNMNotEnumeratingErr;
		
	// ..and request more games...... 
	_send_game_request_packet(Config);

	//if we're not using the worker_thread, give some time for
	//network processing
	NMIdle(NULL);
	
	return(kNMNoError);

} /* NMIdleEnumeration */


/* 
 * Function: NMEndEnumeration
 *--------------------------------------------------------------------
 * Parameters:
 *  
 *
 * Returns:
 *  
 *
 * Description:
 *   Function 
 *
 *--------------------------------------------------------------------
 */

NMErr NMEndEnumeration(NMConfigRef Config)
{

	DEBUG_ENTRY_EXIT("NMEndEnumeration");

	if (module_inited < 1)
		return kNMInternalErr;

	int status;

	if (!Config)
		return(kNMParameterErr);

	if (Config->cookie != config_cookie)
		return(kNMInvalidConfigErr);

	//we do nothing for inactive enumeration
	if (Config->activeEnumeration == false)
		return kNMNoError;

	if (Config->enumerating)
	{
		if (Config->game_count)
		{
			Config->game_count = 0;

			if (Config->games)
				free(Config->games);
		}
		Config->games = NULL;

		Config->callback = NULL;
		Config->enumerating = false;

		if (Config->enumeration_socket == INVALID_SOCKET)
			return(kNMInvalidConfigErr);

		status = close(Config->enumeration_socket);

		if (status)
		{
			DEBUG_PRINT("Error in NMEndEnumeration: closesocket returned %d", op_errno);
		}

		Config->enumeration_socket = INVALID_SOCKET;

	} 
	else
		return(kNMNotEnumeratingErr);

	return(kNMNoError);

} /* NMEndEnumeration */
