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

#include "OPUtils.h"
#include "String_Utils.h"


//----------------------------------------------------------------------------------------
// doCopyC2PStrMax
//----------------------------------------------------------------------------------------

void doCopyC2PStrMax(const char *sourceString, NMUInt8 *destString, NMSInt32 maxLengthToCopy)
{
	NMSInt32		charIndex;
	NMSInt32		strLength;
	
	strLength = (NMSInt32) strlen(sourceString);
			
	if (strLength > maxLengthToCopy)
		strLength = maxLengthToCopy;
		
	destString[0] = strLength;
	for (charIndex = 0; charIndex <= strLength; charIndex++)
		destString[charIndex + 1] = sourceString[charIndex];

}

//----------------------------------------------------------------------------------------
// doCopyC2PStr
//----------------------------------------------------------------------------------------

void doCopyC2PStr(const char *sourceString, NMUInt8 *destString)
{
	NMSInt32		charIndex;
	NMSInt32		strLength;
	
	strLength = (NMSInt32) strlen(sourceString);
	
	destString[0] = strLength;		
	for (charIndex = 0; charIndex <= strLength; charIndex++)
		destString[charIndex + 1] = sourceString[charIndex];

}

//----------------------------------------------------------------------------------------
// doCopyPStrMax
//----------------------------------------------------------------------------------------

void doCopyPStrMax(const NMUInt8 *inSourceStr, NMUInt8 *outDestStr, NMSInt32 inMaxLen)
{
NMSInt32	dataLen = (inSourceStr[0] + 1 < inMaxLen) ? inSourceStr[0] + 1 : inMaxLen;
	
	machine_copy_data(inSourceStr, outDestStr, dataLen);
	outDestStr[0] = dataLen - 1;
}

//----------------------------------------------------------------------------------------
// doCopyPStr
//----------------------------------------------------------------------------------------

void doCopyPStr(const NMUInt8 *inSourceStr, NMUInt8 *outDestStr)
{
	machine_copy_data(inSourceStr, outDestStr, inSourceStr[0] + 1);
}

//----------------------------------------------------------------------------------------
// doCopyP2CStr
//----------------------------------------------------------------------------------------

void	doCopyP2CStr(const NMUInt8 *inSourceStr, char *outDestStr)
{
	machine_copy_data(&inSourceStr[1], outDestStr, inSourceStr[0]);
	outDestStr[inSourceStr[0]] = 0;
}

//----------------------------------------------------------------------------------------
// doConcatPStr
//----------------------------------------------------------------------------------------

void doConcatPStr(NMUInt8 *stringOne, NMUInt8 *stringTwo)
{
	NMSInt32		charIndex;
	NMSInt32		strLength1;
	NMSInt32		strLength2;
	
	strLength1 = (NMSInt32) stringOne[0];
	strLength2 = (NMSInt32) stringTwo[0];
	
	for (charIndex = 1; charIndex <= strLength2; charIndex++)
	{
		stringOne[strLength1 + charIndex] = stringTwo[charIndex];
	}

	stringOne[0] = (NMUInt8)(strLength1 + strLength2);
	
//	PLstrcat(stringOne, stringTwo);

}

//----------------------------------------------------------------------------------------
// doComparePStr
//----------------------------------------------------------------------------------------

NMBoolean doComparePStr(const NMUInt8 *stringOne, const NMUInt8 *stringTwo)
{
	NMBoolean		stringsEqual = true;
	NMSInt32		charIndex;
	NMSInt32		strLength1;
	NMSInt32		strLength2;
	
	strLength1 = (NMSInt32) stringOne[0];
	strLength2 = (NMSInt32) stringTwo[0];
	
	if (strLength1 != strLength2)
		stringsEqual = false;
	
	charIndex = 1;
	
	while ((stringsEqual) && (charIndex <= strLength1))
	{
		if (stringOne[charIndex] != stringTwo[charIndex])
			stringsEqual = false;
		else
			charIndex++;
	}
	
	return stringsEqual;
}

//----------------------------------------------------------------------------------------
// doGetConfigSubString
//----------------------------------------------------------------------------------------
//obtain the value of an item within a config string
NMBoolean 	doGetConfigSubString(char *configStr, char *itemName, char *buffer, long bufferLen)
{
	char *value = strstr(configStr,itemName);
	if (value == NULL){
		return false;
	}
	value += strlen(itemName) + 1;
	
	while ((bufferLen > 1) && (*value != '\n') && (*value != '\r') && (*value != '\0') && (*value != '\t')){
		*buffer++ = *value++;
		bufferLen--;
	}
	if (bufferLen > 0)
		*buffer++ = 0;
		
	return true;
}

//----------------------------------------------------------------------------------------
// doSetConfigSubString
//----------------------------------------------------------------------------------------

//set the value of an item within a config string, or append it if it does not exist

void		doSetConfigSubString(char *configStr, char *itemName, char *itemValue)
{
	char buffer[256];
	char *nameStart;
	
	//copy all to buffer first
	strcpy(buffer,configStr);
	
	nameStart = strstr(buffer, itemName);

	//if its not in the string, just append it at the end
	if (nameStart == NULL){
		sprintf(configStr,"%s\t%s=%s",buffer,itemName,itemValue);
		return;
	}	

	//replace first char of namestart with a term char so the first part is its own string
	*nameStart = 0;
	nameStart++;
	
	//now move past the current contents till we hit a space, tab, or end
	while ((*nameStart != ' ') && (*nameStart != 0) && (*nameStart != 0x09)){
		nameStart++;
	}	
	sprintf(configStr,"%s%s=%s%s",buffer,itemName,itemValue,nameStart);

}
