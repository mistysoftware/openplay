/* 
 *-------------------------------------------------------------
 * Description:
 *   Functions which handle configuration
 *
 *------------------------------------------------------------- 
 *   Author: Kevin Holbrook
 *  Created: June 23, 1999
 *
 * Copyright (c) 1999-2002 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * Portions Copyright (c) 1999-2002 Apple Computer, Inc.  All Rights
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

#ifndef __NETMODULE__
#include 			"NetModule.h"
#endif
#include "tcp_module.h"


/* 
 * Static Function: _generate_default_port
 *--------------------------------------------------------------------
 * Parameters:
 *  [IN] GameId =
 *
 * Returns:
 *   port number
 *
 * Description:
 *   Function to create a default port number given a game id
 *
 *--------------------------------------------------------------------
 */

static short _generate_default_port(NMUInt32 GameID)
{
	DEBUG_ENTRY_EXIT("_generate_default_port");

  return (GameID % (32760 - 1024)) + 1024;
}


/* 
 * Static Function: build_standard_config_strings
 *--------------------------------------------------------------------
 * Parameters:
 *  [IN]  config =
 *
 * Returns:
 *   True  = 
 *   False = 
 *
 * Description:
 *   Function 
 *
 *--------------------------------------------------------------------
 */

static NMBoolean _build_standard_config_strings(NMConfigRef config)
{
	DEBUG_ENTRY_EXIT("_build_standard_config_strings");

  NMBoolean success = false;
  NMBoolean status;


  op_assert(config->cookie == config_cookie);

  /* put the type */
  status = put_token(config->buffer, MAXIMUM_CONFIG_LENGTH, kConfigModuleType, LONG_DATA, &config->type, sizeof(long));

  if (status)
  {
    /* put the version */
    status = put_token(config->buffer, MAXIMUM_CONFIG_LENGTH, kConfigModuleVersion, LONG_DATA, &config->version, sizeof(long));

    if (status)
    {
      /* put the gameID */
      status = put_token(config->buffer, MAXIMUM_CONFIG_LENGTH, kConfigGameID, LONG_DATA, &config->gameID, sizeof(long));

      if (status)
      {
        /* put the gameName */
        status = put_token(config->buffer, MAXIMUM_CONFIG_LENGTH, kConfigGameName, STRING_DATA, &config->name, strlen(config->name));

        if(status)
        {
          /* put the mode */
          status = put_token(config->buffer, MAXIMUM_CONFIG_LENGTH, kConfigEndpointMode, LONG_DATA, &config->connectionMode, sizeof(long));

          if (status)
		 	{
		 		//put netsprocket mode
		 		if(put_token(config->buffer, MAXIMUM_CONFIG_LENGTH, kConfigNetSprocketMode, BOOLEAN_DATA, &config->netSprocketMode, sizeof(NMBoolean)))
      				success= true;
      		}
        }
      }
    }
  }
	
  return success;
} /*  _build_standard_config_strings*/


/* 
 * Static Function: build_config_string_into_config_buffer
 *--------------------------------------------------------------------
 * Parameters:
 *  [IN]  config   =
 *
 * Returns:
 *   none
 *
 * Description:
 *   Function 
 *
 *--------------------------------------------------------------------
 */

static void _build_config_string_into_config_buffer(NMConfigRef config)
{
	DEBUG_ENTRY_EXIT("_build_config_string_into_config_buffer");

  NMBoolean success = false;
  NMBoolean status;


  config->buffer[0] = 0;
  success = _build_standard_config_strings(config);

  if ( success ) /* insert module specific information */
  {
    /* insert HOST name */
    status = put_token(config->buffer, MAXIMUM_CONFIG_LENGTH, kIPConfigAddress, 
                       STRING_DATA, config->host_name, strlen(config->host_name));

    if (status)
    {
      long port = ntohs(config->hostAddr.sin_port);
		
      /* insert PORT, in host byte order */
      status = put_token(config->buffer, MAXIMUM_CONFIG_LENGTH, kIPConfigPort, LONG_DATA, &port, sizeof(long));

      if(status)
        success = true;
    }
  }
	
  if (!success)
  {
    DEBUG_PRINT("Unable to build the config string into the config buffer!");
  }

  return;
} /* _build_config_string_into_config_buffer */


