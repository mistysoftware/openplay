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
#include 			"OpenPlay.h"
#endif
#ifndef __NETMODULE__
#include 			"NetModule.h"
#endif


#ifdef OP_API_PLUGIN_MACHO
	#ifndef __CORESERVICES__
		#include <CoreServices/CoreServices.h>
	#endif
#endif




#include "OPUtils.h"
#include "portable_files.h"
#include "find_files.h"
#include "op_definitions.h"
#include "op_globals.h"
#include "op_resources.h"
#include "OPDLLUtils.h"
#include "String_Utils.h"
#include "module_management.h"

#ifdef OP_API_PLUGIN_WINDOWS
    #define MAX_STRINGS_PER_GROUP 128 /* must match that in build_rc.c */
#endif

/* -------- local prototypes */

#ifdef OP_API_PLUGIN_MACHO
	extern "C"{
	void _init(void);
	}
#endif

/*--------- local data */

#ifdef OP_API_PLUGIN_MAC_CFM
	static NMBoolean haveRefreshedProtocols = false;
#endif //OP_API_PLUGIN_MAC_CFM

/* -------- Protocol Management Layer */


 //doxygen instruction:
/** @addtogroup Configuration
 * @{
 */

//----------------------------------------------------------------------------------------
// GetIndexedProtocol
//----------------------------------------------------------------------------------------
/**
	Retrieve info about available OpenPlay protocols.
	@brief Retrieve info about available OpenPlay protocols.
	@param index The number of the protocol to return.  The first protocol has an index of 0.
	@param protocol The \ref NMProtocolStruct to be filled in with the protocol's info.
	@return \ref kNMNoError on success. \n
	\ref kNMNoMoreNetModulesErr if there is no protocol at the provided index.
	\n\n\n\n
 */
/* Find the folder with the protocols, find all the protocols, and return the index'd name. */
NMErr GetIndexedProtocol(
	NMSInt16 index,
	NMProtocolStruct *protocol)
{
	NMErr err;

#ifdef OP_API_PLUGIN_MAC_CFM
	if (haveRefreshedProtocols == false){
		refresh_protocols();
		haveRefreshedProtocols = true;
	}
#endif //OP_API_PLUGIN_MAC_CFM
	
	/* call as often as possible (anything that is synchronous) */
	op_idle_synchronous(); 

	/* Use this while loop */
	if(protocol->version==CURRENT_OPENPLAY_VERSION)
	{
		if(index>= 0)
		{
			if(index<gOp_globals.module_count)
			{
				if(protocol)
				{
					op_assert(gOp_globals.modules);
					machine_copy_data(&gOp_globals.modules[index].module, protocol, sizeof(NMProtocol));
					err= kNMNoError; /* no error */
				} else {
					err= kNMParameterErr;
				}
			} else {
				err= kNMNoMoreNetModulesErr;
			}
		} else {
			err= kNMParameterErr;
		}
	} else {
		err= kNMBadVersionErr;
	}

	return err;
}

//----------------------------------------------------------------------------------------
// ProtocolCreateConfig
//----------------------------------------------------------------------------------------
/**
	Creates a new \ref PConfigRef of the given type using either default values or a configuration string.
	@brief Create a new \ref PConfigRef using default values or a configuration string.
	@param type The type of protocol to create.
	@param game_id The game's type, used to locate fellow games on the network when enumerating.
	@param game_name The game's name.
	@param enum_data Custom enumeration data, to be passed to enumeration callbacks, or NULL if not needed.
	@param enum_data_len Length of the enumeration data.  The data is copied and stored in its entirety in the \ref PConfigRef.
	@param configuration_string A configuration string to use when creating the \ref PConfigRef, or NULL if default values are desired.
	@param inConfig Pointer to a \ref PConfigRef, to be filled in by the function.
	@return \ref kNMNoError if the function succeeds.\n
	Otherwise, returns an error code.
	\n\n\n\n
 */
