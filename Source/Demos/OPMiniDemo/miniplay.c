/* ----------------------------------------------------------------------
/ 
/   FILE:  MiniPlay.c
/  
/   AUTHOR:  Joe Gervais, CER (Chief Executive Rodent), Lead Engineer
/            Naked Mole-Rat Software, <www.NakedMoleRat.com>
/
/   REV:  1.3a  08/2/2000
/
/	Changes for 1.3a:
/
/		- Raw implementation of enumeration added.
/
/		- First stab at Windows version.
/		
/		- Added "// OpenPlay API" comment preceding all OpenPlay API
/		  calls (to help newcomers see exactly where OpenPlay was being
/		  used vs. sample code).
/
/
/	Changes for 1.2:
/
/		- Now able to select and use both AppleTalk and TCP/IP.
/
/		- Windows version coming soon (for anyone wishing to volunteer,
/		  the code is already cross-platform safe).
/
/		- Comments updated.
/
/
/	Changes for 1.1:  
/
/		- Now passing events to SIOUX console app, so console window
/		  is properly updated under MacOS.
/
/		- Changed SIOUX settings to quit without asking to save output.
/
/
/   DESCRIPTION:  
/
/		- MiniPlay is a "bare metal" OpenPlay example app, using only 
/		  the core required OpenPlay routines to send data. MiniPlay
/		  was written as a learning tool to help make the jump to 
/		  fully understanding the OpenPlay Test app, which is a bit
/		  opaque.
/
/		- To run the app:
/
/			1) Build the MacOS Debug target in OpenPlayMasterBuild.mcp.
/				
/			2) Run the first MiniPlay app as a server (enter 'S' or 
/			   's' at the prompt). Select which protocol to start up
/			   with. Its status will print out to let you know if the
/			   server successfully initialized.
/
/              IMPORTANT (PPP Users): If MiniPlay fails to start a 
/			   TCP/IP server, check and see if you have a TCP/IP 
/			   connection active.
/
/			3) Run the second MiniPlay app as a client. When the 
/			   Client Menu appears, first open the connection to  
/			   the server using a matching protocol, then try sending 
/			   a few packets. 
/
/			4) When you shutdown the client, the server will print 
/			   the collection of packets it received and will also
/			   shutdown.
/
/ 
/   NOTES:  
/
/       - This is a BARE METAL learning tool. My goal was to strip
/         away EVERYTHING but the most core, essential elements
/         of what an OpenPlay app needs to do to connect and send  
/         packets between two processes. All bells and whistles have
/         been gutted. Nevertheless, error checking/handling is 
/         reasonably thorough. 
/
/       - All user I/O is via text display (using the SIOUX console
/         on MacOS, I presume Windows provides some form of support
/         for printf() console-like I/O). Since printf() on MacOS
/         does NOT appear to be interrupt-safe, adding calls to printf()
/         within the kNMDatagramData case of the callback function  
/         MAY CAUSE YOUR PROGRAM TO CRASH. 
/
/       - Setting debugging breakpoints in the asynchronous callback   
/         function will likely cause Codewarrior Pro5 to crash and 
/         burn. You have been warned.
/
/
/   THINGS OUR LEGAL COUNSEL MADE US SAY:
/
/   (c) 1999 Naked Mole-Rat Software, except where superceded by the
/   Apple Public Source License. All rights reserved.
/
/   This code may be freely used and distributed provided this entire
/   file is kept intact.
/   
/   OpenPlay is released under the Apple Public Source License (the
/   'License'), and may not be used except in compliance with the 
/   License. Please obtain a copy of the License at:
/
/            <http://www.apple.com/publicsource>
/
/   and read it before using this file.
/
/   This code is provided 'AS IS', WITHOUT WARRANTY OF ANY KIND, EITHER 
/   EXPRESS OR IMPLIED. NAKED MOLE RAT SOFTWARE AND THEIR AGENTS HEREBY 
/   DISCLAIM ALL SUCH WARRANTIES, INCLUDING AND WITHOUT LIMITATION ANY
/   WARRANTIES AGAINST LOSS OF DATA, LOSS OF EQUIPMENT, LOSS OF PRODUCTIVITY,
/   OR OTHER NEGATIVE CONSEQUENCES OF USING THIS CODE.
/
/   (In other words, if the result of compiling and running this
/    code is a Plague O' Locusts, the collapse of the Pork Bellies
/    Futures market, or the 3rd Rise Of Disco, we aren't responsible.
/    Proceed at your own risk.)
/
/ ------------------------------------------------------------------------ */

