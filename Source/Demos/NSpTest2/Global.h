/***********************************************************************
#
#		Global.h
#
#		This file contains the global definitions
#
#		Author: Michael Marinkovich
#				Apple Developer Technical Support
#
#
#		Modification History: 
#
#			6/4/95	MWM 	Initial coding					 
#			3/2/02		ECF		Carbon/OpenPlay port				 
#
#
#		Copyright © 1992-94 Apple Computer, Inc., All Rights Reserved
#
#
***********************************************************************/

#if (!OP_PLATFORM_MAC_MACHO)
	#include <Drag.h>
#endif

Boolean 		gHasDMTwo;						// is DM 2.0 available?
Boolean 		gInBackground = false;			// are we in the background?
Boolean			gDone = false;					// app is done flag		
Boolean			gHasDrag;						// we have Drag Manager?
Boolean			gHasAbout;						// do we have an about box showing?
short			gWindCount;						// window counter for new windows