/*
	Binds to the protocol_names library, mallocs a structure (which specifies 
	the FSSpec of the library), calls its internal Config(char *), and assigns 
	its return value to part of its structure, and then returns the conglomerate
	If configuration_string is NULL, creates default...
*/	

NMErr ProtocolCreateConfig(
	NMType type,
	NMType game_id, 
	const char *game_name,
	const void *enum_data,
	NMUInt32 enum_data_len,
	char *configuration_string,
	PConfigRef *inConfig)
{
	PConfigRef config= (PConfigRef) NULL;
	NMErr err= kNMNoError;

#ifdef OP_API_PLUGIN_MAC_CFM
	if (haveRefreshedProtocols == false) {
		refresh_protocols();
		haveRefreshedProtocols = true;
	}
#endif //OP_API_PLUGIN_MAC_CFM

	/* call as often as possible (anything that is synchronous) */
	op_idle_synchronous(); 

	config= (PConfigRef) new_pointer(sizeof(ProtocolConfig));
	if(config)
	{
		NMSInt16 index;
	
		/* Clear it.. */
		machine_mem_zero(config, sizeof(ProtocolConfig));
		config->type= type;

		/* Search.. */
		for(index= 0; index<gOp_globals.module_count; index++)
		{
			if(type==gOp_globals.modules[index].module.type)
			{
				err= bind_to_protocol(type, &gOp_globals.modules[index].module_spec, &config->connection);
				config->file= gOp_globals.modules[index].module_spec;
				break;
			}
		}

		if(!err && index!=gOp_globals.module_count)
		{
			/* Creation/deletion */
			config->NMCreateConfig= (NMCreateConfigPtr) load_proc(config->connection, kNMCreateConfig);
			config->NMGetConfigLen= (NMGetConfigLenPtr) load_proc(config->connection, kNMGetConfigLen);
			config->NMGetConfig= (NMGetConfigPtr) load_proc(config->connection, kNMGetConfig);
			config->NMDeleteConfig= (NMDeleteConfigPtr) load_proc(config->connection, kNMDeleteConfig);
			config->NMFunctionPassThrough= (NMFunctionPassThroughPtr) load_proc(config->connection, kNMFunctionPassThrough);

			/* Dialog functions */
			config->NMGetRequiredDialogFrame= (NMGetRequiredDialogFramePtr) load_proc(config->connection, kNMGetRequiredDialogFrame);
			config->NMSetupDialog= (NMSetupDialogPtr) load_proc(config->connection, kNMSetupDialog);
			config->NMHandleEvent= (NMHandleEventPtr) load_proc(config->connection, kNMHandleEvent);
			config->NMHandleItemHit= (NMHandleItemHitPtr) load_proc(config->connection, kNMHandleItemHit);
			config->NMTeardownDialog= (NMTeardownDialogPtr) load_proc(config->connection, kNMTeardownDialog);

			/* binding the enumeration stuff. */
			config->NMBindEnumerationItemToConfig = (NMBindEnumerationItemToConfigPtr) load_proc(config->connection, 
                                                                 kNMBindEnumerationItemToConfig);

			config->NMStartEnumeration = (NMStartEnumerationPtr) load_proc(config->connection, kNMStartEnumeration);
			config->NMIdleEnumeration  = (NMIdleEnumerationPtr)  load_proc(config->connection, kNMIdleEnumeration);
			config->NMEndEnumeration   = (NMEndEnumerationPtr)   load_proc(config->connection, kNMEndEnumeration);
/*			config->NMStopAdvertising  = (NMStopAdvertisingPtr)  load_proc(config->connection, kNMStopAdvertising); */
/*			config->NMStartAdvertising = (NMStartAdvertisingPtr) load_proc(config->connection, kNMStartAdvertising); */
			/* [Edmark/PBE] 11/8/99 moved NMStart/StopAdvertising from ProtocolConfig to Endpoint */

			/* And create the configuration data */		
                        /* Ryan, where do we get the game name?	*/
			err= config->NMCreateConfig(configuration_string, game_id, game_name, enum_data, enum_data_len, 
                                                     (NMConfigRef *)(&config->configuration_data));
			if(!err)
			{
				*inConfig= config;
			} else {
				ProtocolDisposeConfig(config);
			}
		} 
		else if(index==gOp_globals.module_count)
		{
			DEBUG_PRINT("Didn't find module type: 0x%x (%c%c%c%c) Count: %d", type, (type >> 24) & 0xFF, (type >> 16) & 0xFF,
			(type >> 8) & 0xFF, type & 0xFF, gOp_globals.module_count);

			err= kNMModuleNotFoundErr;	
		}
	} else {
		err= kNMOutOfMemoryErr;
	}
	
	return err;
}