#include "OpenPlay.h"
#include "OPUtils.h"
#include <stdio.h>
#include <unistd.h>

#if defined( OP_PLATFORM_MAC_CFM )
	#include <sioux.h>
#endif


#define  kMaxPackets  256      // Max packets to receive & store.

static const char *kIPConfigAddressTag = "IPaddr";  // KLUDGE:  Tags stolen from NetModuleIP.h, 
static const char *kIPConfigPortTag    = "IPport";  // module_config.c, etc. -  See comments in 
static const char *kTypeTag            = "type";    // Open_Connection() below. Tags used to 
static const char *kVersionTag         = "version"; // manually build config string when necessary.
static const char *kGameIDTag          = "gameID";
static const char *kGameNameTag        = "gameName";
static const char *kModeTag            = "mode";

const char *kLoopBackAddress  = "127.0.0.1";

const NMType    kGameID              = 'MOOF';
const NMUInt32  kAppleTalk_ProtoTag  = 'Atlk';
const NMUInt32  kTCP_ProtoTag        = 'Inet';

typedef enum { kAppleTalk, kTCP } ProtocolType;
typedef enum { kClient, kServer } EndpointType;


// The sample structure that will be sent.
typedef struct {

	NMUInt32  data;  // Stuffed with a generic number.
	NMUInt32  id;    // Serialized ID, helps track packets.

} PlayerPacket;


// ---------------  Globals  -----------------
// Sure, global variables are usually a source 
// of evil, but they make this sample app easier
// to follow.                                  

PEndpointRef  gLocalEndpoint = NULL;

int           gPacketCount = 0;
PlayerPacket  gPackets[ kMaxPackets ];

NMBoolean     gDone = false;
NMBoolean     gInServerMode = false;
NMBoolean     gClientConnected = false;

NMUInt32      gEnumHostID = 0;
char          gEnumHostName[ BUFSIZ ];


// ----------  Function Prototypes  ----------

NMBoolean     IsError( NMErr err );
PEndpointRef  Create_Endpoint( PConfigRef config, NMBoolean active );
void                 Do_Client_Menu( void );
void                 Do_Server_Menu( void );

NMBoolean     Get_Protocol( NMType *type, ProtocolType protocolType );

NMBoolean     Create_Config( EndpointType endpointType,
                             ProtocolType protocolType, 
                             NMType protocol, 
                             const char *hostIPStr, 
                             PConfigRef *config );

NMBoolean     Open_Connection( EndpointType endpointType, 
                               ProtocolType protocolType,
                               const char *hostIPStr );
                               
void          Accept_Connection( PEndpointRef inEndpoint, void* inCookie );
void          Send_Packet( void );
void          Close_Connection( void );
void          Print_Packets( void );

void          OpenPlay_Callback( PEndpointRef inEndpoint, 
                                 void* inContext,
	                             NMCallbackCode inCode, 
	                             NMErr inError, 
	                             void* inCookie );

void          Do_Enumeration( PConfigRef config );

void          Enumeration_Callback( void *inContext,
                                    NMEnumerationCommand inCommand, 
                                    NMEnumerationItem *item );



// --------------  Begin Code  --------------

