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

/*
	module_enumeration.c
*/


#ifndef __OPENPLAY__
#include "OpenPlay.h"
#endif
#include "OPUtils.h"
#include "NetModule.h"
#include "ip_enumeration.h"
#include "win_ip_module.h"


//====================================================================================================================
// Typedefs
//====================================================================================================================


//====================================================================================================================
// globals
//====================================================================================================================


	extern NMBoolean gAdvertisingGames;

//====================================================================================================================
//	local prototypes
//====================================================================================================================

	static void handle_game_enumeration_packet	(	NMConfigRef 					inConfig,
													IPEnumerationResponsePacket *	packet,
													NMUInt32						host, 
													NMUInt16						port);

	static NMErr send_game_request_packet		(	NMConfigRef inConfig);
	
	static void handle_packets					(	NMConfigRef inConfig);

//====================================================================================================================
//	Enumeration functions
//====================================================================================================================


//----------------------------------------------------------------------------------------
// NMBindEnumerationItemToConfig
//----------------------------------------------------------------------------------------

NMErr 
NMBindEnumerationItemToConfig(
					     NMConfigRef inConfig, 
					     NMHostID inID)
{
	NMErr 		err= kNMNoError;

	op_assert(inConfig->cookie==config_cookie);
	if(inConfig->enumerating)
	{
		NMSInt16	index;

		for(index= 0; index<inConfig->game_count; ++index)
		{
			if((NMHostID)(inConfig->games[index].host)==inID)
	    	{
				inConfig->hostAddr.sin_family = AF_INET;
				/* sjb 19990330 should these be converted to network byte order? */
				inConfig->hostAddr.sin_addr.S_un.S_addr = htonl(inConfig->games[index].host);	/* [Edmark/PBE] 11/12/99 added SWAP4 */
				inConfig->hostAddr.sin_port = htons(inConfig->games[index].port);				/* [Edmark/PBE] 11/12/99 added SWAP2 */
				/*	      inConfig->host_name[0] = '\0'; */
				strcpy(inConfig->host_name, inet_ntoa(inConfig->hostAddr.sin_addr));	/* [Edmark/PBE] 11/12/99 added */
				break;
		    }
		}
		
		if	(index==inConfig->game_count)
	  		err= kNMInvalidConfigErr;

	} else
		err= kNMNotEnumeratingErr;

	return err;	
}

//----------------------------------------------------------------------------------------
// NMStartEnumeration
//----------------------------------------------------------------------------------------

NMErr
NMStartEnumeration(
  NMConfigRef inConfig, 
  NMEnumerationCallbackPtr inCallback, 
  void *inContext, 
  NMBoolean inActive)
{
	NMErr err = kNMNoError;

    op_assert(inConfig->cookie==config_cookie);

	//	If they don't want us to actively get the enumeration, there is nothing to do
	inConfig->activeEnumeration = inActive;
	if (! inActive)
		return kNMNoError;

  	if (!inConfig->enumerating)
    {
		op_assert(!inConfig->callback);
		op_assert(!inConfig->games);
		op_assert(!inConfig->game_count);
		op_assert(inCallback);
		op_assert(inConfig->enumeration_socket==INVALID_SOCKET);
		
      /* Create the socket.. */
		inConfig->enumeration_socket= socket(PF_INET, SOCK_DGRAM, 0);
		if (inConfig->enumeration_socket != INVALID_SOCKET)
		{
	  		SOCKADDR_IN sin;
		
			sin.sin_family= AF_INET;
			sin.sin_addr.s_addr= INADDR_ANY;
			sin.sin_port= 0;
	
	  /* bind */
	  		err = bind(inConfig->enumeration_socket, (LPSOCKADDR) &sin, sizeof(sin));
	  		if(!err)
	    	{
	      		BOOL		option;
				NMUInt32	do_not_block;
				
				/* allow broadcasting on this socket. */
				option= TRUE;
				setsockopt(inConfig->enumeration_socket, SOL_SOCKET, SO_BROADCAST, (const char *) &option, sizeof(option));
					
				/* Don't block */
				do_not_block= TRUE;
				ioctlsocket(inConfig->enumeration_socket, FIONBIO, &do_not_block);

				inConfig->callback= inCallback;
				inConfig->user_context= inContext;
				inConfig->enumerating= TRUE;
				inConfig->ticks_at_last_enumeration_request= 0; // force the immediate one.

			/* clear the list.. */
			/* [Edmark/PBE] 11/16/99 removed extraneous clear callback */
	      /* inConfig->callback(inConfig->user_context, kNMEnumClear, NULL); */
	    	} else {
				DEBUG_NETWORK_API("Bind for Enumeration", err);
				return kNMEnumerationFailedErr;
	    	}
		} else {
	  		err= WSAGetLastError();
	  		DEBUG_NETWORK_API("Bind for Enumeration", err);
	 		return kNMEnumerationFailedErr;
		} 
    } else {
		err= WSAGetLastError();
		DEBUG_NETWORK_API("Bind for Enumeration", err);
		return kNMEnumerationFailedErr;
    }
	
  // clear the list..
  inConfig->callback(inConfig->user_context, kNMEnumClear, NULL);
  return err;
}

