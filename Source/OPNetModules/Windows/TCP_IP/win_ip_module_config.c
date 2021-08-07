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
	module_config.c
*/

// Issues:
// 1) config should keep name around for DNS.

#ifndef __OPENPLAY__
#include "OpenPlay.h"
#endif
#include "OPUtils.h"
#include "NetModule.h"
#include "ip_enumeration.h"
#include "win_ip_module.h"
#include "configuration.h"
#include "configfields.h"

#define PARSE_CHARACTER '\t'

const char *kIPConfigAddress = "IPaddr";
const char *kIPConfigPort = "IPport";

/* ------- local prototypes */
static NMSInt16 GenerateDefaultPort(NMType inGameID);
static NMErr parse_config_string(char *string, NMUInt32 inGameID, NMConfigRef config);
static void build_config_string_into_config_buffer(NMConfigRef inConfig);
static NMErr get_standard_config_strings(char *string, NMConfigRef config);

/* Configurator functions */


//----------------------------------------------------------------------------------------
// NMCreateConfig
//----------------------------------------------------------------------------------------

NMErr
NMCreateConfig(
			      char *inConfigStr, 
			      NMType inGameID, 
			      const char *inGameName, 
			      const void *inEnumData,
			      NMUInt32 inEnumDataLen,
			      NMConfigRef *outConfig)
{
	NMConfigRef config;
	NMErr err= kNMNoError;

	UNUSED_PARAMETER(inEnumData)
	UNUSED_PARAMETER(inEnumDataLen)

	if (inConfigStr == NULL)
	{
		op_vassert_return((inGameName != NULL),"Game name is NIL!",kNMParameterErr);
		op_vassert_return((inGameID != 0),"Game ID is 0.  Use your creator type!",kNMParameterErr);
	}

	op_vassert_return((outConfig != NULL),"outConfig pointer is NIL!",kNMParameterErr);

	*outConfig= NULL;
	config= (NMConfigRef) new_pointer(sizeof(NMProtocolConfigPriv));
	if(config)
    {
		machine_mem_zero(config, sizeof(NMProtocolConfigPriv));

		// Fill in the defaults.
		config->cookie= config_cookie;
		config->type= kModuleID;
		config->version= kVersion;
		config->gameID= inGameID;
		config->netSprocketMode = kDefaultNetSprocketMode;
		config->hostAddr.sin_family = AF_INET;
		config->hostAddr.sin_port = htons(GenerateDefaultPort(inGameID));
		config->hostAddr.sin_addr.S_un.S_addr = INADDR_NONE;
		config->enumeration_socket = INVALID_SOCKET;
		config->connectionMode= 3; // stream and datagram.

		nm_getcstr(config->host_name, strNAMES, idDefaultHost, sizeof(config->host_name));

		if(inGameName)
		{
			strncpy(config->name, inGameName, kMaxGameNameLen);
			config->name[kMaxGameNameLen]= '\0';
		} else {
			config->name[0]= '\0';
		}

		if(inConfigStr)
		{
	 	 // Parse it.
			err= parse_config_string(inConfigStr, inGameID, config);
		}
		
		if(err)
		{
	  		dispose_pointer(config);
	  		config= NULL;
		}
		
     	 // return it..
    	*outConfig= config;
	} else {
      err= kNMOutOfMemoryErr;
	}

	return err;
}

//----------------------------------------------------------------------------------------
// NMGetConfigLen
//----------------------------------------------------------------------------------------

NMSInt16
NMGetConfigLen(
    NMConfigRef inConfig)
{
	op_vassert_return((inConfig != NULL),"Config ref is NULL!",0);
	op_vassert_return((inConfig->cookie==config_cookie),"inConfig->cookie==config_cookie",kNMParameterErr);

	build_config_string_into_config_buffer(inConfig);
	return strlen(inConfig->buffer);
}

//----------------------------------------------------------------------------------------
// NMGetConfig
//----------------------------------------------------------------------------------------