int 
main( void ) 
{	
	NMEvent event;
	char  mode;
	NMBoolean  gotEvent;
	
#if defined( OP_PLATFORM_MAC_CFM )
	SIOUXSettings.asktosaveonclose = false;
#endif
	
	printf( "Server or Client Mode? (s/c) --> " );
	fflush(stdout);
	
	mode = getchar();
	
	if ( mode == 's' || mode == 'S' ) {
	
		gInServerMode = true;
		printf( "\nSERVER MODE ACTIVE.\n" );
		fflush(stdout);
	}

	while ( !gDone ) {

		// OS-SPECIFIC SYSTEM CALL, MacOS in this case.
		// Cut some CPU cycles loose for system stuff,
		// else things get ugly. Linux/Windows developers,
		// this should be the only line you need to change.
		   
#if defined(OP_PLATFORM_MAC_CFM)

		gotEvent = WaitNextEvent( everyEvent, &event, 1, NULL );
		
		if ( gotEvent ) {	// Give the SIOUX console the events it needs.
		
			SIOUXHandleOneEvent( &event );
			if (event.what == keyDown) {
			
				char 	aKey;
			
				aKey = (char) ( event.message & charCodeMask );
				if (aKey == 'q') {
					
					gDone = true;
				}
			}
			
		}
#endif
		if ( ! gInServerMode ) {

			Do_Client_Menu();			
		}
		else if ( gLocalEndpoint == NULL ) {
			
			Do_Server_Menu();
		}
	}

	printf( "Closing connection, exiting...\n\n" );
	fflush(stdout);
	
	Close_Connection();
	
	if ( gInServerMode ) {
	
		Print_Packets();
	}

	printf( "\nDone!\n" );
	fflush(stdout);

	return( 0 );
}



/* --------------------------------------------------------------
/ 
/  FUNCTION:  IsError()
/  
/  PARAMETERS:
/ 
/      err  - (IN)  Error value to check.
/ 
/  DESCRIPTION:  'Nuff said.
/ 
/  NOTES:  I hate macros. In C++ I'd make it an inline function....
/ 
/  --------------------------------------------------------------*/

NMBoolean 
IsError( NMErr err ) 
{
    // OpenPlay returns negative numbers for all errors.
    
	return ( ( err < 0 ) ? true : false ); 
}



/* --------------------------------------------------------------
/ 
/  FUNCTION:  Do_Client_Menu()
/  
/  PARAMETERS:
/ 
/      N/A
/ 
/  DESCRIPTION:  Cheesy little console menu to get user input. Hey,
/      at least it's cross-platform. ;-)
/ 
/  --------------------------------------------------------------*/

void 
Do_Client_Menu( void ) 
{
	char  selection;
	char  hostIPStr[ BUFSIZ ];
	
	printf( "\nSelect:\n\n\t" );
	printf( "1 - Open AppleTalk Connection to Server\n\t" );
	printf( "2 - Open TCP/IP Connection to Server\n\t" );
	printf( "3 - Send Packet\n\t" );
	printf( "4 - Shutdown App (will trigger server to print results...)\n\n" );

	printf( "--> " );
	fflush(stdout);
	
	do {
		selection = getchar();
	} while (  selection < '1' || selection > '4' );
	
	
	switch ( selection ) {
	
		case '1':
			printf( "Starting AppleTalk client...\n" );
			fflush(stdout);
			Open_Connection( kClient, kAppleTalk, NULL );
			break;
		
		case '2':
			
			printf( "\nEnter IP Address of host (ex. 254.171.0.2): " );
			fflush(stdout);
			scanf( "%s", hostIPStr );
			
			printf( "\nStarting TCP/IP client, connecting to host %s...\n", hostIPStr );
			fflush(stdout);
			Open_Connection( kClient, kTCP, hostIPStr );
			break;
		
		case '3':
			printf( "Sending packet...\n" );
			fflush(stdout);
			Send_Packet();
			break;
		
		case '4':
			gDone = true;
			break;
			
		default:
			printf( "Selection out of range, blame society!\n" );
			fflush(stdout);
	}
	
	return;
}



/* --------------------------------------------------------------
/ 
/  FUNCTION:  Do_Server_Menu()
/  
/  PARAMETERS:
/ 
/      N/A
/ 
/  DESCRIPTION:  Cheesy little console menu to get user input. Hey,
/      at least it's cross-platform. ;-)
/ 
/  --------------------------------------------------------------*/

