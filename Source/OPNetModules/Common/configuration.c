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

//	------------------------------	Includes

#include "OPUtils.h"
#include "configuration.h"

//	------------------------------	Private Definitions
//	------------------------------	Private Types
//	------------------------------	Private Variables
//	------------------------------	Private Functions 

static NMSInt32 atol_tab(const char * s);
static NMSInt32 strlen_tab(const char * s);
static NMBoolean ascii_to_hex(const char *inAscii, NMSInt32 inLen, char *ioData);
static NMBoolean hex_to_ascii(const char *inData, NMSInt32 inLen, char *ioAscii);

//	------------------------------	Public Variables

const char *kConfigModuleType = "type";
const char *kConfigModuleVersion = "version";
const char *kConfigGameID = "gameID";
const char *kConfigGameName = "gameName";
const char *kConfigEndpointMode = "mode";
const char *kConfigNetSprocketMode = "netSprocket";
const char *kConfigCustomData = "enumData";

//	--------------------	get_token

/*	get_token	get data of the token_type with the token_label from the configuration string s.
 *
 *	returns true if the data was successfully retrieved.
 *	returns false if there was no data of that type, invalid data, or too much data for the buffer.
 *	io_length is size of buffer(in), and length of data retrieved(out).
 */

//----------------------------------------------------------------------------------------
// get_token
//----------------------------------------------------------------------------------------

NMBoolean
get_token(
	const char	*s,
	const char	*token_label,
	NMSInt16	token_type,
	void		*token_data,
	NMSInt32	*io_length)
{
const char	*p = s;
	
	while (*p)
	{
	const char * t_label = token_label;
	NMBoolean fail_match = false;
	NMBoolean token_match = false;

                /* loop to check the current label for a match */
		while (*p && !(fail_match || token_match))
		{
			if (*p++ == *t_label++)
			{
				if (*p == LABEL_TERM && *t_label == 0)
				{
					++p;
					token_match = true;
				}
			}
			else
			{
				fail_match = true;
			}
		}
		
		if (token_match)	/* convert/copy the appropriate data out (p now points to data) */
		{
		NMSInt32	data_len = strlen_tab(p);
				
			switch (token_type)
			{
				case STRING_DATA:
					if (*io_length < data_len + 1)
					  return false;	/* terminated string is too long for buffer */
						
					machine_copy_data(p, token_data, data_len);
					*((char *)token_data + data_len) = 0;
					*io_length = data_len + 1;
					break;

				case LONG_DATA:
				{
				NMSInt32	long_data;

					if (*io_length < sizeof(NMSInt32))
					  return false;	/* a NMSInt32 won't fit in the buffer provided */
						
					long_data = atol_tab(p);
					machine_copy_data(&long_data, token_data, sizeof(NMSInt32));
					*io_length = sizeof(NMSInt32);
				}
				break;

				case BOOLEAN_DATA:
				{
				NMBoolean	bool_data;
					
					if (*io_length < sizeof(NMBoolean))
					  return false;	/* a NMSInt32 won't fit in the buffer provided */
						
					bool_data = (*p == 't');
					machine_copy_data(&bool_data, token_data, sizeof(NMBoolean));
					*io_length = sizeof(NMBoolean);
				}
				break;

				case BINARY_DATA:
				{
					if (*io_length < data_len / 2)
						return false;
					
					ascii_to_hex(p, data_len, (char *)token_data);
					*io_length = data_len / 2;
				}
				break;

				default:
					return false;
					break;
			}

			return true;
		}
		else	/* next token possibility */
		{
			while (*p && *p++ != TOKEN_SEPARATOR)
				;
		}
	}

	return false;
}

//----------------------------------------------------------------------------------------
// put_token
//----------------------------------------------------------------------------------------
/*	put_token	put data of the token_type with the token_label into the configuration string s
 *
 *	returns true if the data was successfully written
 *	returns false if the config string was not NMSInt32 enough to hold the data, or if the token_type was invalid
 */

