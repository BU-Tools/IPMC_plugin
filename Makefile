PackagePath = $(shell pwd)


LIBRARY_IPMISOL_DEVICE = lib/libBUTool_IPMISOLDevice.so
LIBRARY_IPMISOL_DEVICE_SOURCES = $(wildcard src/IPMISOLDevice/*.cc)
LIBRARY_IPMISOL_DEVICE_OBJECT_FILES = $(patsubst src/%.cc,obj/%.o,${LIBRARY_IPMISOL_DEVICE_SOURCES})

LIBRARY_IPMISOL = lib/libBUTool_IPMISOL.so
LIBRARY_IPMISOL_SOURCES = $(wildcard src/IPMISOL/*.cc)
LIBRARY_IPMISOL_OBJECT_FILES = $(patsubst src/%.cc,obj/%.o,${LIBRARY_IPMISOL_SOURCES})


INCLUDE_PATH = \
							-Iinclude  \
							-I$(BUTOOL_PATH)/include \
							-I$(IPBUS_REG_HELPER_PATH)/include

LIBRARY_PATH = \
							-Llib \
							-L$(BUTOOL_PATH)/lib \
							-L$(IPBUS_REG_HELPER_PATH)/lib

ifdef BOOST_INC
INCLUDE_PATH +=-I$(BOOST_INC)
endif
ifdef BOOST_LIB
LIBRARY_PATH +=-L$(BOOST_LIB)
endif

LIBRARIES =    	-lToolException	\
		-lBUTool_IPBusIO \
		-lboost_regex




CPP_FLAGS = -std=c++11 -g -O3 -rdynamic -Wall -MMD -MP -fPIC ${INCLUDE_PATH} -Werror -Wno-literal-suffix

CPP_FLAGS +=-fno-omit-frame-pointer -Wno-ignored-qualifiers -Werror=return-type -Wextra -Wno-long-long -Winit-self -Wno-unused-local-typedefs  -Woverloaded-virtual

LINK_LIBRARY_FLAGS = -shared -fPIC -Wall -g -O3 -rdynamic ${LIBRARY_PATH} ${LIBRARIES} -Wl,-rpath=$(IPBUS_REG_HELPER_PATH)/lib,-rpath=${PackagePath}/lib



# ------------------------
# IPBUS stuff
# ------------------------
UHAL_LIBRARIES = -lcactus_uhal_log 		\
                 -lcactus_uhal_grammars 	\
                 -lcactus_uhal_uhal 		

UHAL_INCLUDE_PATH = \
	         					-isystem$(CACTUS_ROOT)/include 

UHAL_LIBRARY_PATH = \
							-L$(CACTUS_ROOT)/lib 

UHAL_CPP_FLAGHS = ${UHAL_INCLUDE_PATH}

UHAL_LIBRARY_FLAGS = ${UHAL_LIBRARY_PATH}





.PHONY: all _all clean _cleanall build _buildall _cactus_env

default: build
clean: _cleanall
_cleanall:
	rm -rf obj
	rm -rf bin
	rm -rf lib


all: _all
build: _all
buildall: _all
_all: _cactus_env ${LIBRARY_IPMISOL_DEVICE} ${LIBRARY_IPMISOL}

_cactus_env: 
ifndef CACTUS_ROOT
	$(warning CACTUS_ROOT enviornment variable is not set)
	$(warning CACTUS_ROOT must list the path containing the CACTUS libraries and must be added to the LD_LIBRARY_PATH)
	$(error   CACTUS NOT FOUND)
endif

# ------------------------
# IPBusRegHelper library
# ------------------------
${LIBRARY_IPMISOL_DEVICE}: ${LIBRARY_IPMISOL_DEVICE_OBJECT_FILES} ${IPBUS_REG_HELPER_PATH}/lib/libBUTool_IPBusIO.so ${LIBRARY_IPMISOL}
	g++ ${LINK_LIBRARY_FLAGS} -lBUTool_IPMISOL ${LIBRARY_IPMISOL_DEVICE_OBJECT_FILES} -o $@
	@echo "export BUTOOL_AUTOLOAD_LIBRARY_LIST=\$$BUTOOL_AUTOLOAD_LIBRARY_LIST:$$PWD/${LIBRARY_IPMISOL_DEVICE}" > env.sh

${LIBRARY_IPMISOL}: ${LIBRARY_IPMISOL_OBJECT_FILES} ${IPBUS_REG_HELPER_PATH}/lib/libBUTool_IPBusIO.so
	g++ ${LINK_LIBRARY_FLAGS}  ${LIBRARY_IPMISOL_OBJECT_FILES} -lipmiconsole -o $@



#	g++ ${LINK_LIBRARY_FLAGS} ${UHAL_LIBRARY_FLAGS} ${UHAL_LIBRARIES} $^ -o $@

#${LIBRARY_IPMISOL_DEVICE_OBJECT_FILES}: obj/%.o : src/%.cc
obj/%.o : src/%.cc
	mkdir -p $(dir $@)
	mkdir -p {lib,obj}
	g++ ${CPP_FLAGS} ${UHAL_CPP_FLAGHS} -c $< -o $@

-include $(LIBRARY_OBJECT_FILES:.o=.d)