void 
Do_Server_Menu( void ) 
{
	char  selection;
	NMBoolean  success = true;
	
	printf( "\nSelect:\n\n\t" );
	printf( "1 - Open Appletalk Server\n\t" );
	printf( "2 - Open TCP/IP Server\n\t" );
	printf( "3 - Abort\n\n" );

	printf( "--> " );
	fflush(stdout);
	
	do
	{
		selection = getchar();		
	}while ((selection < '0') || (selection > 'z'));
	
	switch ( selection ) {
	
		case '1':
			printf( "Starting Appletalk server...\n" );
			fflush(stdout);
			success = Open_Connection( kServer, kAppleTalk, NULL );
			break;
		
		case '2':
			printf( "Starting TCP/IP server...\n" );
			fflush(stdout);
			success = Open_Connection( kServer, kTCP, kLoopBackAddress );
			break;
		
		case '3':
			gDone = true;
			break;
			
		case 'q':
			gDone = true;
			break;
			
		default:
			printf( "Selection out of range, blame society!\n" );
			fflush(stdout);
	}
	
	if ( success ) {
		printf( "Waiting for client to connect... \n\n");
		#if (OP_PLATFORM_MAC_CFM)
			printf("Press 'q' to stop or Client Send Close)\n\n" );
		#endif
		fflush(stdout);
	}
	else {
		gDone = true;
	}

	return;
}



/* --------------------------------------------------------------
/ 
/  FUNCTION:  Get_Protocol()
/  
/  PARAMETERS:
/ 
/      type  - (OUT)  Protocol selected.
/ 
/  DESCRIPTION:  Searches for the desired protocol module.
/ 
/  NOTES:  Verify the returned NMBoolean for success, or risk 
/      passing trash into ProtocolCreateConfig() when you return
/      to Open_Connection().
/ 
/  --------------------------------------------------------------*/

NMBoolean 
Get_Protocol( NMType *type, ProtocolType protocolTarget )	
{
	NMErr  err = 0;
	NMBoolean  protocolFound = false;
	NMBoolean  status = false;
	NMProtocol  protocol;
	unsigned int  index = 0;
	
	
	protocol.version = CURRENT_OPENPLAY_VERSION;
	
	while ( ! IsError( err ) && ! protocolFound ) {
	
		// OpenPlay API
		err = GetIndexedProtocol( index++, &protocol );
		
		if ( ! IsError( err ) ) {
		
			if ( protocolTarget == kAppleTalk &&
			     protocol.type == kAppleTalk_ProtoTag ) {
			     
			     protocolFound = true;
			}
			else if ( protocolTarget == kTCP &&
			          protocol.type == kTCP_ProtoTag ) {
			     
			     protocolFound = true;
			}
		}
	}

	if ( ! IsError( err ) ) {

		*type = protocol.type;  // Copy by value, it's safe.
		status = true;
	}
	else {

		printf( "ERROR: %d from GetIndexedProtocol()...\n", err);
		fflush(stdout);
	}

	return( status );
}



/* --------------------------------------------------------------
/ 
/  FUNCTION:  Send_Packet()
/  
/  PARAMETERS:
/ 
/      N/A
/ 
/  DESCRIPTION:  Sends a packet down the endpoint.
/ 
/  NOTES:  In MiniPlay, only the client will call Send_Packet().
/ 
/  --------------------------------------------------------------*/

void 
Send_Packet( void )
{
	NMErr  err;
	NMUInt32  packetID = 0;
	static PlayerPacket  packet;
	                                 
	// Don't try to send data down a NULL endpoint....
	
	if ( gLocalEndpoint != NULL ) {
		
		// Stuff some data into the packet.
		packet.id = packetID++; // Helps tell packets apart when printed.
		packet.data = 42;	    // Because 42 is The Answer....

	
//perform byteswapping on little-endian systems(x86, etc) to put our data in network(big-endian) order 
#if (little_endian)
		DEBUG_PRINT("we have as 0x%x",packet.id);
		packet.id = SWAP4(packet.id);
		packet.data = SWAP4(packet.data);
		DEBUG_PRINT("going out as 0x%x",packet.id);
#endif		
		// OpenPlay API
		err = ProtocolSendPacket( gLocalEndpoint, 
		                          (void*) &packet, 
		                          sizeof( packet ),
		                          0 );
		
		if ( IsError( err ) ) {
			printf( "ERROR: %d on ProtocolSendPacket!\n", err );
			fflush(stdout);
		}
	}
	else {
		printf( "No connection to server yet!\n" );	
		fflush(stdout);
	}
	
	return;
}



