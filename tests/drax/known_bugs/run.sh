#!/bin/bash
# Runs the reproductions of known bugs. Each file asserts the CORRECT behavior,
# so a file that passes means the bug was fixed (move it to tests/drax/).
#
#   sh tests/drax/known_bugs/run.sh            report only, always exit 0
#   sh tests/drax/known_bugs/run.sh --strict   exit 1 if any bug is still present
#
# Files starting with "_" are helpers used by other reproductions.

export MODE=test
strict=0
[ "$1" = "--strict" ] && strict=1

open=0
fixed=0
for f in ./tests/drax/known_bugs/*.dx; do
  name=$(basename "$f")
  case "$name" in _*) continue ;; esac

  out=$(timeout 10 ./bin/drax "$f" 2>&1)
  code=$?

  if [ $code -eq 0 ]; then
    echo -e "\033[32mFIXED  \033[0m $name"
    fixed=$((fixed + 1))
  else
    reason="exit $code"
    [ $code -eq 124 ] && reason="hang (timeout)"
    [ $code -ge 128 ] && reason="crash (signal $((code - 128)))"
    echo -e "\033[31mOPEN   \033[0m $name  [$reason]"
    open=$((open + 1))
  fi
done

echo "----------------------------------------"
echo "$open known bug(s) still open, $fixed fixed"
[ $strict -eq 1 ] && [ $open -gt 0 ] && exit 1
exit 0