//----------------------------------------------------------------------------------------
// NMIdleEnumeration
//----------------------------------------------------------------------------------------

NMErr
NMIdleEnumeration(
	 NMConfigRef inConfig)
{
	NMErr err= kNMNoError;

	op_assert(inConfig->cookie==config_cookie);
	
	//we do nothing for inactive enumeration
	if (inConfig->activeEnumeration == false)
		return kNMNoError;

	if (inConfig->enumerating)
    {
    	NMSInt16	index;

	// give ourself idle time (which ships us our win32 messages if need be)
	NMIdle(NULL);

	// Receive broadcast packets and if they are queries...
     	handle_packets(inConfig);

		/* Now add it to our list.. */
		if (inConfig->new_game_count)
		{
	 		NMSInt16				added_game_count= 0;
	  		available_game_data *	new_games;
		
	// first: check for multiple packets from same host and mark extras as duplicate
	 		for(index= 0; index<inConfig->new_game_count; ++index)
	    	{
				NMSInt16 compare_index;
	    
	    		for(compare_index= index + 1; compare_index<inConfig->new_game_count; ++compare_index)
				{
					if(	inConfig->new_games[index].host==inConfig->new_games[compare_index].host &&
				    	inConfig->new_games[index].port==inConfig->new_games[compare_index].port)
		    		{
						inConfig->new_games[index].flags |= _duplicate_game_flag;
						break;
		    		}
				}
	    	}
		
		// for each new query packet receieved	
			for(index= 0; index<inConfig->new_game_count; ++index)
	    	{
	    		NMSInt16 compare_index;
			// compare it against the ongoing list to see if it already exists
		    	
		    	for(compare_index= 0; compare_index<inConfig->game_count; ++compare_index)
				{
					if(inConfig->new_games[index].host==inConfig->games[compare_index].host &&
				    	 inConfig->new_games[index].port==inConfig->games[compare_index].port)
		    		{
		    // if it does then mark it as duplicate and update the timestamp of the original
				    	inConfig->new_games[index].flags |= _duplicate_game_flag;
				    	inConfig->games[compare_index].ticks_at_last_response= machine_tick_count();
				    	break;
		    		}
				}
			// if we looped all the way through then it is not a duplicate so increment 	
		     	if(compare_index==inConfig->game_count)
		  			added_game_count++;
	    	}
			
	  /* Now allocate and add... */
			if(added_game_count)
	    	{
				//DEBUG_PRINT("Got %d new games!", added_game_count);
		    	new_games= (available_game_data *) new_pointer((inConfig->game_count+added_game_count)*sizeof(available_game_data));
	    		if(new_games)
				{
		// allocate new block, copy the exisitng games into it
					machine_copy_data(inConfig->games, new_games, inConfig->game_count*sizeof(available_game_data));
					
		  // bump the count.
		 			for(index= 0; index<inConfig->new_game_count; ++index)
		    		{
				    	if (!(inConfig->new_games[index].flags & _duplicate_game_flag)) // not duplicate
						{
							available_game_data *new_game= &new_games[inConfig->game_count++];
						// copy new game, counter is incremented
							machine_copy_data(&inConfig->new_games[index], new_game, sizeof(available_game_data));
						}
		    		}
		  			if (inConfig->games) 
		  				dispose_pointer(inConfig->games);
					inConfig->games= new_games;
				}
	    	}

	  // reset the new game count, since it is now in our global...			
			inConfig->new_game_count= 0;
		}

		// Give their callback the items...
     	index= 0;
		while(index<inConfig->game_count)
		{
	  		NMEnumerationItem item;
			
			if(inConfig->games[index].flags & _new_game_flag)
	    	{
				inConfig->games[index].flags &= ~_new_game_flag;
				item.id= inConfig->games[index].host;
				item.name= inConfig->games[index].name;
			//DEBUG_PRINT("giving callbacks item %d (Host: 0x%x port: %d) as new game", index, inConfig->games[index].host, inConfig->games[index].port);
				inConfig->callback(inConfig->user_context, kNMEnumAdd, &item);
				// next!
	   			index++;
	   		} else {
				// Drop if necessary

				if (machine_tick_count()-inConfig->games[index].ticks_at_last_response>TICKS_BEFORE_GAME_DROPPED)
				{
					// drop this one..
					item.id= inConfig->games[index].host;
					item.name= inConfig->games[index].name;
					//DEBUG_PRINT("giving callbacks item %d as delete game", index);
					inConfig->callback(inConfig->user_context, kNMEnumDelete, &item);
					machine_move_data(&inConfig->games[index+1],&inConfig->games[index], 
					  (inConfig->game_count-index-1)*sizeof(available_game_data));
					inConfig->game_count-= 1;
				} else {
					  // next!
				  index++;
				}
	   		}
		}

		/* ..and request more games...... */
    	err= send_game_request_packet(inConfig);
    } else {
		err= kNMNotEnumeratingErr;
    }
	
	return err;
}

