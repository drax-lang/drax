CC=gcc
APP=drax

LIBS= 

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
	   ./src/mods/d_mod_os.c

DEBUGF= -ggdb \
		-g

OUTDIR=./bin/

OUTBIN= -o $(OUTDIR)$(APP)

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
		-Wall  \
		-ansi \
		$(WARNING) \
		$(DWN_CCFLAGS) \
		$(LIBS) \
		$(OUTBIN)

ifeq ($(LIGHT), 1)
    DRAX_BUILD_FULL= 
else
    DRAX_BUILD_FULL= \
	    -D_B_BUILF_FULL \
	    -ledit
endif

DEFAULT_BUILD = \
		$(CC) \
		$(FLAGS) \
		$(FILES) \
		$(ASM_LINKS) \
		$(DRAX_BUILD_FULL)

all:
	$(DEFAULT_BUILD)

debug:
	$(DEFAULT_BUILD) $(DEBUGF)

inspect:
	$(DEFAULT_BUILD) $(DEBUGF) -D_AST_INSPECT

run:
	./bin/$(APP)

config:
	mkdir bin

test:
	sh tests/run-test.sh

clean:
	rm -rf ./bin/$(APP)