//----------------------------------------------------------------------------------------
// ProtocolDisposeConfig
//----------------------------------------------------------------------------------------
/**
	Dispose of a \ref PConfigRef
	@brief Dispose of a \ref PConfigRef
	@param config the \ref PConfigRef to dispose of.
	@return \ref kNMNoError if the function succeeds.\n
	Otherwise, returns an error code.
	\n\n\n\n
 */
/*
	Rebinds, calls the protocol DisposeConfig, frees internal structures, and returns
*/
NMErr ProtocolDisposeConfig(
	PConfigRef config)
{
	NMErr err= kNMNoError;
	
	/* call as often as possible (anything that is synchronous) */
	op_idle_synchronous(); 

	if(config)
	{
		if(config->configuration_data) 
		{
			err= config->NMDeleteConfig((NMProtocolConfigPriv*)config->configuration_data);
			config->configuration_data= NULL;
		}
		unbind_from_protocol(config->type, true);
		dispose_pointer(config);
	}
	
	return err;
}

//----------------------------------------------------------------------------------------
// ProtocolGetConfigType
//----------------------------------------------------------------------------------------
/**
	Utility function to return the type of a \ref PConfigRef.
	@brief Utility function to return the type of a \ref PConfigRef.
	@param config The config to return the type of.
	@return The config's type.
	\n\n\n\n
 */
NMType ProtocolGetConfigType( PConfigRef config )
{
	if( config )
		return( config->type );
	else
		return( NULL );
}

//----------------------------------------------------------------------------------------
// ProtocolGetConfigString
//----------------------------------------------------------------------------------------
/**
	Extracts a configuration string from a \ref PConfigRef.
	@brief Extracts a configuration string from a \ref PConfigRef.
	@param config the \ref PConfigRef to extract a configuration string from.
	@param config_string The buffer where the configuration string should be placed.
	@param max_length The length of the provided buffer.
	@return \ref kNMNoError if the function succeeds.\n
	Otherwise, returns an error code.
	\n\n\n\n
 */
/*
	get the configuration string.
	(if you call it with a config_string of NULL, it will put the length into max_length for you..)
*/
NMErr ProtocolGetConfigString(
	PConfigRef config, 
	char *config_string, 
	NMSInt16 max_length)
{
	NMErr err= kNMNoError;

	if(config)
	{
		if(config->NMGetConfig)
		{
			err= config->NMGetConfig((NMProtocolConfigPriv*)config->configuration_data, config_string, &max_length);
		} else {
			err= kNMFunctionNotBoundErr;
		}
	} else {
		err= kNMInvalidConfigErr;
	}
	
	return err;
}

//----------------------------------------------------------------------------------------
// ProtocolGetConfigStringLen
//----------------------------------------------------------------------------------------
/**
	Get the required length for a \ref PConfigRef 's configuration string.
	@brief Get the required length for a \ref PConfigRef 's configuration string.
	@param config The \ref PConfigRef whose configuration string length is to be extracted.
	@param length Pointer to a value which is filled in with the configuration string's length.
	@return \ref kNMNoError if the function succeeds.\n
	Otherwise, returns an error code.
	\n\n\n\n
 */

/*
	get the configuration string.
	(if you call it with a config_string of NULL, it will put the length into max_length for you..)
*/
NMErr ProtocolGetConfigStringLen(
	PConfigRef config, 
	NMSInt16 *length)
{
	NMErr err= kNMNoError;
	
	if(config)
	{
		if(config->NMGetConfigLen)
		{
			*length= config->NMGetConfigLen((NMProtocolConfigPriv*)config->configuration_data);
		} else {
			err= kNMFunctionNotBoundErr;
		}
	} else {
		err= kNMInvalidConfigErr;
	}
	
	return err;
}

