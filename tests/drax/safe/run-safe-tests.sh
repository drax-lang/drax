#!/bin/bash
# Safe mode (--safe) and the permissions (--allow-*). Run from the root of the repository.
# Needs the extensions of tests/drax/ext (sh tests/drax/ext/build.sh).

DRAX=./bin/drax
D=tests/drax/safe

fail() {
  echo "safe mode test failed: $1"
  [ -n "$2" ] && echo "---- output ----" && echo "$2" && echo "----------------"
  rm -rf bin/safe_mkdir_test bin/safe_pmkdir_test
  exit 1
}

rm -rf bin/safe_mkdir_test bin/safe_pmkdir_test

# the default is everything allowed: nothing changes for a script
for f in run run_status system env net native; do
  out=$($DRAX $D/$f.dx 2>&1) || fail "$f.dx must work without --safe" "$out"
done
for f in mkdir pmkdir; do
  rm -rf bin/safe_mkdir_test bin/safe_pmkdir_test
  out=$($DRAX $D/$f.dx 2>&1) || fail "$f.dx must work without --safe" "$out"
done
rm -rf bin/safe_mkdir_test bin/safe_pmkdir_test

# with --allow-* and without --safe nothing changes either
out=$($DRAX --allow-run $D/run.dx 2>&1) || fail "--allow-run without --safe" "$out"

# --safe: what needs a permission fails and says which option allows it
denied() { # script, text that must be in the message, options...
  local file=$1 want=$2
  shift 2
  local out
  out=$($DRAX --safe "$@" $D/$file 2>&1) && fail "$file must fail in --safe mode ($*)" "$out"
  echo "$out" | grep -q -- "permission denied" || fail "$file: the message must say 'permission denied'" "$out"
  echo "$out" | grep -q -- "$want" || fail "$file: the message must say '$want'" "$out"
  echo "$out" | grep -q -- "safe mode" || fail "$file: the message must say 'safe mode'" "$out"
}

allowed() { # script, options...
  local file=$1
  shift
  local out
  out=$($DRAX --safe "$@" $D/$file 2>&1) || fail "$file must work with --safe $*" "$out"
}

denied run.dx        "Os.cmd: permission denied"
denied run.dx        "--allow-run"
denied run_status.dx "Os.cmd_with_status: permission denied"
denied run_status.dx "--allow-run"
denied system.dx     "Os.system: permission denied"
denied system.dx     "--allow-run"
denied env.dx        "Os.get_env: permission denied"
denied env.dx        "--allow-env"
denied mkdir.dx      "Os.mkdir: permission denied"
denied mkdir.dx      "--allow-fs"
denied pmkdir.dx     "Os.pmkdir: permission denied"
denied pmkdir.dx     "--allow-fs"
denied net.dx        "TCPServer.new: permission denied"
denied net.dx        "--allow-net"
denied native.dx     "Native.load: permission denied"
denied native.dx     "--allow-native"

[ ! -e bin/safe_mkdir_test ] || fail "Os.mkdir created a directory without the permission"
[ ! -e bin/safe_pmkdir_test ] || fail "Os.pmkdir created a directory without the permission"

# each option allows only its own permission
denied run.dx    "--allow-run" --allow-env --allow-net --allow-fs --allow-native
denied env.dx    "--allow-env" --allow-run --allow-net --allow-fs --allow-native
denied native.dx "--allow-native" --allow-run --allow-env --allow-net --allow-fs

allowed run.dx        --allow-run
allowed run_status.dx --allow-run
allowed system.dx     --allow-run
allowed env.dx        --allow-env
allowed net.dx        --allow-net
allowed mkdir.dx      --allow-fs
allowed pmkdir.dx     --allow-fs
allowed native.dx     --allow-native
rm -rf bin/safe_mkdir_test bin/safe_pmkdir_test

# the order of the options does not matter
allowed run.dx --allow-run
out=$($DRAX --allow-run --safe $D/run.dx 2>&1) || fail "--allow-run before --safe" "$out"

# --allow-all
allowed run.dx    --allow-all
allowed native.dx --allow-all
rm -rf bin/safe_mkdir_test bin/safe_pmkdir_test

