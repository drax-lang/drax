#!/bin/sh
# Builds the extensions that the tests load, into bin/ext/ (run from the root of the repository).
set -e

OUT=bin/ext
mkdir -p "$OUT"

CC=${CC:-gcc}
CXX=${CXX:-g++}

case "$(uname -s)" in
  Darwin) SUFFIX=dylib ;;
  MINGW*|MSYS*|CYGWIN*) SUFFIX=dll ;;
  *) SUFFIX=so ;;
esac

for name in greet greet2 plain badabi noinit badinit clash badreg other newapi; do
  $CC -shared -fPIC -Wall -Wextra -std=c99 -I src -o "$OUT/$name.$SUFFIX" "tests/drax/ext/$name.c"
done

# C++ is optional: the test is skipped when there is no compiler
rm -f "$OUT/cppext.$SUFFIX"
if command -v "$CXX" >/dev/null 2>&1; then
  $CXX -shared -fPIC -Wall -Wextra -I src -o "$OUT/cppext.$SUFFIX" tests/drax/ext/cppext.cpp
fi