/* --------------------------------------------------------------
/ 
/  FUNCTION:  Create_Endpoint()
/  
/  PARAMETERS:
/ 
/      config  -
/      active  -
/ 
/  DESCRIPTION:  Attempts to open endpoint using config data.
/
/  NOTES:  According to the OpenPlay Programmer's Dox, the 'flag'
/     parameter to ProtocolOpenEndpoint() is unused, but from
/     looking at the OpenPlay source and running this app with
/     different flags (well, either kOpenActive or kOpenNone) it
/     seems ProtocolOpenEndpoint() DOES USE THE FLAGS.
/ 
/  --------------------------------------------------------------*/

PEndpointRef
Create_Endpoint( PConfigRef config, NMBoolean active )
{
	NMErr  err;
	NMOpenFlags  flags;
	static PEndpointRef  newEndpoint;
	
	if ( config == NULL ) {
		printf( "ERROR: NULL PConfigRef parameter in Create_Endpoint()!\n" );
		fflush(stdout);
		return( NULL );
	}
	
	if ( active ) {
		flags = kOpenActive;
	} 
	else {
		flags = kOpenNone;
	}

	printf( "\t\tAttempting to open endpoint...\n" );
	fflush(stdout);

	// OpenPlay API
	err = ProtocolOpenEndpoint( config, 
	                            OpenPlay_Callback, 
	                            NULL, 
	                            &newEndpoint, 
	                            flags );

	if ( IsError( err ) ) {
		newEndpoint = NULL;
		printf( "\tERROR: %d opening endpoint. (Verify a server is running \n", err );
		printf( "\t       and that your network connection is active.) \n" );
		fflush(stdout);
	}
	
	return( newEndpoint );
}



/* --------------------------------------------------------------
/ 
/  FUNCTION:  OpenPlay_Callback()
/  
/  PARAMETERS:
/ 
/      inEndpoint  -  (All of these are fed from OpenPlay. See
/      inContext       the OpenPlay Dox for more info.)
/      inCode      
/      inError     
/      inCookie    
/ 
/  DESCRIPTION:  The callback routine that OpenPlay will call
/      (rather abruptly) when an OpenPlay event occurs.
/ 
/  NOTES:
/ 
/      Been wondering why all the hassle in OpenPlay Test was made 
/      to create 'interrupt_safe_log' routines? To keep you from
/      going down in flames. But to reduce the code to the barest
/      of essentials, I flirt with disaster using printf()'s. As
/      long as they don't show up in the kNMDatagramData case,
/      all seems to flow smoothly.
/
/  --------------------------------------------------------------*/