/* 
 * Static Function: _get_standard_config_strings
 *--------------------------------------------------------------------
 * Parameters:
 *  [IN]  string = 
 *  [IN]  config =
 *
 * Returns:
 *   
 *   
 *
 * Description:
 *   Function 
 *
 *--------------------------------------------------------------------
 */

static NMErr _get_standard_config_strings(char *string, NMConfigRef config)
{
	DEBUG_ENTRY_EXIT("_get_standard_config_strings");

  long       length;
  NMBoolean  status;
  NMType     type;
  NMErr      error = kNMInvalidConfigErr;
	

  length = sizeof(type);
  status = get_token(string, kConfigModuleType, LONG_DATA, &type, &length);

  if (status)
  {
    if (type == config->type)
    {
      long version;
			
      /* newer versions should handle older configs */
      length = sizeof(version);
      status = get_token(string, kConfigModuleVersion, LONG_DATA, &version, &length);

      if (status)
      {
        if (version == kVersion)
        {
          /* get the gameID */
          length = sizeof(config->gameID);
          status = get_token(string, kConfigGameID, LONG_DATA, &config->gameID, &length);

          if (status)
          {
            /* get the game name */
            length = kMaxGameNameLen;
            status = get_token(string, kConfigGameName, STRING_DATA, config->name, &length);

            if(status)
            {
              /* get the mode */
              length = sizeof(config->connectionMode);
              status = get_token(string, kConfigEndpointMode, LONG_DATA, &config->connectionMode, &length);
              if (status)
	        error = 0;
            }
          }
        }
      }
    }
  }

  return error;
} /*  _get_standard_config_strings */


/* 
 * Static Function: _parse_config_string
 *--------------------------------------------------------------------
 * Parameters:
 *  [IN]  string =
 *  [IN]  GameID =
 *  [IN]  Config = 
 *
 * Returns:
 *   
 *   
 *
 * Description:
 *   Function 
 *
 *--------------------------------------------------------------------
 */

static NMErr _parse_config_string(char *string, NMUInt32 GameID, NMConfigRef config)
{
	DEBUG_ENTRY_EXIT("_parse_config_string");

  NMErr      err;
  NMBoolean  status;
	

  err = _get_standard_config_strings(string, config);

  if (!err)
  {
    long length;

    err = kNMInvalidConfigErr;


	// Get the "NetSprocket Mode".
	length = sizeof(NMBoolean);
	if (!get_token(string, kConfigNetSprocketMode, BOOLEAN_DATA, &config->netSprocketMode, &length))
		config->netSprocketMode = kDefaultNetSprocketMode;

		
    length = sizeof(config->host_name);
    status = get_token(string, kIPConfigAddress, STRING_DATA, &config->host_name, &length);

    if (status)
    {
      long port = _generate_default_port(GameID);

      length = sizeof(port);
      status = get_token(string, kIPConfigPort, LONG_DATA, &port, &length);

      if (status)
      {
        if (port >= 0 && port <= 65535)
	{
          config->hostAddr.sin_port = htons(port);
          err = 0;
	}
      }
    }
  }
	
  return err;
} /* _parse_config_string */


/* 
 * Static Function: _get_default_port
 *--------------------------------------------------------------------
 * Parameters:
 *  [IN]  GameID =
 *
 * Returns:
 *   
 *
 * Description:
 *   Function 
 *
 *--------------------------------------------------------------------
 */

static short _get_default_port(NMUInt32 GameID)
{
	DEBUG_ENTRY_EXIT("_get_default_port");

  return((GameID % (32760 - 1024)) + 1024);
}


/* 
 * Function: NMCreateConfig
 *--------------------------------------------------------------------
 * Parameters:
 *  [IN]  ConfigStr   =
 *  [IN]  GameID      =
 *  [IN]  GameName    =
 *  [IN]  EnumData    =
 *  [IN]  EnumDataLen =
 *  [OUT] Config      =
 *
 * Returns:
 *   Network module error
 *
 * Description:
 *   Function 
 *
 *--------------------------------------------------------------------
 */