//----------------------------------------------------------------------------------------
// ProtocolConfigPassThrough
//----------------------------------------------------------------------------------------
/**
	Access NetModule-specific functionality for a \ref PConfigRef.
	@brief Access NetModule-specific functionality for a \ref PConfigRef.
	@param config The \ref PConfigRef on which the supplementary function acts.
	@param inSelector An index that tells the NetModule which custom operation is to be done.
	@param inParamBlock data to pass to the NetModule for the function.
	@return \ref kNMNoError if the function succeeds.\n
	\ref kNMUnknownPassThrough if the NetModule does not recognize the value of \e inSelector. \n
	Otherwise, returns an error code.
	\n\n\n\n
 
 */

NMErr ProtocolConfigPassThrough(
	PConfigRef config, 
	NMUInt32 inSelector, 
	void *inParamBlock)
{
	NMErr err= kNMNoError;
	
	if(config)
	{
		/* keep a local copy. */
		if(config->NMFunctionPassThrough)
		{
			err= config->NMFunctionPassThrough(NULL, inSelector, inParamBlock);
		} else {
			err= kNMFunctionNotBoundErr;
		}
	} else {
		err= kNMParameterErr;
	}

	return err;
}

/** @}*/
/* --------- local code */

//----------------------------------------------------------------------------------------
// build_find_protocol_search_start
//----------------------------------------------------------------------------------------

static void build_find_protocol_search_start(
	FileDesc *search_start)
{

#ifdef OP_API_PLUGIN_MAC_CFM
/*-------------------------------Macintosh Section----------------------------*/

	char name[64];
	NMErr err;
	char *source_dir= "OpenPlay Modules";
	NMBoolean theTargetIsFolder, theWasAliased;
	short diskNum;
	long folderNum;

	*search_start= gOp_globals.file_spec;
	strcpy(name, source_dir);
	c2pstr(name);

	//gmp 4/25/04: Use the same disk and dir as the shared library itself, not 0, 0
	//					(whatever the app happens to consider the current folder)

	diskNum = search_start->vRefNum;
	folderNum = search_start->parID;

	err = FSMakeFSSpec(diskNum, folderNum, /*0, 0,*/ (StringPtr) name, (FSSpec *) search_start);
	DEBUG_PRINTonERR("Err %d on FSMakeFSSpec", err);

	err = ResolveAliasFile((FSSpec *) search_start, true, &theTargetIsFolder, &theWasAliased);
	DEBUG_PRINTonERR("Err %d on ResolveAliasFile", err);

#elif defined(OP_API_PLUGIN_WINDOWS)
/*--------------------------------Windows Section-----------------------------*/

	char subfolder_name[100];
	char *dest;
	NMSInt16 length;

	GetModuleFileName(gOp_globals.dll_instance, search_start->name, MAX_PATHNAME_LENGTH);
	dest= strrchr(search_start->name, '\\');
	op_assert(dest);
	*dest= 0;
	strcat(search_start->name, "\\");
	
	op_assert(gOp_globals.dll_instance);
	
	length= LoadString(gOp_globals.dll_instance,
                           ((strFILENAMES-128)*MAX_STRINGS_PER_GROUP)+moduleFolder,
		           subfolder_name, sizeof(subfolder_name));
	
	strcat(search_start->name, subfolder_name);

#elif defined(OP_API_PLUGIN_MACHO)
/*----------------------------------Posix Section-----------------------------*/

	// for the project-builder build, we load netmodules from bundles contained as
	// resources within the openplay framework.  for all other posix builds,
	// we load plugins from disk based on an absolute location specified
	// by an environment variable (or a default location if not set)
	
		search_start->bundle = CFBundleGetBundleWithIdentifier(CFSTR("com.apple.openplay"));
		op_assert(search_start->bundle);
		
#elif defined(OP_API_PLUGIN_POSIX) || defined(OP_API_PLUGIN_POSIX_DARWIN)

  char *env_ptr;

  env_ptr = getenv("OPENPLAY_LIB");
 
  if (env_ptr)
		DEBUG_PRINT("env \"OPENPLAY_LIB\" == %s\n",env_ptr);
	else
		DEBUG_PRINT("environment variable \"OPENPLAY_LIB\" not set. Using default: %s\n",d_OPENPLAY_LIB_LOCATION);
  if (env_ptr)
  {
    if (strlen(env_ptr) < PATH_MAX-1)
      strcpy(search_start->name, env_ptr);
    else
      strcpy(search_start->name, d_OPENPLAY_LIB_LOCATION);
  }
  else
    strcpy(search_start->name, d_OPENPLAY_LIB_LOCATION);

/*----------------------------------------------------------------------------*/

#else
	#error "Porting Error - Unknown platform"
#endif

} /* build_find_protocol_search_start */

