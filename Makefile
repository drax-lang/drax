CC=gcc
APP=beorn

LIBS= 

ASM_LINKS=

FILES= ./src/bvm.c \
       ./src/btypes.c \
       ./src/bparser.c \
       ./src/bfunctions.c \
       ./src/bprint.c \
       ./src/beorn.c

DEBUGF= -ggdb

OUTDIR=./bin/

OUTBIN= -o $(OUTDIR)$(APP)

FLAGS=  -std=c99        \
		-ledit          \
		-Wall           \
		-g              \
		$(DWN_CCFLAGS)  \
		$(LIBS)         \
		$(OUTBIN)

all:
	$(CC) $(FLAGS) $(FILES) $(ASM_LINKS)

debug:
	$(CC) $(FLAGS) $(DEBUGF) $(FILES) $(ASM_LINKS)

run:
	./bin/$(APP)

config:
	mkdir bin

clean:
	rm -rf ./bin/beorn
