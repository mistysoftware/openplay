# OpenPlay Make file for Linux & Unix targets

#where the makefile is
TOP = $(PWD)

#we building debug or release?
ifneq ($(OP_BUILD_TYPE),release)
	OP_BUILD_TYPE = debug
	OP_DEBUG_DEFINES = -DDEBUG -DDEBUGCALLCHAIN
endif

#openplay headers needed for development
OP_DEVEL_HEADERS = $(TOP)/../../Interfaces/OpenPlay.h

#store intermediates and targets based on architecture and machine
OSTYPE_DIR		= $(OSTYPE)-$(HOSTTYPE)
TARGET_DIR		= $(TOP)/../../Targets/posix/$(OSTYPE_DIR)/$(OP_BUILD_TYPE)
MODULES_DIR		= $(TOP)/../../Targets/posix/$(OSTYPE_DIR)/$(OP_BUILD_TYPE)/OpenPlay\ Modules
DOXYGEN_OP_DIR	= $(TOP)/../../Documentation/html

#standard location for netmodules on posix
OP_NETMODULE_DIR = /usr/share/openplay/netmodules

#stick object files in the dir for this os
OBJECT_DIR = $(TOP)/objs_$(OSTYPE_DIR)-$(OP_BUILD_TYPE)
OUTPUT_OPTION = -o $(OBJECT_DIR)/$@

ifeq ($(OSTYPE),darwin)
	CC = g++	
	INOSTYPE = os_darwin
	GLOBAL_FLAGS = -Dmacho_build -Wno-four-char-constants
	OP_SHLIB_NAME = libopenplay.dylib
	APPFLAGS =	 -L$(TARGET_DIR)  -lopenplay -lpthread -lstdc++ 	
else
	CC = libtool g++
	INOSTYPE = os_posix_generic
	OP_SHLIB_NAME = libopenplay.so
	APPFLAGS = 	-L$(TARGET_DIR)  -ldl -lopenplay -lpthread -lstdc++	
endif

OP_SHLIB_PATH = $(TARGET_DIR)/$(OP_SHLIB_NAME)

TCP_MODULE_NAME = libtcp_ip.so
TCP_MODULE_PATH	= $(MODULES_DIR)/$(TCP_MODULE_NAME)
ENUM_TEST_PATH = $(TARGET_DIR)/openumtest
NSP_TEST_PATH = $(TARGET_DIR)/nsptest
OP_EXAMPLE1_PATH = $(TARGET_DIR)/opexample1
NSP_EXAMPLE1_PATH = $(TARGET_DIR)/nspexample1
MINI_PLAY_PATH = $(TARGET_DIR)/miniplay
OP_DOWNLOADHTTP_PATH = $(TARGET_DIR)/opdownloadhttp

#where to find sources/headers/libs  
OUR_PATHS = $(TOP)/../../Interfaces\
	$(TOP)/../../Source/OpenPlayLib/Common\
	$(TOP)/../../Source/OpenPlayLib/Posix\
	$(TOP)/../../Source/Utilities\
	$(TOP)/../../Source/Utilities/Posix\
	$(TOP)/../../Source/OPNetModules\
	$(TOP)/../../Source/OPNetModules/Common\
	$(TOP)/../../Source/OPNetModules/Posix/TCP_IP\
	$(TOP)/../../Source/Demos/OPEnumTest\
	$(TOP)/../../Source/NetSprocketLib\
	$(TOP)/../../Source/Demos/NSpTestApp\
	$(TOP)/../../Source/Demos/OPMiniDemo\
	$(TOP)/../../Source/Demos/OPExample1\
	$(TOP)/../../Source/Demos/OPDownloadHTTP

	
HINCLUDES = $(OUR_PATHS)
	
#where make looks for sources of any sort (besides the default obj dir)
#.PATH: $(HINCLUDES) $(OBJECT_DIR)
VPATH = $(HINCLUDES) $(OBJECT_DIR)