void
OpenPlay_Callback( PEndpointRef inEndpoint, 
	               void* inContext,
	               NMCallbackCode inCode, 
	               NMErr inError, 
	               void* inCookie )	
{
	NMErr err;
	PlayerPacket  packet;
	unsigned long  data_length = sizeof( packet );
	NMFlags flags;

	// Re-entrant programs are annoying....
	if ( gDone ) {
		return;
	}

// TEMP REMOVE - commented out printfs for a test

	switch( inCode ) {
	
		case kNMConnectRequest:

//			printf( "Got a connection request!\n" );
			Accept_Connection( inEndpoint, inCookie );
			break;
			

		case kNMDatagramData:

//printf( "Packet Man - Gotta catch 'em all!.\n" );

			// According to the OpenPlay Programmer's Dox entry for 
			// ProtocolReceivePacket(), when kNMDatagramData is received
			// you should keep calling ProtocolReceivePacket() until a
			// kNMNoDataErr is returned. Of course I quit the loop as 
			// soon as ANY error is received, but the point holds. Seems
			// we need to drain the buffer of any backed-up packets. 
			   				   
			// OpenPlay API
			err = ProtocolReceivePacket( inEndpoint, &packet, &data_length, &flags );
			
			while ( ! IsError( err ) && gPacketCount < kMaxPackets - 1) {

				//perform byteswapping on little-endian systems(x86, etc) to get our data out of network(big-endian) order 
				#if (little_endian)
					packet.id = SWAP4(packet.id);
					packet.data = SWAP4(packet.data);
				#endif		

				gPackets[ gPacketCount ].id = packet.id;
				gPackets[ gPacketCount ].data = packet.data;
				gPacketCount++;
				
				printf( "We got a new package.\n");
				fflush(stdout);

				// Evil - since ProtocolReceivePacket() uses my 'data_length'
				// parameter for both input *and* output, need to be sure and
				// reset it each time through. Just in case. 
				   
				data_length = sizeof( packet );

				// OpenPlay API
				err = ProtocolReceivePacket( inEndpoint, &packet, &data_length, &flags );

				
			}
			
			break;

			
		case kNMStreamData:
//			printf( "Got kNMStreamData message (no action taken).\n" );
			break;


		case kNMFlowClear:
//			printf( "Got kNMFlowClear message (no action taken).\n" );
			break;


		case kNMAcceptComplete:
			printf( "Accept complete!\n" );
//			printf( "(Idling, collecting packets until client disconnects...)\n" );
			fflush(stdout);
			gClientConnected = true; 			
			break;


		case kNMHandoffComplete:
			printf( "Handoff complete!\n" );
			printf( "Got kHandoffComplete: Cookie: 0x%x\n", inCookie );
			fflush(stdout);
			break;


		case kNMEndpointDied:
//			printf( "Endpoint died, closing...\n" );
			gDone = true;
			break;


		case kNMCloseComplete:
//			printf( "Endpoint successfully closed.\n" );
			gDone = true;
			break;


		default:
//			printf( "Got unknown inCode in the callback.\n" );
			break;
	}
	
	return;
}



/* --------------------------------------------------------------
/ 
/  FUNCTION:  Create_Config()
/  
/  PARAMETERS:
/ 
/      endpointType  - (IN)  Whether to open active or passive.
/      protocolType  - (IN)  Protocol desired.
/      protocol      - (IN)  OpenPlay protocol tag.
/      hostIPStr     - (IN)  IP address, in case we're using TCP.
/      configHandle  - (OUT)  Pointer to config ref we'll build.
/ 
/  DESCRIPTION:  Tries to create a config reference. 
/ 
/  NOTES:  
/ 
/  --------------------------------------------------------------*/

NMBoolean
Create_Config( EndpointType endpointType,
               ProtocolType protocolType, 
               NMType protocol,
               const char *hostIPStr, 
               PConfigRef *configHandle ) 
{
	char       infoStr[ 1024 ];
	short      configStrLen = 0;
	NMBoolean  result = false;
	NMErr      err = 0;
	char       configStr[ BUFSIZ ];
 
	memset( configStr, 0, sizeof( configStr ) );

	if ( protocolType == kTCP ) {
	
		// If we're running as a TCP/IP process, we need to feed the target 
		// host's IP address into ProtocolCreateConfig. Unfortunately that 
		// means building the obtuse config string ourselves.... Note the
		// tab-delineated fields. We need to bring these OpenPlay config 
		// string tags to a publicly-accessible level so the user can refer 
		// to module-defined constant data types instead of having to 
		// hardcode it themselves.

		sprintf( configStr, "%s=%u\t%s=%u\t%s=%u\t%s=%s\t%s=%u\t%s=%s\t%s=%u", 
		              kTypeTag, protocol,
		              kVersionTag, 0x00000100,
		              kGameIDTag, kGameID,
		              kGameNameTag, "Clarus",
		              kModeTag, 3,
		              kIPConfigAddressTag, hostIPStr,
		              kIPConfigPortTag, 25710
		            );

		printf( "\n\t TCP/IP Config string: [ %s ]\n",  configStr );
		fflush(stdout);
	}

	// OpenPlay API
	err = ProtocolCreateConfig( protocol,    // Which protocol to use (TCP/IP, Appletalk, etc.)
	                            kGameID, // 'Key' to find same game type on network.
	                            "Clarus",// 'Key' to a particular game of that type.
	                            NULL, 
	                            0, 
	                            configStr, 
	                            configHandle    // Retrieve your config info with this critter.
	                          );

	if ( ! IsError( err ) ) {
	
		result = true;
	
		printf( "\nCreated Config Ref.\n" );
		fflush(stdout);
		
		// OpenPlay API
		ProtocolGetConfigStringLen( *configHandle, &configStrLen );
		
		// OpenPlay API
		ProtocolGetConfigString( *configHandle, infoStr, sizeof(infoStr) );
		
		printf( "\nConfig Info:  %s\n", infoStr );
		fflush(stdout);
	}
	else {
	
		printf( "ERROR: %d creating config.\n", err );
		fflush(stdout);
	}
		
	return( result );
}