//----------------------------------------------------------------------------------------
// valid_protocol_file
//----------------------------------------------------------------------------------------

/* This should get more robust.. */
static NMBoolean valid_protocol_file(
	FileDesc *file,
	void *data)
{
  NMBoolean valid_protocol_module = false;	

#ifdef OP_API_PLUGIN_MAC_CFM
/*-------------------------------Macintosh Section----------------------------*/
UNUSED_PARAMETER(file);

  CInfoPBRec *pb = (CInfoPBRec *) data;

  if(pb->hFileInfo.ioFlFndrInfo.fdCreator == NET_MODULE_CREATOR)
  {
    valid_protocol_module = true;
  }

#elif defined(OP_API_PLUGIN_WINDOWS)
/*--------------------------------Windows Section-----------------------------*/

  WIN32_FIND_DATA *pb= (WIN32_FIND_DATA *) data;
  char *last_period;

  UNUSED_PARAMETER(file);

  last_period = strrchr(pb->cFileName, '.');
  if(last_period)
  {
    last_period++;
    valid_protocol_module = true;
  }

#elif defined(OP_API_PLUGIN_MACHO)

	valid_protocol_module = true;

#elif defined(OP_API_PLUGIN_POSIX) || defined(OP_API_PLUGIN_POSIX_DARWIN)
/*----------------------------------Posix Section-----------------------------*/
	
  NMSInt32 namelen;
  NMSInt32 dirlen;

  if (!file || !data || !file->name)
    return(valid_protocol_module);
	
  namelen = strlen(file->name);
  dirlen = strlen((char*)data);

  /* minimum name is "x.so" */
  if (namelen >= 4)
  { 
    /* check that extension is ".so" */   
    if ( strcmp(&(file->name[namelen-3]), ".so") == 0 )
    {
      FileDesc tempfile;

      /* fix up name to include directory path 
       * this is needed when get_module_name_and_type() is called so the
       * fully qualified filename is used to load the shared library
        */

      /* copy in passed directory name */
      strcpy(tempfile.name, (char*)data);

      /* Check if directory name ended with "/", add if needed */
      if (tempfile.name[dirlen-1] != '/')
        strcat(tempfile.name, "/");

      /* copy in filename */
      strcat(tempfile.name, file->name);

      /* replace file with tempfile */
      strcpy(file->name, tempfile.name);

      valid_protocol_module = true;
    }
  }

#else
/*----------------------------------------------------------------------------*/

	#error "Porting Error - Unknown platform"

#endif

  return valid_protocol_module;

} /* valid_protocol_file */

//----------------------------------------------------------------------------------------
// get_module_name_and_type
//----------------------------------------------------------------------------------------