#summarize our flags to pass to the compiler
INCLUDE_FLAGS = $(addprefix -I,$(OUR_PATHS))
CFLAGS = -g -fPIC -fno-common $(GLOBAL_FLAGS) -D$(INOSTYPE) $(OP_DEBUG_DEFINES) $(INCLUDE_FLAGS)
CXXFLAGS = $(CFLAGS)

#all objects we require for openplayLib
OP_LIB_OBJECTS =	module_management.o\
					op_endpoint.o\
					op_hi.o\
					op_module_mgmt.o\
					op_packet.o\
					openplay.o\
					openplay_posix.o\
					find_files_posix.o\
					dll_utils_posix.o\
					OPUtils.o\
					CATEndpoint_OP.o\
					CIPEndpoint_OP.o\
					CEndpoint_OP.o\
					EntryPoint.o\
					NetSprocketLib.o\
					NSpGame.o\
					NSpGameMaster.o\
					NSpGamePrivate.o\
					NSpGameSlave.o\
					NSpLists_OP.o\
					NSpProtocolRef.o\
					NSp_InterruptSafeList.o\
					machine_lock.o\
					ByteSwapping.o\
					String_Utils.o\
					ERObject.o\
					TPointerArray.o\
					DebugPrint.o\
					op_dlopen.o
				
TCP_MODULE_OBJECTS = 		tcp_module_communication.o\
							tcp_module_config.o\
							tcp_module_enumeration.o\
							tcp_module_gui.o\
							tcp_module_main.o\
							ip_enumeration.o\
							configuration.o\
							OPUtils.o\
							DebugPrint.o\
							machine_lock.o

ENUM_TEST_OBJECTS = OPEnumTest.o

NSP_TEST_OBJECTS = NSpTestApp.o

MINI_PLAY_OBJECTS = miniplay.o

OP_EXAMPLE1_OBJECTS = 	main.o\
						op_network.o\
						OPUtils.o

NSP_EXAMPLE1_OBJECTS =  main.o\
						nsp_network.o
			
OP_DOWNLOADHTTP_OBJECTS = 	OPDownloadHTTP.o\
							OPGetURL.o\
							String_Utils.o
							
################################################################################
#	TARGETS
################################################################################
							
#builds all - default target
MAIN: $(OP_SHLIB_PATH) $(TCP_MODULE_PATH) $(ENUM_TEST_PATH) $(NSP_TEST_PATH) $(OP_EXAMPLE1_PATH) $(NSP_EXAMPLE1_PATH) $(MINI_PLAY_PATH) $(OP_DOWNLOADHTTP_PATH)
	@echo openplay build complete!

#clears out object files from the current posix build
clean:
	-rm -rf $(OBJECT_DIR)
	-rm -rf $(TARGET_DIR)
	-rm -rf $(MODULES_DIR)