NMErr NMCreateConfig(char *ConfigStr, 
	             NMType GameID, 
                     const char *GameName, 
	             const void *EnumData, 
                     NMUInt32 EnumDataLen,
	             NMConfigRef *Config)
{
	DEBUG_ENTRY_EXIT("NMCreateConfig");

	if (module_inited < 1)
		return kNMInternalErr;

	NMConfigRef _config;
	NMErr err = kNMNoError;
	int status;

	UNUSED_PARAMETER(EnumData)
	UNUSED_PARAMETER(EnumDataLen)

	//make sure we've inited winsock (doing this duringDllMain causes problems)
	#ifdef OP_API_NETWORK_WINSOCK
		initWinsock();
	#endif

	//make sure a worker-thread is running if need-be (doing this in _init can cause problems)
	#if (USE_WORKER_THREAD)	
		createWorkerThread();
	#endif

	_config = (struct NMProtocolConfigPriv *) new_pointer(sizeof(struct NMProtocolConfigPriv));

	if (_config)
	{
		_config->cookie  = config_cookie;
		_config->type    = kModuleID;
		_config->version = kVersion;
		_config->gameID  = GameID;

		_config->hostAddr.sin_family = AF_INET;
		_config->hostAddr.sin_port = htons( _get_default_port(GameID) );
		_config->hostAddr.sin_addr.s_addr = INADDR_NONE;

		_config->enumeration_socket = INVALID_SOCKET;
		_config->enumerating = false;
		_config->connectionMode = 3; /* stream and datagram. */
		_config->netSprocketMode = kDefaultNetSprocketMode;
		_config->callback = NULL;
		_config->games = NULL;
		_config->game_count = 0;
		_config->new_game_count = 0;
	}
	else
	{
		*Config = NULL;
		return(kNMOutOfMemoryErr);
	}

	status = gethostname(_config->host_name, 256);

	if (GameName)
	{
		strncpy(_config->name, GameName, kMaxGameNameLen);
		_config->name[kMaxGameNameLen] = '\0';
	}
	else
		_config->name[0] = '\0';

	if (ConfigStr)
		err = _parse_config_string(ConfigStr, GameID, _config);

	if (err)
	{
		free(_config);
		*Config = NULL;
		return(err);
	}

	*Config = _config;
	return(kNMNoError);

} /* NMCreateConfig */


/* 
 * Function: NMGetConfigLen
 *--------------------------------------------------------------------
 * Parameters:
 *  [IN] Config = ptr to configuration data structure
 *
 * Returns:
 *  length of configuration string 
 *
 * Description:
 *   Function to get length of configuration string.
 *
 *--------------------------------------------------------------------
 */

short NMGetConfigLen(NMConfigRef Config)
{
	DEBUG_ENTRY_EXIT("NMGetConfigLen");

	if (module_inited < 1)
		return 0;

	if (Config)
	{
		_build_config_string_into_config_buffer(Config);

		return( strlen(Config->buffer) );
	}
	else
		return(0);

} /* NMGetConfigLen */


/* 
 * Function: NMGetConfig
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

NMErr NMGetConfig(NMConfigRef inConfig, char *outConfigStr, short *ioConfigStrLen)
{
	DEBUG_ENTRY_EXIT("NMGetConfig");

	if (module_inited < 1)
		return kNMInternalErr;

	NMErr err;

	op_vassert_return((inConfig != NULL),"Config ref is NULL!",kNMParameterErr);
	op_vassert_return((outConfigStr != NULL),"outConfigStr is NULL!",kNMParameterErr);
	op_vassert_return((ioConfigStrLen != NULL),"ioConfigStrLen is NULL!",kNMParameterErr);
	op_vassert_return((inConfig->cookie==config_cookie),"inConfig->cookie==config_cookie",kNMParameterErr);

	_build_config_string_into_config_buffer(inConfig);
	
	strncpy(outConfigStr, inConfig->buffer, *ioConfigStrLen);
	if(*ioConfigStrLen< (NMSInt16)strlen(inConfig->buffer))
	{
		err= kNMConfigStringTooSmallErr;
		
	} else {
		*ioConfigStrLen= strlen(inConfig->buffer);
		err= kNMNoError;
	}

	return err;
} /* NMGetConfig */


/* 
 * Function: NMDeleteConfig
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

NMErr NMDeleteConfig(NMConfigRef inConfig)
{
	DEBUG_ENTRY_EXIT("NMDeleteConfig");

	if (module_inited < 1)
		return kNMInternalErr;

	op_vassert_return((inConfig != NULL),"Config ref is NULL!",kNMParameterErr);
	op_vassert_return((inConfig->cookie==config_cookie),"inConfig->cookie==config_cookie",kNMParameterErr);

	inConfig->cookie= 'bad ';
	dispose_pointer(inConfig);
	
	return kNMNoError;
} /* NMDeleteConfig */