/* --------------------------------------------------------------
/ 
/  FUNCTION:  Open_Connection()
/  
/  PARAMETERS:
/ 
/      endpointType  - (IN)  Whether to open active or passive.
/      protocolType  - (IN)  Protocol desired.
/      hostIPStr     - (IN)  IP address to use if TCP/IP used.
/ 
/  DESCRIPTION:  Tries to open either an active or passive 
/      connection. 
/ 
/  NOTES:  
/ 
/  --------------------------------------------------------------*/

NMBoolean
Open_Connection( EndpointType endpointType, 
                 ProtocolType protocolType,
                 const char *hostIPStr )
{
	NMErr err = 0;
	NMType type;
	PConfigRef config;
	NMBoolean  activeConnection = false;
	NMBoolean  state = false;
	
	if ( gLocalEndpoint != NULL ) {
	
		printf( "Connection already opened, Chumley!\n" );
		fflush(stdout);
		return( true );
	}
	
	if ( endpointType == kClient ) {
	
		activeConnection = true;
	}

	// Grab target protocol.
	
	if ( Get_Protocol( &type, protocolType ) ) {

		printf( "\nProtocol selected: %c%c%c%c\n",
				        (type>>24) & 0xFF, 
				        (type>>16) & 0xFF,	
				        (type>>8) & 0xFF, 
				        type & 0xFF );
		fflush(stdout);

		if ( Create_Config( endpointType, protocolType, type, hostIPStr, &config ) ) {

			if ( ! gInServerMode && protocolType == kAppleTalk ) {
			
				Do_Enumeration( config );
			}
				
			gLocalEndpoint = Create_Endpoint( config, activeConnection );
			
			if ( gLocalEndpoint != NULL ) {
			
				state = true;  // Connection is good.
			
				printf( "\nCreated endpoint from Config Ref. %s\n",
				              ( activeConnection ? "Active (client)" : "Passive (server)" ) );
				fflush(stdout);

				if ( ! activeConnection ) {
					// Passive connection == Server connection for MiniPlay.
					gInServerMode = true;
				}
			}	
			else {

				printf( "ERROR: failed to create endpoint.\n" );
				fflush(stdout);
			}
			
			// OpenPlay API
			err = ProtocolDisposeConfig( config );
			
			if ( IsError( err ) ) {
				// Not a fatal error, but need to report it.
				printf( "ERROR: %d disposing the config.\n", err );
				fflush(stdout);
			}
		} 
	}
	
	else {
		printf( "ERROR: Unable to find protocol module.\n");
		fflush(stdout);
	}
	
	return( state );
}



/* --------------------------------------------------------------
/ 
/  FUNCTION:  Accept_Connection()
/  
/  PARAMETERS:
/ 
/      inEndpoint  - (IN)  Something to feed directly to OpenPlay.
/      inCookie    - (IN)  Ditto.
/ 
/  DESCRIPTION:  Attempt to complete a connection request.
/ 
/  --------------------------------------------------------------*/

void 
Accept_Connection( PEndpointRef inEndpoint, void* inCookie )
{
	NMErr err;

	// Only accept a single client for this test app....
	
	if ( ! gClientConnected ) {
		
		// OpenPlay API
		err = ProtocolAcceptConnection( inEndpoint, 
		                                inCookie, 
			                            OpenPlay_Callback, 
			                            NULL );
			
		if ( ! IsError( err ) ) {
			printf( "Accepting connection...\n" );
			fflush(stdout);
		} 
		else {
			printf( "ERROR: %d Failed connection request!\n", err );
			fflush(stdout);
		}
	} 
	
	// Else we already have our connection, tell the extra client
	// to go pound sand.
	
	else {
	
		printf( "Connection refused - Server already connected!\n" );
		fflush(stdout);
		
		// OpenPlay API
		err = ProtocolRejectConnection( inEndpoint, inCookie );
		
		if ( IsError( err ) ) {
			printf( "ERROR: %d in ProtocolRejectConnection()!\n", err );
			fflush(stdout);
		}
	}

	return;
}



