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

LIBRARY_PATH = \
							-Llib \
							-L$(BUTOOL_PATH)/lib \

ifdef BOOST_INC
INCLUDE_PATH +=-I$(BOOST_INC)
endif
ifdef BOOST_LIB
LIBRARY_PATH +=-L$(BOOST_LIB)
endif

LIBRARIES =    	-lToolException	\
		-lboost_regex




CPP_FLAGS = -std=c++11 -g -O3 -rdynamic -Wall -MMD -MP -fPIC ${INCLUDE_PATH} -Werror -Wno-literal-suffix

CPP_FLAGS +=-fno-omit-frame-pointer -Wno-ignored-qualifiers -Werror=return-type -Wextra -Wno-long-long -Winit-self -Wno-unused-local-typedefs  -Woverloaded-virtual

LINK_LIBRARY_FLAGS = -shared -fPIC -Wall -g -O3 -rdynamic ${LIBRARY_PATH} ${LIBRARIES} -Wl,-rpath=${PackagePath}/lib



.PHONY: all _all clean _cleanall build _buildall

default: build
clean: _cleanall
_cleanall:
	rm -rf obj
	rm -rf bin
	rm -rf lib


all: _all
build: _all
buildall: _all
_all: ${LIBRARY_IPMISOL_DEVICE} ${LIBRARY_IPMISOL}

${LIBRARY_IPMISOL_DEVICE}: ${LIBRARY_IPMISOL_DEVICE_OBJECT_FILES} ${LIBRARY_IPMISOL}
	g++ ${LINK_LIBRARY_FLAGS} -lBUTool_IPMISOL ${LIBRARY_IPMISOL_DEVICE_OBJECT_FILES} -o $@
	@echo "export BUTOOL_AUTOLOAD_LIBRARY_LIST=\$$BUTOOL_AUTOLOAD_LIBRARY_LIST:$$PWD/${LIBRARY_IPMISOL_DEVICE}" > env.sh

${LIBRARY_IPMISOL}: ${LIBRARY_IPMISOL_OBJECT_FILES}
	g++ ${LINK_LIBRARY_FLAGS}  ${LIBRARY_IPMISOL_OBJECT_FILES} -lipmiconsole -o $@



#	g++ ${LINK_LIBRARY_FLAGS} $^ -o $@

#${LIBRARY_IPMISOL_DEVICE_OBJECT_FILES}: obj/%.o : src/%.cc
obj/%.o : src/%.cc
	mkdir -p $(dir $@)
	mkdir -p {lib,obj}
	g++ ${CPP_FLAGS} -c $< -o $@

-include $(LIBRARY_OBJECT_FILES:.o=.d)