NMBoolean
put_token(
	char		*s,
	NMSInt32		s_max_length,
	const char	*token_label,
	NMSInt16	token_type,
	void		*token_data,
	NMSInt32		data_len)
{
char		*p = s;
NMSInt32	new_data_len;
char		temp_string[MAX_BIN_DATA_LEN + 1];
const char	*put_string;
	
        /* First prepare the data to be put into the config string. */
	switch (token_type)
	{
		case STRING_DATA:
			put_string = (const char *)token_data;
			break;

		case LONG_DATA:
			sprintf(temp_string, "%ld", *(NMSInt32 *)token_data);
			put_string = (const char *)temp_string;
			break;

		case BOOLEAN_DATA:
			if (*(char *)token_data)
				sprintf(temp_string, "true");
			else
				sprintf(temp_string, "false");
			put_string = (const char *)temp_string;
			break;

		case BINARY_DATA:
		{
			if (data_len > MAX_BIN_DATA_LEN)
			  return false;	/* the data is more than we can handle */

			hex_to_ascii((const char *)token_data, data_len, temp_string);
			put_string = temp_string;
		}
		break;

		default:
			return false;
			break;
	}

	new_data_len = strlen(put_string);	/* Add 2 for ':' and '\t' */

	while (*p)
	{
	const char	*t_label = token_label;
	NMBoolean	token_match = false;
	NMBoolean	fail_match = false;
		
                /* loop to check the current label for a match */
		while (*p && !(fail_match || token_match))
		{
			if (*p++ == *t_label++)
			{
				if (*p == LABEL_TERM)
				{
					++p;
					token_match = true;
				}
			}
			else
			{
				fail_match = true;
			}
		}
		
		if (token_match)	/* put the data into the existing spot, if possible */
		{
		  NMSInt32	old_data_len = strlen_tab(p);	/* length of old token data */
			
			if (new_data_len > old_data_len
			&& 	(NMSInt32)strlen(s) - old_data_len + new_data_len > s_max_length)
			  return false;	/* string would be too NMSInt32 for buffer */
	
			if (new_data_len <= old_data_len)	/* copy the new data, then scooch the end in. */
			{
				machine_copy_data(put_string, p, new_data_len);
				machine_move_data(p + old_data_len, p + new_data_len, strlen(p + old_data_len) + 1);	
			}
			else	/* scooch the end out, then copy the new data */
			{
				machine_move_data( p + old_data_len, p + new_data_len,strlen(p + old_data_len) + 1);
				machine_copy_data(put_string, p, new_data_len);
			}

			return true;
		}
		else	/* move to next token possibility */
		{
			while (*p && *p++ != TOKEN_SEPARATOR)
				;
		}
	}

	/* append the token and data to the end of the config string */
	if ((NMSInt32)strlen(s) + new_data_len > s_max_length)
	{
	  return false;	/* string would be too NMSInt32 for buffer */
	}
	else
	{
          if (strlen(s))	/* don't put a token at the beginning of the string */
            *p++ = TOKEN_SEPARATOR;

		machine_copy_data(token_label, p, strlen(token_label));
		p += strlen(token_label);
		
		*p++ = LABEL_TERM;

		machine_copy_data(put_string, p, new_data_len + 1);
	}

	return true;
}

/*	--------------------	atol_tab */

/*	accepts tab terminated string, or null terminated string */
static NMSInt32
atol_tab(const char * s)
{
const char	*p;
NMSInt32		value;

	value = 0;

	for (p = s; *p && *p != TOKEN_SEPARATOR; ++p)
	{
		if(*p >= '0' && *p <= '9')
		{
			value *= 10;
			value += *p-'0';
		}
	}
	
	return value;
}

//----------------------------------------------------------------------------------------
// strlen_tab
//----------------------------------------------------------------------------------------

/*	accepts tab terminated string, or null terminated string */
static NMSInt32
strlen_tab(const char * s)
{
const char	*p;
NMSInt32	length = 0;
	
	for (p = s; *p && *p != TOKEN_SEPARATOR ; ++p)
	{
		++length;
	}

	return length;
}

//----------------------------------------------------------------------------------------
// ascii_to_hex
//----------------------------------------------------------------------------------------

static NMBoolean
ascii_to_hex(const char *inAscii, NMSInt32 inLen, char *ioData)
{
const char	*character = inAscii;
char		theByte = 0;
char		*theData = ioData;
NMSInt32			i;

        /* return if the data pointer isn't big enough or there are an odd number of characters */
	if (inLen & 0x0001)
	{
		DEBUG_PRINT("There are an odd number of characters in the data string (%p)", inAscii);
		return false;
	}	
	
	character = inAscii;
	
	for (i = 0; i < inLen; i++)
	{
                /* read the hi nibble */
		if (*character >= '0' && *character <= '9')
			theByte = *character - '0';
		else if (*character >= 'A' && *character <= 'F')
			theByte =  *character - 'A' + 0x0A;
		else
			DEBUG_PRINT("Got a bogus character '%c'", *character);

		/* shift it */
		theByte = theByte << 4;

		character++;
		
		/* Add in the lo nibble */
		if (*character >= '0' && *character <= '9')
			theByte |= *character - '0';
		else if (*character >= 'A' && *character <= 'F')
			theByte |=  *character - 'A' + 0x0A;
		else
			DEBUG_PRINT("Got a bogus character '%c'", *character);
		
		*theData++ = theByte;

		character++;
	};
	
	return true;
}

//----------------------------------------------------------------------------------------
// hex_to_ascii
//----------------------------------------------------------------------------------------

/*	Note!! we need 2 bytes in the string for every byte of data! */
static NMBoolean
hex_to_ascii(const char *inData, NMSInt32 inLen, char *ioAscii)
{
	char	lo, hi;
	char	*	string = ioAscii;
	NMSInt32	i;

	if (inLen > MAX_BIN_DATA_LEN)
	{
		DEBUG_PRINT("Asked to stuff %d bytes of data.  We can only handle up to 256.", inLen);
		return false;
	}
	
	for (i = 0; i < inLen; i++)
	{
		hi = (inData[i] & 0xF0) >> 4;
		lo = inData[i] & 0x0F;

		if (hi > 9)
			hi = hi - 0x0A + 'A';
		else
			hi = hi + '0';
			
		if (lo > 9)
			lo = lo  - 0x0A + 'A';
		else
			lo = lo + '0';
		
		*string++ = hi;
		*string++ = lo;	
	}
	
	*string = 0;
	
	return true;
}