# what needs no permission works
out=$($DRAX --safe $D/pure.dx 2>&1) || fail "pure.dx must work in --safe mode" "$out"

# native extensions: all of them, or only the ones in the list
allowed native.dx --allow-native=./bin/ext/greet.so
allowed native.dx --allow-native=bin/ext/greet.so
allowed native.dx --allow-native=./bin/ext/greet
allowed native.dx --allow-native="$PWD/bin/ext/greet.so"
allowed native.dx --allow-native=./bin/ext/other.so,./bin/ext/greet.so
allowed native.dx --allow-native=./bin/ext/other.so --allow-native=./bin/ext/greet.so

out=$($DRAX --safe --allow-native=./bin/ext/other.so $D/native.dx 2>&1) && fail "a library that is not in the list must be denied" "$out"
echo "$out" | grep -q "not allowed to load the native extension '$PWD/bin/ext/greet.so'" || fail "the denial must name the extension (absolute path)" "$out"
echo "$out" | grep -q -- "--allow-native=$PWD/bin/ext/greet.so" || fail "the denial must give the option that allows it" "$out"
echo "$out" | grep -q -- "(or --allow-native to allow every extension)" || fail "the denial must say how to allow every extension" "$out"

# the denial does not depend on the file existing (the answer must not tell what is on the disk)
cat > bin/safe_missing.dx <<'EOS'
r = Native.load?("./bin/ext/does_not_exist")
print(Error.message(r))
EOS
out=$($DRAX --safe bin/safe_missing.dx 2>&1) || fail "safe_missing.dx" "$out"
echo "$out" | grep -q "permission denied" || fail "a library that does not exist must be denied too, not reported as missing" "$out"
rm -f bin/safe_missing.dx

# a denial is a runtime error: f?() turns it into an Error
out=$($DRAX --safe $D/protected.dx 2>&1) || fail "protected.dx" "$out"

# the imported code runs with the same permissions
out=$($DRAX --safe $D/imports.dx 2>&1) || fail "imports.dx" "$out"

# wrong options say what is wrong and the status is 2
option_error() { # text that must be in the message, options...
  local want=$1
  shift
  local out status
  out=$($DRAX "$@" $D/pure.dx 2>&1)
  status=$?
  [ $status -eq 2 ] || fail "drax $* must end with status 2 (it was $status)" "$out"
  echo "$out" | grep -q -- "$want" || fail "drax $*: the message must say '$want'" "$out"
}

option_error "unknown option '--frobnicate'" --frobnicate
option_error "unknown option '--allow-ftp'" --safe --allow-ftp
option_error "The permissions are: --allow-run, --allow-net, --allow-env, --allow-fs, --allow-native" --allow-ftp
option_error "option '--allow-run' does not take a value" --safe --allow-run=1
option_error "option '--allow-native=' needs at least one path" --safe --allow-native=
option_error "option '--allow-native=' needs at least one path" --safe --allow-native=,

# an option after the script would be ignored, and a --safe that is ignored is not safe
out=$($DRAX $D/pure.dx --safe 2>&1)
status=$?
[ $status -eq 2 ] || fail "drax script.dx --safe must end with status 2 (it was $status)" "$out"
echo "$out" | grep -q "the option '--safe' is after the script and would be ignored" || fail "the message must say that --safe is after the script" "$out"
echo "$out" | grep -q "Options go before the script (example: drax --safe " || fail "the message must say where the option goes" "$out"
$DRAX $D/pure.dx --allow-run >/dev/null 2>&1 && fail "drax script.dx --allow-run must be refused"

# a script that is the argument of nothing else: a file name that is not an option is fine
out=$($DRAX $D/pure.dx 2>&1) || fail "pure.dx must work without options" "$out"

# a denial is the same through aliases, pipes and callbacks
out=$($DRAX --safe $D/alias.dx 2>&1) || fail "alias.dx" "$out"

# help
out=$($DRAX --help 2>&1) || fail "--help" "$out"
for want in "--safe" "--allow-run" "--allow-net" "--allow-env" "--allow-fs" "--allow-native" "--allow-all"; do
  echo "$out" | grep -q -- "$want" || fail "--help must describe $want" "$out"
done

echo "safe mode: all tests passed"