/* --------------------------------------------------------------
/ 
/  FUNCTION:  Close_Connection
/  
/  PARAMETERS:
/ 
/      N/A
/ 
/  DESCRIPTION:  Shuts down the endpoint.
/ 
/  --------------------------------------------------------------*/

void 
Close_Connection( void )
{	
	NMErr err;
	const char *mPython = "The Comfy Chair!";  // Not actually used in this function.  
	                                           // But a mighty fine sketch....
	
	if ( gLocalEndpoint != NULL ) {

		// Let OpenPlay clean up.
		// OpenPlay API
		err = ProtocolCloseEndpoint( gLocalEndpoint, false );	

		if ( IsError( err ) ) {
			printf( "ERROR: %d closing endpoint!\n", err );
			fflush(stdout);
		}

		gLocalEndpoint = NULL;
	}

	return; 
}



/* --------------------------------------------------------------
/ 
/  FUNCTION:  Enumeration_Callback()
/  
/  PARAMETERS:
/ 
/  DESCRIPTION: 
/ 
/  --------------------------------------------------------------*/

void  
Enumeration_Callback( void *inContext,
                      NMEnumerationCommand inCommand, 
                      NMEnumerationItem *item ) {

	// 11/29/2000 [GG] - safety catch for null item - it happens!
	if( item ) {
		strcpy( gEnumHostName, item->name );
		gEnumHostID = item->id;
	}

	return;
}



/* --------------------------------------------------------------
/ 
/  FUNCTION:  Do_Enumeration()
/  
/  PARAMETERS:
/ 
/  DESCRIPTION: 
/ 
/  --------------------------------------------------------------*/

void          
Do_Enumeration( PConfigRef config ) {

	NMErr err;
	char c = 0;
	int x;
	
	printf( "\n\nSCANNING FOR AVAILABLE HOSTS\n" );
	fflush(stdout);
	
	ProtocolStartEnumeration( config,
                              Enumeration_Callback, 
                              NULL, 
                              true
                            );

	for ( x = 0; x < 5; x++ ) {
		err = ProtocolIdleEnumeration( config );
		printf( ".\n" );
		fflush(stdout);
		sleep( 1 );
	}

	printf( "\n\tHost Found: \t HostID = %u, Name = %s \n\n", gEnumHostID, gEnumHostName );
	fflush(stdout);

	printf( "Bind to Host %s / ID %u? (y/n):  ", gEnumHostName, gEnumHostID );
	fflush(stdout);
	
	while ( c != 'y' && c != 'Y' && c != 'n' && c != 'N' ) {
	
		c = getchar();
	}
	
	if ( c == 'y' || c == 'Y' ) {

		err = ProtocolBindEnumerationToConfig( config,  gEnumHostID );
		
		if ( IsError( err ) ) {
		
			printf( "ERROR: Could not bind enumeration item to config.\n" );
			fflush(stdout);
		}
	}

	err = ProtocolEndEnumeration( config );
	
	return;
}



/* --------------------------------------------------------------
/ 
/  FUNCTION:  Print_Packets()
/  
/  PARAMETERS:
/ 
/      N/A
/ 
/  DESCRIPTION:  Prints out any packets that may have been collected
/      before the client disconnected.
/ 
/  --------------------------------------------------------------*/

void 
Print_Packets( void )
{
	int  x;
	
	printf( "Packets received: %d\n\n", gPacketCount );
	fflush(stdout);
	
	for ( x = 0; x < gPacketCount && x < kMaxPackets; x++ ) {
	
		printf( "\tPacket[ %d ]: ID=%d, data=%d\n",
		        x, gPackets[ x ].id, gPackets[ x ].data );
		fflush(stdout);
	}

	return;
}

// EOF  MiniPlay.c
