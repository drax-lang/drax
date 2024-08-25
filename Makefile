CC=gcc
APP=drax

## libcivetweb
HTTP_LIB_NAME = libcivetweb.a
HTTP_LIB=./src/http
HTTP_LIB_FLAGS=-DUSE_WEBSOCKET -DUSE_IPV6 -lcrypto -lssl -DUSE_SSL_DH=1

LIBS= -I$(HTTP_LIB)/include $(HTTP_LIB_NAME)

## Asm

ASM_LINKS=

FILES= ./src/dvm.c \
       ./src/dtypes.c \
       ./src/dparser.c \
       ./src/dshell.c \
       ./src/dflags.c \
       ./src/dio.c \
       ./src/dlex.c \
       ./src/drax.c \
       ./src/dhandler.c \
       ./src/dstructs.c \
       ./src/dbuiltin.c \
       ./src/dtime.c \
	   ./src/dstring.c \
	   ./src/dlist.c \
	   ./src/dgc.c \
	   ./src/deval.c \
	   ./src/dscheduler.c \
	   ./src/mods/d_mod_os.c \
	   ./src/mods/d_mod_http.c

DEBUGF= -ggdb \
		-g

OUTDIR=./bin/

OUTBIN= -o $(OUTDIR)$(APP)

ifeq ($(TARGET_OS),)
  ifeq ($(OS),Windows_NT)
    TARGET_OS = WIN32
  else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        TARGET_OS = LINUX
    else
        ifeq ($(UNAME_S),Darwin)
            TARGET_OS = OSX
        else
            TARGET_OS = BSD
        endif
    endif
  endif
endif

ifeq ($(TARGET_OS),LINUX)
	LIBS += -ldl -lrt
endif

ifdef WITH_ZLIB
	LIBS += -lz
endif

WARNING= \
		-Wextra \
		-Wundef \
		-Wshadow \
		-Wfatal-errors \
		-Wsign-compare \
		-Wwrite-strings \
		-Wredundant-decls \
		-Wdouble-promotion \
		-Wmissing-declarations \
		-Wdisabled-optimization \

FLAGS=  -std=c99 \
		-lm \
		-Wall  \
		-ansi \
		-lpthread \
		$(WARNING) \
		$(DWN_CCFLAGS) \
		$(LIBS) \
		$(HTTP_LIB_FLAGS) \
		$(OUTBIN)

ifdef NO_SSL
  FLAGS += -DNO_SSL
endif

ifeq ($(LIGHT), 1)
    DRAX_BUILD_FULL= 
else
    DRAX_BUILD_FULL= \
	    -D_B_BUILF_FULL \
	    -ledit
endif

DEFAULT_BUILD = \
		$(CC) \
		$(FILES) \
		$(ASM_LINKS) \
		$(DRAX_BUILD_FULL) \
		$(FLAGS)

$(HTTP_LIB_NAME):
	$(MAKE) -C $(HTTP_LIB) WITH_IPV6=1 WITH_WEBSOCKET=1 COPT='-DNO_SSL_DL=1' clean lib
	 cp $(HTTP_LIB)/$(HTTP_LIB_NAME) .

all: $(HTTP_LIB_NAME)
	$(DEFAULT_BUILD)

debug: $(HTTP_LIB_NAME)
	$(DEFAULT_BUILD) $(DEBUGF)

inspect: $(HTTP_LIB_NAME)
	$(DEFAULT_BUILD) $(DEBUGF) -D_AST_INSPECT

run:
	./bin/$(APP)

config:
	mkdir bin

test:
	sh tests/run-test.sh

clean:
	rm -rf ./bin/$(APP)
