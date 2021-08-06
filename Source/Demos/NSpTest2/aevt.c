/*************************************************************************************
#
#		aevt.c
#
#		Apple events handler.
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
#include "OpenPlay.h"

#if !(OP_PLATFORM_MAC_MACHO)
	#include <AppleEvents.h>
	#include <MacWindows.h>
#endif

#include "NetStuff.h"

#include "App.h"
#include "Proto.h"


extern Boolean			gDone;

AEEventHandlerUPP		gAEOpenUPP;
AEEventHandlerUPP		gAEQuitUPP;
AEEventHandlerUPP		gAEOpenDocUPP;
AEEventHandlerUPP		gAEPrintDocUPP;


//----------------------------------------------------------------------
//
//	AEInit - initialize all the core apple events
//
//
//----------------------------------------------------------------------

OSErr AEInit(void)
{	
	OSErr		err = noErr;

	#if (OP_PLATFORM_MAC_CARBON_FLAG)
		gAEOpenUPP 		= NewAEEventHandlerUPP((AEEventHandlerProcPtr)DoAEOpenApp);
		gAEQuitUPP 		= NewAEEventHandlerUPP((AEEventHandlerProcPtr)DoAEQuitApp);
		gAEOpenDocUPP	= NewAEEventHandlerUPP((AEEventHandlerProcPtr)DoAEOpenDoc);
		gAEPrintDocUPP	= NewAEEventHandlerUPP((AEEventHandlerProcPtr)DoAEPrintDoc);
	#else
		gAEOpenUPP 		= NewAEEventHandlerProc(DoAEOpenApp);
		gAEQuitUPP 		= NewAEEventHandlerProc(DoAEQuitApp);
		gAEOpenDocUPP	= NewAEEventHandlerProc(DoAEOpenDoc);
		gAEPrintDocUPP	= NewAEEventHandlerProc(DoAEPrintDoc);
	#endif //OP_PLATFORM_MAC_CARBON_FLAG
								//	install auto Open App
	err = AEInstallEventHandler(kCoreEventClass, kAEOpenApplication, 
								gAEOpenUPP, 0L, false );
													
	if (err == noErr)			// install auto Quit App
		err = AEInstallEventHandler(kCoreEventClass, kAEQuitApplication, 
									gAEQuitUPP, 0L, false );
		
	if (err == noErr)			// install auto Open Document
		err = AEInstallEventHandler(kCoreEventClass,kAEOpenDocuments,
									gAEOpenDocUPP,0L,false);
														
	if (err == noErr)			// install auto Print Document
		err = AEInstallEventHandler(kCoreEventClass,kAEPrintDocuments,
									gAEPrintDocUPP,0L,false);
	
							
	return err;
												
}


//----------------------------------------------------------------------
//
//	AERemove - remove all the allocated apple events
//
//
//----------------------------------------------------------------------

OSErr AERemove(void)
{	
	OSErr		err = noErr;

	// deallocate all the core AppleEvents and assume that they are still around 
	// since we shouldn't have removed them before now.
	err = AERemoveEventHandler(kCoreEventClass, kAEOpenApplication, gAEOpenUPP, false);
	err = AERemoveEventHandler(kCoreEventClass, kAEQuitApplication, gAEQuitUPP, false);
	err = AERemoveEventHandler(kCoreEventClass, kAEOpenDocuments, gAEOpenDocUPP, false);
	err = AERemoveEventHandler(kCoreEventClass, kAEPrintDocuments, gAEPrintDocUPP, false);

	#if (OP_PLATFORM_MAC_CARBON_FLAG)
		DisposeAEEventHandlerUPP(gAEOpenUPP);
		DisposeAEEventHandlerUPP(gAEQuitUPP);
		DisposeAEEventHandlerUPP(gAEOpenDocUPP);
		DisposeAEEventHandlerUPP(gAEPrintDocUPP);
	#else
		DisposeRoutineDescriptor(gAEOpenUPP);
		DisposeRoutineDescriptor(gAEQuitUPP);
		DisposeRoutineDescriptor(gAEOpenDocUPP);
		DisposeRoutineDescriptor(gAEPrintDocUPP);
	#endif //OP_PLATFORM_MAC_CARBON_FLAG
	
	return err;
}


//----------------------------------------------------------------------
//
//	DoAEOpenApp - called by the Finder at launch time
//
//
//----------------------------------------------------------------------

#if (OP_PLATFORM_MAC_CARBON_FLAG)
	pascal OSErr DoAEOpenApp(const AppleEvent *event,AppleEvent *reply,long refCon)
#else
	pascal OSErr DoAEOpenApp(AppleEvent *event,AppleEvent reply,long refCon)
#endif //OP_PLATFORM_MAC_CARBON_FLAG
{
						
	return noErr;
		
}


//----------------------------------------------------------------------
//
//	 DoAEQuitApp -	called when the Finder asks app to quit
//
//
//----------------------------------------------------------------------

#if (OP_PLATFORM_MAC_CARBON_FLAG)
	pascal OSErr DoAEQuitApp(const AppleEvent *event,AppleEvent *reply,long refCon)
#else
	pascal OSErr DoAEQuitApp(AppleEvent *event,AppleEvent reply,long refCon)
#endif
{
	#pragma unused( event, reply, refCon )

	// set the global quit flag
	gDone = true;
	
	return noErr;
		
}


//----------------------------------------------------------------------
//
//	DoAEOpenDoc - called when the Finder asks app to open a document
//
//
//----------------------------------------------------------------------

#if (OP_PLATFORM_MAC_CARBON_FLAG)
	pascal OSErr DoAEOpenDoc(const AppleEvent *event,AppleEvent *reply,long refCon)
#else
	pascal OSErr DoAEOpenDoc(AppleEvent *event,AppleEvent reply,long refCon)
#endif //OP_PLATFORM_MAC_CARBON_FLAG
{
	#pragma unused( event, reply, refCon )
	

	return noErr;

}
				
										
//----------------------------------------------------------------------
//
//	DoAEPrintDoc - called when the Finder asks app to print a document
//
//
//----------------------------------------------------------------------

#if (OP_PLATFORM_MAC_CARBON_FLAG)
	pascal OSErr DoAEPrintDoc(const AppleEvent *event,AppleEvent *reply,long refCon)
#else
	pascal OSErr DoAEPrintDoc(AppleEvent *event,AppleEvent reply,long refCon)
#endif //OP_PLATFORM_MAC_CARBON_FLAG
{
	#pragma unused( event, reply, refCon )
	

	return noErr;

}


//----------------------------------------------------------------------
//
//	GotAEParams - make sure we got proper AE params for an Open 
//				  Document from the Finder
//
//----------------------------------------------------------------------

OSErr GotAEParams(AppleEvent *appleEvent)
{
	OSErr			err;
	DescType		type;
	Size			size;
	
	err = AEGetAttributePtr(appleEvent,keyMissedKeywordAttr,
							typeWildCard,&type,nil,0,&size);
											
	if (err == errAEDescNotFound)
		return(noErr);
	
	else
	{
		if (err == noErr)
			return(errAEEventNotHandled);
	}
	
	return err;

} 
