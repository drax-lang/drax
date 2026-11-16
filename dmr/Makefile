# dmr - drax manager & resolver
#
# Standalone: nothing here links the Drax interpreter. To move dmr to its
# own repository, copy this directory; only `lang-sync` needs the sources of
# the interpreter (set DRAX_SRC) and it is skipped when they are missing.

CC ?= gcc
BIN = ./bin/dmr
DRAX_SRC ?= ../src

SRCS = src/main.c src/dfs.c src/dmanifest.c src/dlock.c src/dinstall.c src/dadd.c \
       src/dgraph.c src/dlang.c src/dlang_data.c

CFLAGS = -std=c99 -Wall -Wextra -Wshadow -Wundef -Wsign-compare -Wwrite-strings \
         -Wmissing-declarations -Wdouble-promotion

all: $(BIN)

$(BIN): $(SRCS) $(wildcard src/*.h)
	mkdir -p bin
	$(CC) $(CFLAGS) $(SRCS) -o $(BIN)

# regenerates the language table from the interpreter
lang-sync:
	sh tools/gen_lang.sh $(DRAX_SRC) > src/dlang_data.c

# fails when src/dlang_data.c is stale (skipped without the interpreter sources)
lang-check:
	@if [ -f $(DRAX_SRC)/dbuiltin.c ]; then \
		sh tools/gen_lang.sh $(DRAX_SRC) | cmp -s - src/dlang_data.c \
			|| { echo "src/dlang_data.c is stale: run make lang-sync"; exit 1; }; \
		echo "language table ok"; \
	else echo "language table check skipped (no $(DRAX_SRC))"; fi

test: all lang-check
	sh tests/run.sh

.PHONY: all lang-sync lang-check test