NMErr
NMGetConfig(
   NMConfigRef inConfig, 
   char *outConfigStr, 
   NMSInt16 *ioConfigStrLen)
{
	NMErr err;

	op_vassert_return((inConfig != NULL),"Config ref is NULL!",kNMParameterErr);
	op_vassert_return((outConfigStr != NULL),"outConfigStr is NULL!",kNMParameterErr);
	op_vassert_return((ioConfigStrLen != NULL),"ioConfigStrLen is NULL!",kNMParameterErr);
	op_vassert_return((inConfig->cookie==config_cookie),"inConfig->cookie==config_cookie",kNMParameterErr);

	build_config_string_into_config_buffer(inConfig);
	
	strncpy(outConfigStr, inConfig->buffer, *ioConfigStrLen);
	if(*ioConfigStrLen< (NMSInt16)strlen(inConfig->buffer))
	{
		err= kNMConfigStringTooSmallErr;
		
	} else {
		*ioConfigStrLen= strlen(inConfig->buffer);
		err= kNMNoError;
	}

	return err;
}

//----------------------------------------------------------------------------------------
// NMDeleteConfig
//----------------------------------------------------------------------------------------

NMErr
NMDeleteConfig(
			      NMConfigRef inConfig)
{
	op_vassert_return((inConfig != NULL),"Config ref is NULL!",kNMParameterErr);
	op_vassert_return((inConfig->cookie==config_cookie),"inConfig->cookie==config_cookie",kNMParameterErr);

	inConfig->cookie= 'bad ';
	dispose_pointer(inConfig);
	
	return kNMNoError;
}

/* ------- local prototypes */

//----------------------------------------------------------------------------------------
// GenerateDefaultPort
//----------------------------------------------------------------------------------------

static NMSInt16 GenerateDefaultPort(
				 NMType inGameID)
{
	return (NMSInt16)((inGameID % (32760 - 1024)) + 1024);
}

//----------------------------------------------------------------------------------------
// my_char_token
//----------------------------------------------------------------------------------------

static char *my_char_token(
			   char *string,
			   char parse_character)
{
	static char *next_start;
	char *start;
	char *end;
	
	if(string)
    {
		next_start= string;
    }
	
	start= next_start;
	end= start;
	
	if(start)
    {
   		while(*end && *end != parse_character)
		{
	 		end++;
		}
 		next_start= (*end) ? end+1 : NULL;
		*end= '\0';
	}
	
	return start;
}

//----------------------------------------------------------------------------------------
// get_standard_config_strings
//----------------------------------------------------------------------------------------

static NMErr 
get_standard_config_strings(
					 char *string,
					 NMConfigRef config)
{
	NMSInt32	length;
	NMType	type;
	NMErr 	err = kNMInvalidConfigErr; // worst case
	
	length= sizeof(type);
  	if (get_token(string, kConfigModuleType, LONG_DATA, &type, &length))
    {
    	if(type==config->type)
		{
			NMSInt32	version;
			
	  // newer versions should handle older configs, but we don't have to worry about that yet.
	 		length= sizeof(version);
		  	if (get_token(string, kConfigModuleVersion, LONG_DATA, &version, &length))
	    	{
	      		if(version==kVersion)
				{
		  // get the gameID
		  			length= sizeof(config->gameID);
		  			if(get_token(string, kConfigGameID, LONG_DATA, &config->gameID, &length))
		    		{
		      	// get the game name
		      			length= kMaxGameNameLen;
		   				if(get_token(string, kConfigGameName, STRING_DATA, config->name, &length))
						{
					  // get the mode
							length= sizeof(config->connectionMode);
			 				if(get_token(string, kConfigEndpointMode, LONG_DATA, &config->connectionMode, &length))
								err= kNMNoError;			      
						}
		    		}
				}
	    	}			
		}
    }

	return err;
}

//----------------------------------------------------------------------------------------
// build_standard_config_strings
//----------------------------------------------------------------------------------------