static NMBoolean get_module_name_and_type(
	FileDesc *file,
	NMType *type,
	char *name)
{
  NMErr          err;
  ConnectionRef  conn_id;
  NMBoolean      valid = false;

  err= bind_to_library(file, &conn_id);
  if(!err)
  {
    NMGetModuleInfoPtr NMGetModuleInfo = NULL;
 		
    NMGetModuleInfo = (NMGetModuleInfoPtr) load_proc(conn_id, kNMGetModuleInfo);

    if(NMGetModuleInfo)
    {
      NMModuleInfo info;

      info.size = sizeof(info);

      NMGetModuleInfo(&info);

	
		//DEBUG_PRINT("loaded module: \"%s\"\n",info.name);
		//DEBUG_PRINT("size: %d\n",info.size);
		//DEBUG_PRINT("copyright: \"%s\"\n",info.copyright);
		//DEBUG_PRINT("type: %c%c%c%c\n",(info.type >> 24),(info.type >> 16) & 0xFF, (info.type >> 8) & 0xFF, info.type & 0xFF); 
  		//DEBUG_PRINT("maxPacketSize: %d\n",info.maxPacketSize);
  		//DEBUG_PRINT("flags: %d\n",info.flags);
  		
      *type = info.type;
      strcpy(name, info.name);			
      valid = true;
    }

    free_library(conn_id);
  }
	
  return valid;
}

//----------------------------------------------------------------------------------------
// find_protocol_callback
//----------------------------------------------------------------------------------------

static NMBoolean find_protocol_callback(
	FileDesc *file,
	void *data)
{
  if(valid_protocol_file(file, data))
  {
      module_data new_module;

      /* Fill in what we got.. */
      new_module.module_spec= *file;
      
      //keep the temporary bundle from being disposed 

#ifdef OP_API_PLUGIN_MACHO
		CFRetain(new_module.module_spec.bundle);
      #endif
      
      new_module.module.version= CURRENT_OPENPLAY_VERSION;

      /* Get the type and the name from the fragment. */
      if(get_module_name_and_type(file, &new_module.module.type, new_module.module.name))
      {
		  module_data *old_module_list= gOp_globals.modules;

		  gOp_globals.modules= (module_data *) new_pointer((gOp_globals.module_count+1)*sizeof(module_data));
		  if(gOp_globals.modules)
		  {
		      if(old_module_list)
		      {
				  machine_copy_data(old_module_list, gOp_globals.modules, gOp_globals.module_count*sizeof(module_data));
				  dispose_pointer(old_module_list);
		      }
		      machine_copy_data(&new_module, &gOp_globals.modules[gOp_globals.module_count], sizeof(module_data));
		      gOp_globals.module_count++;
		  }
      }
  }
  return true;
}

//----------------------------------------------------------------------------------------
// find_protocols
//----------------------------------------------------------------------------------------

static void find_protocols(
	void)
{
	find_file_pb pb;
	NMErr err;

	/* Clear out whatever was there before */
	if(gOp_globals.module_count)
	{
		op_assert(gOp_globals.modules);
		dispose_pointer(gOp_globals.modules);
		gOp_globals.modules= NULL;
		gOp_globals.module_count= 0;
	}
	
	/* Setup for searching and finding */
	machine_mem_zero(&pb, sizeof(pb));

	pb.version = FIND_FILE_VERSION;
	pb.flags = _ff_callback_with_catinfo;
	pb.search_type = _callback_only;

	build_find_protocol_search_start(&pb.start_search_from);

   pb.type_to_find = d_LIBRARY_TYPE;
	pb.buffer = NULL;
	pb.max = MAXIMUM_NETMODULES;
	pb.count = 0;
	pb.callback = find_protocol_callback;
	pb.user_data = NULL;
	
	err = find_files(&pb);
}

//----------------------------------------------------------------------------------------
// refresh_protocols
//----------------------------------------------------------------------------------------

/* called from init (to avoid fragmentation) */
void refresh_protocols(
	void)
{
/*	if(!gOp_globals.module_count || 
           machine_tick_count()-gOp_globals.ticks_at_last_protocol_search>30*MACHINE_TICKS_PER_SECOND) 
 */
	{
		/* find the protocols. */
		find_protocols();
		
		/* Set our cache date... */
		gOp_globals.ticks_at_last_protocol_search= machine_tick_count();
	}
}
