CC=gcc
APP=beorn

LIBS= 

ASM_LINKS=

FILES= ./src/bvm.c \
       ./src/btypes.c \
       ./src/bparser.c \
       ./src/bfunctions.c \
       ./src/bprint.c \
	   ./src/bflags.c \
	   ./src/bio.c \
       ./src/beorn.c

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
		-ledit \
		-Wall  \
		$(WARNING) \
		$(DWN_CCFLAGS) \
		$(LIBS) \
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