#clears out all builds (cw, posix, etc)
distclean: clean
	-rm -rf objs_*
	-rm -rf $(TOP)/../../Targets
	-rm -rf $(TOP)/../ProjectBuilder/OpenPlay.build
	-rm -rf $(TOP)/../CWPro7/*Data
	-rm -rf $(TOP)/../CWPro6/*Data
	-rm -rf $(TOP)/../VC/OpenPlayLib/Release
	-rm -rf $(TOP)/../VC/OpenPlayLib/Debug
	-rm $(TOP)/../VC/OpenPlayLib/*.opt
	-rm $(TOP)/../VC/OpenPlayLib/*.ncb
	-rm -rf $(TOP)/../VC/OPNetModulesWin/Release
	-rm -rf $(TOP)/../VC/OPNetModulesWin/Debug
	-rm $(TOP)/../VC/OPNetModulesWin/*.opt
	-rm $(TOP)/../VC/OPNetModulesWin/*.ncb
	
#required directories
#build_dirs: $(OBJECT_DIR)
#	mkdir -p $(OBJECT_DIR)
	
$(OBJECT_DIR):
	mkdir -p $(OBJECT_DIR)

#create the documentation using doxygen
docs:
	@rm -rf $(DOXYGEN_OP_DIR)
	@mkdir -p $(DOXYGEN_OP_DIR)
	@doxygen DoxygenConfigOP

#build the library itself 
$(OP_SHLIB_PATH): $(OBJECT_DIR) $(OP_LIB_OBJECTS)
	mkdir -p $(TARGET_DIR)
ifeq ($(OSTYPE),darwin)
	cd $(OBJECT_DIR); $(CC) -dynamiclib -o $(OP_SHLIB_PATH)  $(OP_LIB_OBJECTS)
else
	cd $(OBJECT_DIR); ld -shared -o $(OP_SHLIB_PATH) $(OP_LIB_OBJECTS)
#	cd $(OBJECT_DIR); $(CC) -shared -o $(OP_SHLIB_PATH) $(OP_LIB_OBJECTS)
endif
	
#install the library and netmodules
install: MAIN
	cp $(OP_SHLIB_PATH) /usr/lib
	mkdir -p $(OP_NETMODULE_DIR)
	cp $(TCP_MODULE_PATH) $(OP_NETMODULE_DIR)
	cp $(OP_DEVEL_HEADERS) /usr/include

#uninstall the library and netmodules
uninstall:
	rm -f /usr/lib/$(OP_SHLIB_NAME)
	rm -f $(OP_NETMODULE_DIR)/$(TCP_MODULE_NAME)
	-rmdir $(OP_NETMODULE_DIR)
	
#the ip module
$(TCP_MODULE_PATH):  $(OBJECT_DIR) $(TCP_MODULE_OBJECTS)
	mkdir -p $(MODULES_DIR)
ifeq ($(OSTYPE),darwin)	
	cd $(OBJECT_DIR); $(CC) -bundle -flat_namespace -o $(TCP_MODULE_PATH) $(TCP_MODULE_OBJECTS)
else
	cd $(OBJECT_DIR); ld -shared -o $(TCP_MODULE_PATH) $(TCP_MODULE_OBJECTS)
endif

#enumeration test
$(ENUM_TEST_PATH): $(OBJECT_DIR) $(ENUM_TEST_OBJECTS)
	cd $(OBJECT_DIR); $(CC) $(APPFLAGS) -o $(ENUM_TEST_PATH) $(ENUM_TEST_OBJECTS)

#nsp test
$(NSP_TEST_PATH): $(OBJECT_DIR) $(NSP_TEST_OBJECTS)
	cd $(OBJECT_DIR); $(CC) $(APPFLAGS) -o $(NSP_TEST_PATH) $(NSP_TEST_OBJECTS)

#opexample1
$(OP_EXAMPLE1_PATH): $(OBJECT_DIR) $(OP_EXAMPLE1_OBJECTS)
	cd $(OBJECT_DIR); $(CC) $(APPFLAGS) -o $(OP_EXAMPLE1_PATH) $(OP_EXAMPLE1_OBJECTS)

#nspexample1
$(NSP_EXAMPLE1_PATH): $(OBJECT_DIR) $(NSP_EXAMPLE1_OBJECTS)
	cd $(OBJECT_DIR); $(CC) $(APPFLAGS) -o $(NSP_EXAMPLE1_PATH) $(NSP_EXAMPLE1_OBJECTS)

#mini-play
$(MINI_PLAY_PATH): $(OBJECT_DIR) $(MINI_PLAY_OBJECTS)
	cd $(OBJECT_DIR); $(CC) $(APPFLAGS) -o $(MINI_PLAY_PATH) $(MINI_PLAY_OBJECTS)

#download-http
$(OP_DOWNLOADHTTP_PATH): $(OBJECT_DIR) $(OP_DOWNLOADHTTP_OBJECTS)
	cd $(OBJECT_DIR); $(CC) $(APPFLAGS) -o $(OP_DOWNLOADHTTP_PATH) $(OP_DOWNLOADHTTP_OBJECTS)
