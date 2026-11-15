#!/bin/bash
set -e

export MODE=test

echo "----------------------------------------"
echo -e "\n\033[32mRunning tests!\033[0m"
echo "----------------------------------------"

echo -e "\n\033[43mExpecting errors .....\033[0m"
./bin/drax ./tests/drax/expressions.dx
echo -e "\n\033[43m----------------------\033[0m"

./bin/drax ./tests/drax/print.dx

./bin/drax ./tests/drax/number.dx

./bin/drax ./tests/drax/functions.dx

./bin/drax ./tests/drax/string.dx

./bin/drax ./tests/drax/frames.dx

./bin/drax ./tests/drax/large_frames.dx

./bin/drax ./tests/drax/os.dx

./bin/drax ./tests/drax/list.dx

./bin/drax ./tests/drax/import.dx

./bin/drax ./tests/drax/math.dx

./bin/drax ./tests/drax/lambdas.dx

./bin/drax ./tests/drax/lambdas-nested.dx

./bin/drax ./tests/drax/pipe.dx

./bin/drax ./tests/drax/gc.dx

# the garbage must be collected while the program runs: limited memory.
# (not with sanitizers: they reserve a lot of virtual memory)
if [ -z "$DRAX_MEMCHECK" ]; then
  (ulimit -v 524288 && ./bin/drax ./tests/drax/gc_memory.dx)
else
  ./bin/drax ./tests/drax/gc_memory.dx
fi

./bin/drax ./tests/drax/TCO.dx

./bin/drax ./tests/drax/operators.dx

./bin/drax ./tests/drax/types.dx

./bin/drax ./tests/drax/closures.dx

./bin/drax ./tests/drax/recursion.dx

# every loop in this file must be a tail call: a low call depth proves it
DRAX_MAX_CALL_DEPTH=1000 ./bin/drax ./tests/drax/tail_calls.dx

./bin/drax ./tests/drax/edge_cases.dx

./bin/drax ./tests/drax/errors.dx

# print reaches a file or a pipe line by line, even if the process is killed
# (a server under a supervisor must not lose its log)
PRINT_OUT=$(mktemp)
timeout -s KILL 1 ./bin/drax ./tests/drax/print_flush.dx > "$PRINT_OUT" || true
grep -q "first line" "$PRINT_OUT" || { echo "print is not flushed when stdout is a file"; rm -f "$PRINT_OUT"; exit 1; }
rm -f "$PRINT_OUT"

# http server: real server and real clients (skipped without curl)
bash ./tests/drax/http/run-http-tests.sh

echo "----------------------------------------"

echo -e "\n\033[32mAll tests were executed successfully!\033[0m"

