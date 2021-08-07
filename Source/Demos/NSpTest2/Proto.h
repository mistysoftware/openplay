/*
	File:		Proto.h

	Contains:	xxx put contents here xxx

	Version:	xxx put version here xxx

	Copyright:	© 1998 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Chris De Salvo

		Other Contact:		Steve Bollinger

		Technology:			Apple Game Sprockets

	Writers:

		(cjd)	Chris De Salvo

	Change History (most recent first):

	   <SP2>	 9/25/98	cjd		Removing references to stuff that was in fooutils.c
*/

/*************************************************************************************
#
#		Proto.h
#
#		This file contains the prototypes for the apps procs and funcs
#
#		Author(s): 	Michael Marinkovich
#					Apple Developer Technical Support
#					marink@apple.com
#
#		Modification History: 
#
#			2/10/96		MWM 	Initial coding					 
#			3/2/02		ECF		Carbon/OpenPlay port				 
#
#		Copyright © 1992-96 Apple Computer, Inc., All Rights Reserved
#
#
#		You may incorporate this sample code into your applications without
#		restriction, though the sample code has been provided "AS IS" and the
#		responsibility for its operation is 100% yours.  However, what you are
#		not permitted to do is to redistribute the source as "DSC Sample Code"
#		after having made changes. If you're going to re-distribute the source,
#		we require that you make it clear in the source that the code was
#		descended from Apple Sample Code, but that you've made changes.
#
*************************************************************************************/

#if (!OP_PLATFORM_MAC_MACHO)
	#include <AppleEvents.h>
#endif

#include "app.h"

#ifdef __cplusplus
extern "C" {
#endif
//----------------------------------------------------------------------
//
//	Aevt
//
//----------------------------------------------------------------------

OSErr 			AEInit(void);
OSErr			AERemove(void);

#if (OP_PLATFORM_MAC_CARBON_FLAG)
	pascal OSErr	DoAEOpenApp(const AppleEvent *event,AppleEvent *reply,long refCon);
	pascal OSErr	DoAEQuitApp(const AppleEvent *event,AppleEvent *reply,long refCon);
	pascal OSErr	DoAEOpenDoc(const AppleEvent *event,AppleEvent *reply,long refCon);
	pascal OSErr	DoAEPrintDoc(const AppleEvent *event,AppleEvent *reply,long refCon);
#else
	pascal OSErr	DoAEOpenApp(AppleEvent *event,AppleEvent reply,long refCon);
	pascal OSErr	DoAEQuitApp(AppleEvent *event,AppleEvent reply,long refCon);
	pascal OSErr	DoAEOpenDoc(AppleEvent *event,AppleEvent reply,long refCon);
	pascal OSErr	DoAEPrintDoc(AppleEvent *event,AppleEvent reply,long refCon);
#endif //OP_PLATFORM_MAC_CARBON_FLAG
OSErr 			GotAEParams(AppleEvent *appleEvent);


//----------------------------------------------------------------------
//
//	Initialize
//
//----------------------------------------------------------------------

OSErr			Initialize(void);
void			ToolBoxInit(void);
void 			CheckEnvironment(void);
OSErr			InitApp(void);
OSErr			MenuSetup(void);


//----------------------------------------------------------------------
//
//	Main
//
//----------------------------------------------------------------------


void 			HandleError(short errNo,Boolean fatal);
void 			HandleAlert(short alertID);


//----------------------------------------------------------------------
//
//	Events
//
//----------------------------------------------------------------------

void			EventLoop(void);
short 			MyGetSleep(void);
void 			CustomWindowEvent(short eventType,WindowRef window,void *refCon);
void 			DoEvent(EventRecord *event);
void 			DoIdle(WindowRef window, void *refCon);
void 			HandleMouseDown(EventRecord *event);
void 			HandleMenuChoice(WindowRef window, void *refCon);
void 			AdjustMainMenus(void);
void			HandleContentClick(WindowRef window, void *refCon);
void 			HandleZoomClick(WindowRef window, void *refCon);
void 			HandleGrow(WindowRef window, void *refCon);
void 			UpdateWindow(WindowRef window);
void 			DoActivate(WindowRef window, void *refCon);


//----------------------------------------------------------------------
//
//	Windows
//
//----------------------------------------------------------------------
/* LR
WindowPtr 		CreateWindow(short resID, void *wStorage, Rect *bounds, Str255 title,
							Boolean visible, short procID,short kind, WindowRef behind,
							Boolean goAwayFlag,long refCon);
OSErr 			RemoveWindow(WindowRef window);
void 			DisposeWindowStructure(DocHnd doc);
void 			NewWindowTitle(WindowRef window, Str255 str);
OSErr 			InitWindowProcs(WindowRef window, short windKind);
void			DrawWindow( WindowRef window, void *refCon );
void 			DrawAboutWindow( WindowRef window, void *refCon );
void 			DoResizeWindow (WindowRef window);
short 			GetWindKind(WindowRef window);
Boolean			GetIsAppWindow(WindowRef window);
Boolean 		GetIsAboutWindow( WindowRef window );
*/
#ifdef __cplusplus
}
#endif
