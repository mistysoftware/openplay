#ifndef __NETSTUFF__
#define __NETSTUFF__

#if USING_OLD_NSP
	#include "NetSprocket.h"

	typedef OSErr	NMErr;
	typedef Str32	NMStr31;

	#define kNSpHostID	kNSpHostOnly
	#define NSpPlayer_GetOTAddress NSpPlayer_GetAddress
#else
	#include "OpenPlay.h"
#endif

extern Boolean gHost;
extern NSpGameReference	gNetGame;

enum {
	kUserCancelled = -100
	};
	
#ifdef __cplusplus
extern "C" {
#endif

NMErr 	InitNetworking(NSpGameID inGameID);
void	ShutdownNetworking(void);
NMErr	DoHost(void);
NMErr 	DoJoin(void);
void	HandleNetwork(void);
void	RefreshWindow(WindowPtr inWindow);
void 	HandleNetMenuChoice(short menu, short item);
void	AdjustNetMenus();
void	DoCloseNetWindow(WindowPtr inWindow);

#ifdef __cplusplus
}
#endif

enum {
	kPlayerInputMessage = 1,
	kGameStateMessage,
	kLeaveMessage
	};
	
#define iJunk		1
#define iNormal		2
#define iRegistered 3
//-------------------
#define iBlocking	5
//-------------------
#define i1X			7
#define i10X		8
#define	i30X		9
#define iNoLimit	10
//-------------------
#define i4			12
#define i16			13
#define	i400		14
#define	i1K			15
#define	i10K		16
#define	i100K		17
//-------------------
#define iEnumerate	19

typedef struct PlayerInputMessage
{
	NSpMessageHeader	h;
	UInt8				data[102400];
} PlayerInputMessage;

typedef struct GameStateMessage
{
	NSpMessageHeader	h;
	UInt8				data[102400];
} GameStateMessage;


typedef struct AddPlayerMessage
{
	NSpMessageHeader	h;
	NSpPlayerID			id;
} AddPlayerMessage;

typedef struct WindowStuff
{
	NSpPlayerID		id;
	char			TextLine1[128];
	char			TextLine2[128];
	char			TextLine3[128];
	char			TextLine4[128];
	
	UInt8			first_decode;
	
	UInt32			gameStateCount;
	UInt32			playerInputCount;
	UInt32			prevGameStateCount;
	UInt32			prevPlayerInputCount;
	UInt32			gameStateRate;
	UInt32			playerInputRate;
	
	UInt32			gameStateBytes;
	UInt32			playerInputBytes;
	UInt32			prevGameStateBytes;
	UInt32			prevPlayerInputBytes;
	float			gameStateThroughput;
	float			playerInputThroughput;
	
	UInt32			totalGameStateBytes;
	UInt32			totalPlayerInputBytes;
	UInt32			prevTotalGameStateBytes;
	UInt32			prevTotalPlayerInputBytes;
	float			totalGameStateThroughput;
	float			totalPlayerInputThroughput;

	UInt32			lastThroughputUpdate;
	
	UInt32			transmitCodeCountGameState;
	UInt32			receiveCodeCountGameState;

	UInt32			transmitCodeCountPlayerInput;
	UInt32			receiveCodeCountPlayerInput;
	
	UInt32			bytesReceived;
	UInt32			erroneousGameStates;
	UInt32			outOfOrderGameStates;
	UInt32			erroneousPlayerInputs;
	UInt32			outOfOrderPlayerInputs;
	Boolean			changed;
} WindowStuff;

#endif