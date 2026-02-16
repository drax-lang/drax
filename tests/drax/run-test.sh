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

echo "----------------------------------------"

echo -e "\n\033[32mAll tests were executed successfully!\033[0m"

# ./bin/drax ./tests/drax/http/http-only-response-text-plain.dx