//----------------------------------------------------------------------------------------
// NMEndEnumeration
//----------------------------------------------------------------------------------------

NMErr
NMEndEnumeration(
	NMConfigRef inConfig)
{
	NMErr err= kNMNoError;

	op_vassert_return((inConfig != NULL),"inConfig is NIL!",kNMParameterErr);
    op_vassert_return(inConfig->cookie==config_cookie, csprintf(sz_temporary, "cookie: 0x%x != 0x%x", inConfig->cookie, config_cookie),kNMParameterErr); 
	
	//we do nothing for inactive enumeration
	if (inConfig->activeEnumeration == false)
		return kNMNoError;
	
	if(inConfig->enumerating)
    {
		op_assert(inConfig->enumerating);
		if(inConfig->game_count)
		{
			inConfig->game_count= 0;
			dispose_pointer(inConfig->games);
			inConfig->games= NULL;
		}
		inConfig->callback= NULL;
		inConfig->enumerating= FALSE;
		
		op_assert(inConfig->enumeration_socket!=INVALID_SOCKET);
		err = closesocket(inConfig->enumeration_socket);
	 	DEBUG_NETWORK_API("Closesocet on Enumeration", err);
		inConfig->enumeration_socket= INVALID_SOCKET;

    } else {
		err= kNMNotEnumeratingErr;
    }
	
  return err;
}

/* ---------- local code.. */

//----------------------------------------------------------------------------------------
// handle_game_enumeration_packet
//----------------------------------------------------------------------------------------