static NMBoolean 
build_standard_config_strings(
					       NMConfigRef inConfig)
{
	NMBoolean success= FALSE;

  // put the type
	if(put_token(inConfig->buffer, MAXIMUM_CONFIG_LENGTH, kConfigModuleType, LONG_DATA, &inConfig->type, sizeof(NMSInt32)))
    {
      // put the version
      	if(put_token(inConfig->buffer, MAXIMUM_CONFIG_LENGTH, kConfigModuleVersion, LONG_DATA, &inConfig->version, sizeof(NMSInt32)))
		{
		  // put the gameID
			if(put_token(inConfig->buffer, MAXIMUM_CONFIG_LENGTH, kConfigGameID, LONG_DATA, &inConfig->gameID, sizeof(NMSInt32)))
	    	{
				// put the gameName
	      		if(put_token(inConfig->buffer, MAXIMUM_CONFIG_LENGTH, kConfigGameName, STRING_DATA, &inConfig->name, strlen(inConfig->name)))
				{
				  // put the mode..
				 	if(put_token(inConfig->buffer, MAXIMUM_CONFIG_LENGTH, kConfigEndpointMode, LONG_DATA, &inConfig->connectionMode, sizeof(NMSInt32)))
				 	{
				 		//put netsprocket mode
				 		if(put_token(inConfig->buffer, MAXIMUM_CONFIG_LENGTH, kConfigNetSprocketMode, BOOLEAN_DATA, &inConfig->netSprocketMode, sizeof(NMBoolean)))
		      				success= TRUE;
		      		}
				}		
	    	}
		}
    }
	
	return success;
}

//----------------------------------------------------------------------------------------
// build_config_string_into_config_buffer
//----------------------------------------------------------------------------------------

static void 
build_config_string_into_config_buffer(
						   NMConfigRef inConfig)
{
	NMBoolean success= FALSE;

  	inConfig->buffer[0]= 0;
  	if (build_standard_config_strings(inConfig))
    {
      // now put the ip specific ones.
      // HOST
    	if(put_token(inConfig->buffer,MAXIMUM_CONFIG_LENGTH, kIPConfigAddress, STRING_DATA, inConfig->host_name, strlen(inConfig->host_name)))
		{
	  	// 19990330 this is in host byte order. Remember that.
	  		NMSInt32 port= ntohs((u_short) inConfig->hostAddr.sin_port);
		
	  // PORT
	  		if(put_token(inConfig->buffer, MAXIMUM_CONFIG_LENGTH, kIPConfigPort, LONG_DATA, &port, sizeof(NMSInt32)))
	    	{
	      		success= TRUE;
	    	}
		}
    }
	
  	if (!success)
    {
      DEBUG_PRINT("Unable to build the inConfig string into the inConfig buffer!");
    }
}

//----------------------------------------------------------------------------------------
// parse_config_string
//----------------------------------------------------------------------------------------

static NMErr 
parse_config_string(
				 char *			string, 
				 NMUInt32 		inGameID,
				 NMConfigRef	inConfig)
{
	NMErr err;
	
	err= get_standard_config_strings(string, inConfig);
	if(!err)
    {
    	NMSInt32 length;

      // pessimism reigns.
    	err= kNMInvalidConfigErr;
	
	//	CRT, April 17, 2001
	// Get the "NetSprocket Mode".  Default to "false" if it isn't found...
		length = sizeof(NMBoolean);
		if (!get_token(string, kConfigNetSprocketMode, BOOLEAN_DATA, &inConfig->netSprocketMode, &length))
			inConfig->netSprocketMode = kDefaultNetSprocketMode;
		
	//	CRT, April 17, 2001
	//	Why are we erring out if the IP address and port aren't found in the inConfig
	//	string?  Shouldn't they be set to default values such as 127.0.0.1 and the
	//	value returned by GenerateDefaultPort(inGameID), respectively, and then return
	//	with no error?
	
    	length= sizeof(inConfig->host_name);
   		if(get_token(string, kIPConfigAddress, STRING_DATA, &inConfig->host_name, &length))
		{
	  // 19990330 this is in host byte order. Remember that.
			NMSInt32 port = GenerateDefaultPort(inGameID);
			
	  		length = sizeof(port);
	  		if(get_token(string, kIPConfigPort, LONG_DATA, &port, &length))
	    	{
				// we win.
	     		op_assert(port>=0 && port<=65535);
	      		err= kNMNoError;
	    	}
	 		inConfig->hostAddr.sin_port= htons((u_short) port);
		}
    }
	
  return err;
}

