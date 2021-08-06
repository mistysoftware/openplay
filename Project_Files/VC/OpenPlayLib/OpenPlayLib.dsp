# Microsoft Developer Studio Project File - Name="OpenPlayLib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=OpenPlayLib - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "OpenPlayLib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "OpenPlayLib.mak" CFG="OpenPlayLib - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "OpenPlayLib - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "OpenPlayLib - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "OpenPlayLib - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "OPENPLAYLIB_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "../../../Interfaces/" /I "../../../Source/NetSprocketLib" /I "../../../Source/NetSprocketLib/Common/" /I "../../../Source/NetSprocketLib/Windows/../../../Source/OpenPlayLib" /I "../../../Source/OpenPlayLib/Common/" /I "../../../Source/OpenPlayLib/Windows/" /I "../../../Source/Utilities/" /I "../../../Source/Utilities/Windows/" /I "../../../Source/OPNetModules/" /I "../../../Source/OPNetModules/Common/" /I "../../../Source/OPNetModules/Windows/" /D "WIN32" /D "OPENPLAY_DLLEXPORT" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "OPENPLAYLIB_EXPORTS" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib WS2_32.LIB /nologo /dll /machine:I386 /out:"../../../Targets/VC/Windows/Release/OpenPlay.dll"

!ELSEIF  "$(CFG)" == "OpenPlayLib - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "OPENPLAYLIB_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../../Interfaces/" /I "../../../Source/NetSprocketLib" /I "../../../Source/NetSprocketLib/Common/" /I "../../../Source/NetSprocketLib/Windows/../../../Source/OpenPlayLib" /I "../../../Source/OpenPlayLib/Common/" /I "../../../Source/OpenPlayLib/Windows/" /I "../../../Source/Utilities/" /I "../../../Source/Utilities/Windows/" /I "../../../Source/OPNetModules/" /I "../../../Source/OPNetModules/Common/" /I "../../../Source/OPNetModules/Windows/" /D "WIN32" /D "OPENPLAY_DLLEXPORT" /D "DEBUG" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "OPENPLAYLIB_EXPORTS" /YX /FD /GZ /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib WS2_32.LIB /nologo /dll /debug /machine:I386 /out:"../../../Targets/VC/Windows/Debug/OpenPlay.dll" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "OpenPlayLib - Win32 Release"
# Name "OpenPlayLib - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\..\Source\Utilities\ByteSwapping.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Source\NetSprocketLib\CATEndpoint_OP.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Source\NetSprocketLib\CEndpoint_OP.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Source\NetSprocketLib\CIPEndpoint_OP.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Source\Utilities\Windows\dll_utils_windows.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Source\NetSprocketLib\EntryPoint.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Source\Utilities\ERObject.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Source\Utilities\Windows\find_files_win32.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Source\Utilities\machine_lock.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Source\OpenPlayLib\Common\module_management.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Source\NetSprocketLib\NetSprocketLib.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Source\NetSprocketLib\NSp_InterruptSafeList.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Source\NetSprocketLib\NSpGame.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Source\NetSprocketLib\NSpGameMaster.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Source\NetSprocketLib\NSpGamePrivate.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Source\NetSprocketLib\NSpGameSlave.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Source\NetSprocketLib\NSpLists_OP.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Source\NetSprocketLib\NSpProtocolRef.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Source\OpenPlayLib\Common\op_endpoint.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Source\OpenPlayLib\Common\op_hi.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Source\OpenPlayLib\Common\op_module_mgmt.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Source\OpenPlayLib\Common\op_packet.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Source\OpenPlayLib\Common\openplay.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Source\OpenPlayLib\Windows\openplay_win32.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Source\Utilities\OPUtils.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Source\Utilities\String_Utils.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Source\Utilities\TPointerArray.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\..\Source\Utilities\ByteSwapping.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Source\NetSprocketLib\CATEndpoint_OP.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Source\NetSprocketLib\CEndpoint_OP.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Source\NetSprocketLib\CIPEndpoint_OP.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Source\Utilities\ERObject.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Source\Utilities\machine_lock.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Source\OpenPlayLib\Common\module_management.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Source\NetSprocketLib\NetSprocketLib.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Source\NetSprocketLib\NSp_InterruptSafeList.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Source\NetSprocketLib\NSpGame.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Source\NetSprocketLib\NSpGameMaster.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Source\NetSprocketLib\NSpGamePrivate.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Source\NetSprocketLib\NSpGameSlave.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Source\NetSprocketLib\NSpLists_OP.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Source\NetSprocketLib\NSpProtocolRef.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Source\OpenPlayLib\Common\op_definitions.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Source\OpenPlayLib\Common\op_globals.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Source\Utilities\String_Utils.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Source\Utilities\TPointerArray.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\..\..\Source\OpenPlayLib\Windows\binaries.rc
# End Source File
# End Group
# End Target
# End Project