static void
handle_game_enumeration_packet(	NMConfigRef 					inConfig,
							  	IPEnumerationResponsePacket *	packet,
							  	NMUInt32						host,
							  	NMUInt16						port)
{
	if (inConfig && inConfig->enumerating)
    {
		/* Byteswap the packet.. */
#ifdef windows_build
		packet->host = host; // in network byte order.
		packet->port = port;
		byteswap_ip_enumeration_packet((char *) packet);
#endif

		if (inConfig->gameID==packet->gameID && inConfig->new_game_count<MAXIMUM_GAMES_ALLOWED_BETWEEN_IDLE)
		{
			available_game_data *new_game;

			new_game= &inConfig->new_games[inConfig->new_game_count++];
			machine_mem_zero(new_game, sizeof(available_game_data));
			new_game->host= packet->host;
			new_game->port= packet->port;
			new_game->flags= _new_game_flag;
			new_game->ticks_at_last_response= machine_tick_count();
			strncpy(new_game->name, packet->name, kMaxGameNameLen);
			new_game->name[kMaxGameNameLen]= '\0';
		}
    }	
}

//----------------------------------------------------------------------------------------
// send_game_request_packet
//----------------------------------------------------------------------------------------

static NMErr 
send_game_request_packet( NMConfigRef inConfig)
{
NMErr err = kNMNoError;

	if (machine_tick_count()-inConfig->ticks_at_last_enumeration_request > TICKS_BETWEEN_ENUMERATION_REQUESTS)
    {
    	struct sockaddr_in	dest_address;
    	char 				request_packet[kQuerySize];
    	NMSInt32			bytes_sent;
     	NMSInt32			packet_length;
		
    // Doing this from memory...
		packet_length= build_ip_enumeration_request_packet(request_packet);
		op_assert(packet_length<=sizeof(request_packet));

  	// Build the address.
	    dest_address.sin_family = AF_INET;
	    dest_address.sin_addr.s_addr = INADDR_BROADCAST;    

    // dest_address.sin_addr.s_addr= inConfig->hostAddr.sin_addr | ~inConfig->hostAddr.sin_mask;
    
    //  dest_address.sin_port = htons(inConfig->hostAddr.sin_port); 
    	dest_address.sin_port = inConfig->hostAddr.sin_port;  	/* [Edmark/PBE] 11/12/99 is already in network order */

    // Send the request.
     	op_assert(inConfig->enumeration_socket != INVALID_SOCKET);
		bytes_sent = sendto(inConfig->enumeration_socket, request_packet, packet_length, 0, 
							 (LPSOCKADDR) &dest_address, sizeof(dest_address));
		if (bytes_sent == SOCKET_ERROR)
		{
			err = bytes_sent;
			DEBUG_NETWORK_API("SendTo", err);
		} else {
			op_assert(bytes_sent==packet_length);
		}
		
    	inConfig->ticks_at_last_enumeration_request= machine_tick_count();
    }
    	
    return err;
}

//----------------------------------------------------------------------------------------
// handle_packets
//----------------------------------------------------------------------------------------

static void 
handle_packets(	NMConfigRef inConfig)
{
	struct sockaddr_in	source_address;
	NMSInt32			bytes_read;
	NMBoolean			done= FALSE;
	int					source_address_length = sizeof(source_address);

	op_assert(inConfig->enumerating);
 	op_assert(inConfig->enumeration_socket != INVALID_SOCKET);

 	while (!done)
    {
		bytes_read= recvfrom(inConfig->enumeration_socket, inConfig->buffer, MAXIMUM_CONFIG_LENGTH,
							   0, (LPSOCKADDR) &source_address, &source_address_length);
		if (bytes_read >= 0)
		{
			IPEnumerationResponsePacket *packet= (IPEnumerationResponsePacket *) inConfig->buffer;
			
	  		if (packet->responseCode==SWAP4(kReplyFlag))
	    	{
				DEBUG_PRINT("Got a response packet");
				// deal with it.
				handle_game_enumeration_packet(	inConfig, packet, source_address.sin_addr.s_addr, 
											    source_address.sin_port);
		    } else {
				DEBUG_PRINT("Got a response packet with a size of %d but a response code of: 0x%x", bytes_read, packet->responseCode);
		    }
		} else {

			NMErr err;
			
			err= WSAGetLastError();
			if(err!=WSAEWOULDBLOCK)
	    	{
		   		DEBUG_NETWORK_API("RecvFrom for enumeration", err);
	   		}
			done = TRUE;
		}
    }
}
