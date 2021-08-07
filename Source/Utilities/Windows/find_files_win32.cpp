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

#include "OpenPlay.h"
#include "OPUtils.h"
#include "portable_files.h"
#include "find_files.h"

/* --------- local prototypes */
static int 			alphabetical_names(const FileDesc *a, const FileDesc *b);
static NMErr 	enumerate_files(char *type_to_find, find_file_pb *param_block);
static void 		build_extension_from_type_to_find(long filetype, char *buffer);
	
//----------------------------------------------------------------------------------------
// find_files
//----------------------------------------------------------------------------------------

NMErr find_files(
	 find_file_pb *param_block)
{
	NMErr err = 0;


	op_assert(param_block);

	/* If we need to create the destination buffer */
	if(param_block->flags & _ff_create_buffer) 
	{
	    op_assert(param_block->search_type ==_fill_buffer);
	    param_block->buffer = (FileDesc *) new_pointer(sizeof(FileDesc)*param_block->max);
	}

	/* Assert that we got it. */
	op_assert(param_block->search_type == _callback_only || param_block->buffer);
	op_assert(param_block->version == 0);

	/* Set the variables */
	param_block->count = 0;

	{
    	char type_to_find[5];            // .XXX0
    	char original_path[_MAX_PATH];

	    build_extension_from_type_to_find(param_block->type_to_find, type_to_find);

	    GetCurrentDirectory(sizeof(original_path), original_path);
	    
	    SetCurrentDirectory(param_block->start_search_from.name);
    
		{
			char current[256];
	   		GetCurrentDirectory(sizeof(current), current);
		}
    
   		SetLastError(NO_ERROR);

		err = enumerate_files(type_to_find, param_block);

	    SetCurrentDirectory(original_path);
	    SetLastError(NO_ERROR);
	}

	/* Alphabetical */
	if(param_block->flags & _ff_alphabetical)
	{
		op_assert(param_block->search_type==_fill_buffer);
		qsort(param_block->buffer, param_block->count, 
			sizeof(FileDesc), (int (*)(const void *, const void *))alphabetical_names);
	}

	/* If we created the buffer, make it the exact right size */
	if(param_block->flags & _ff_create_buffer)
	{
		op_assert(param_block->search_type==_fill_buffer);
		param_block->buffer= (FileDesc *)realloc(param_block->buffer, sizeof(FileDesc)*param_block->count);
	}
	
	return err;
}
	
//----------------------------------------------------------------------------------------
// equal_filedescs
//----------------------------------------------------------------------------------------

NMBoolean equal_filedescs(
	FileDesc *a, 
	FileDesc *b)
{
	NMBoolean	equal = false;
	NMUInt32	length = strlen(a->name);
	
	if(length == strlen(b->name) && strcmp(a->name, b->name) == 0)
	{
		equal = true;
	}
	
	return equal;
}

/* --------- Local Code --------- */
	
//----------------------------------------------------------------------------------------
// build_extension_from_type_to_find
//----------------------------------------------------------------------------------------

static void build_extension_from_type_to_find(
	long filetype,
	char *buffer)
{
	NMSInt16 index= 0;
	char *lowercase;
	
	index= 0;
	buffer[index++]= '.';
	buffer[index++]= (char) (filetype >> 24);
	buffer[index++]= (char) (filetype >> 16);
	buffer[index++]= (char) (filetype >> 8);
  //	buffer[index++]= (char) filetype;
	buffer[index++]= 0;

  /* lowercase it (i don't have strlwr for some reason */
	for(lowercase = buffer; *lowercase; ++lowercase) 
		*lowercase = tolower(*lowercase);
	
	return;
}
	
//----------------------------------------------------------------------------------------
// enumerate_files
//----------------------------------------------------------------------------------------

static NMErr enumerate_files(
				 char *type_to_find,
				 	 find_file_pb *param_block) /* Because it is recursive.. */
{
  static WIN32_FIND_DATA   pb; /* static to prevent stack overflow.. */
  static FileDesc          temp_file;
  NMErr                err;
  HANDLE                   hFind;

	
  /* Find the first file. */
	hFind = FindFirstFile("*.*", &pb);

	if(hFind != INVALID_HANDLE_VALUE)
    {
		do {
	/* Ignore these two. */
			if(strcmp(pb.cFileName, ".") !=0 && strcmp(pb.cFileName, "..") != 0)
			{
				if((pb.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && (param_block->flags & _ff_recurse))
				{
				/* Recurse, if you really want to... */
					SetCurrentDirectory(pb.cFileName);
					err = enumerate_files(type_to_find, param_block);
					SetCurrentDirectory("..");
					SetLastError(NO_ERROR);
				} 
				else 
				{
					char *extension, *lowercase;
					
					/* Get the extension. */
					extension = strrchr(pb.cFileName, '.');

					for(lowercase = extension; extension && *lowercase; ++lowercase) 
			            *lowercase = tolower(*lowercase);

					//{
					//	char temporary[256];
					//	GetCurrentDirectory(sizeof(temporary), temporary);
					//	DEBUG_PRINT("Testing file: %s (Directory: %s)", pb.cFileName, temporary);
					//}

					/* Add.. */
					if(param_block->type_to_find == WILDCARD_TYPE || 
		   				(extension && (strcmp(extension, type_to_find) == 0)))
		 			{
		    			/* Build the filename. */
		    			GetCurrentDirectory(sizeof(temp_file.name), temp_file.name);

		    			if(temp_file.name[strlen(temp_file.name)-1] != '\\')
		     			{
							strcat(temp_file.name, "\\");
		      			}

		   				strcat(temp_file.name, pb.cFileName);
		    			//DEBUG_PRINT("%s matches criteria (%s is full)", pb.cFileName, temp_file.name);
						
		    			/* Only add if there isn't a callback or it returns true */
		    			switch(param_block->search_type)
		      			{
		      				case _fill_buffer:
								if(!param_block->callback || param_block->callback(&temp_file, param_block->user_data))
		  						{
		   						 /* Copy it in.. */
		    						machine_copy_data(&param_block->buffer[param_block->count++], &temp_file,
			   								sizeof(FileDesc));
		  						}
								break;
								
							case _callback_only:
								op_assert(param_block->callback);
								if(param_block->flags & _ff_callback_with_catinfo)
								{
			    					//DEBUG_PRINT("callback with catinfo");
			   						param_block->callback(&temp_file, &pb);
			  					} else {
			    					//DEBUG_PRINT("callback with userdata");
			    					param_block->callback(&temp_file, param_block->user_data);
			  					}
								break;
		     				default:
								op_halt();
								break;
		      			}
		    			SetLastError(NO_ERROR);
		  			}
	      		}
	  		}
      } while (FindNextFile(hFind, &pb));
		
      FindClose(hFind);
    }

	return 0;
}
	
//----------------------------------------------------------------------------------------
// alphabetical_names
//----------------------------------------------------------------------------------------

static int alphabetical_names(
	const FileDesc *a, 
	const FileDesc *b)
{
	return strcmp(a->name, b->name);
}

