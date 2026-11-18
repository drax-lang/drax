#!/bin/sh
# dmr tests: everything runs offline, on temporary git repositories (file://).
#
#   sh tests/run.sh          (from the dmr directory, after make)
set -u

HERE=$(cd "$(dirname "$0")/.." && pwd)
DMR="$HERE/bin/dmr"
FIX="$HERE/tests/fixtures"
T=$(mktemp -d "${TMPDIR:-/tmp}/dmr-test.XXXXXX") || exit 1
trap 'rm -rf "$T"' EXIT INT TERM

export DMR_CACHE="$T/cache"
export GIT_TERMINAL_PROMPT=0
FAILS=0
N=0

ok()   { N=$((N + 1)); }
fail() { N=$((N + 1)); FAILS=$((FAILS + 1)); printf 'FAIL: %s\n' "$1"; }
check() { # check "name" command...
  name=$1; shift
  if "$@" >/dev/null 2>&1; then ok; else fail "$name"; fi
}
check_not() {
  name=$1; shift
  if "$@" >/dev/null 2>&1; then fail "$name"; else ok; fi
}
contains() { grep -q -- "$2" "$1"; }

g() { git -c user.name=t -c user.email=t@t "$@"; }

# a git repository with a package.toml and a main.dx
mkrepo() { # mkrepo dir name [dependencies...]
  d=$1; n=$2; shift 2
  mkdir -p "$d" && (
    cd "$d" && git init -q -b main . &&
    printf 'export({ name: "%s" })\n' "$n" > main.dx &&
    { printf '[package]\nname = "%s"\nversion = "1.0.0"\n' "$n"
      if [ $# -gt 0 ]; then printf '\n[dependencies]\n'; for x in "$@"; do printf '%s\n' "$x"; done; fi
    } > package.toml &&
    g add -A && g commit -q -m one
  )
}

R="$T/repos"
mkrepo "$R/json" json
(cd "$R/json" && g tag v1.0.0)
mkrepo "$R/http" http "json = { git = \"file://$R/json\", tag = \"v1.0.0\" }"
(cd "$R/http" && g tag v0.4.1)
mkdir -p "$T/core_utils" && printf 'export({ id: 1 })\n' > "$T/core_utils/main.dx"

mkproj() { # mkproj dir <dependency lines>
  d=$1; shift
  mkdir -p "$d"
  { printf '[package]\nname = "meu_servico"\nversion = "0.1.0"\n\n[dependencies]\n'
    for x in "$@"; do printf '%s\n' "$x"; done; } > "$d/package.toml"
}

P="$T/proj"
mkproj "$P" \
  "# comment" \
  "http = { git = \"file://$R/http\", tag = \"v0.4.1\" }   # trailing comment" \
  "core_utils = { path = \"../core_utils\" }"
mkdir -p "$P/lib" && printf 'export({ mine: 1 })\n' > "$P/lib/mine.dx"

# ---- install: git + transitive git + path, lock, user files untouched
"$DMR" -C "$P" install > "$T/out1" 2>&1
check "install exits 0" test $? -eq 0
check "git package downloaded"        test -f "$P/lib/http/main.dx"
check "transitive package downloaded" test -f "$P/lib/json/main.dx"
check "package without .git"          test ! -e "$P/lib/http/.git"
check "path dependency is a symlink"  test -L "$P/lib/core_utils"
check "path dependency resolves"      test -f "$P/lib/core_utils/main.dx"
check "user module untouched"         test -f "$P/lib/mine.dx"
check "lock exists"                   test -f "$P/package.lock"

HTTP_REV=$(git --git-dir="$R/http/.git" rev-parse 'v0.4.1^{commit}')
check "lock has the full commit"      contains "$P/package.lock" "rev = \"$HTTP_REV\""
check "lock has the request"          contains "$P/package.lock" 'requested = "tag:v0.4.1"'
check "lock has the dependencies"     contains "$P/package.lock" 'dependencies = \["json"\]'
check "lock is sorted"                sh -c "grep '^name' '$P/package.lock' | sort -c"
check "path source is relative"       contains "$P/package.lock" 'source = "path+../core_utils"'

# ---- second install: nothing to do, the lock is byte for byte the same
cp "$P/package.lock" "$T/lock1"
"$DMR" -C "$P" install > "$T/out2" 2>&1
check "second install exits 0"        test $? -eq 0
check "second install: up to date"    contains "$T/out2" "up to date"
check "lock is deterministic"         cmp -s "$P/package.lock" "$T/lock1"

# ---- the lock wins over a moved branch; update follows it
mkproj "$T/proj2" "json = { git = \"file://$R/json\", branch = \"main\" }"
"$DMR" -C "$T/proj2" install >/dev/null 2>&1
OLD=$(git --git-dir="$R/json/.git" rev-parse main)
check "branch pinned in the lock"     contains "$T/proj2/package.lock" "rev = \"$OLD\""
(cd "$R/json" && printf 'export({ name: "json2" })\n' > main.dx && g commit -q -am two)
NEW=$(git --git-dir="$R/json/.git" rev-parse main)
"$DMR" -C "$T/proj2" install >/dev/null 2>&1
check "install keeps the locked rev"  contains "$T/proj2/package.lock" "rev = \"$OLD\""
check "install keeps the old files"   contains "$T/proj2/lib/json/main.dx" '"json"'
"$DMR" -C "$T/proj2" update >/dev/null 2>&1
check "update moves to the new rev"   contains "$T/proj2/package.lock" "rev = \"$NEW\""
check "update changes the files"      contains "$T/proj2/lib/json/main.dx" 'json2'

# ---- a pinned commit
mkproj "$T/proj3" "json = { git = \"file://$R/json\", rev = \"$OLD\" }"
"$DMR" -C "$T/proj3" install >/dev/null 2>&1
check "rev dependency"                contains "$T/proj3/package.lock" "rev = \"$OLD\""

# ---- stale package is removed, user files are not
mkproj "$P" "core_utils = { path = \"../core_utils\" }"
"$DMR" -C "$P" install >/dev/null 2>&1
check "unused package removed"        test ! -e "$P/lib/http"
check "its dependency removed"        test ! -e "$P/lib/json"
check "path dependency kept"          test -L "$P/lib/core_utils"
check "user module still there"       test -f "$P/lib/mine.dx"
check "target of the path kept"       test -f "$T/core_utils/main.dx"

# ---- errors never write the lock and never touch unmanaged files
mkproj "$T/e1" 'json = "1.2.0"'
check_not "registry dependency is an error" "$DMR" -C "$T/e1" install
check "no lock on error"              test ! -e "$T/e1/package.lock"

mkproj "$T/e2" "x = { git = \"file://$R/json\", tag = \"nope\" }"
check_not "missing tag is an error"   "$DMR" -C "$T/e2" install
mkproj "$T/e3" "x = { git = \"file://$T/does-not-exist\" }"
check_not "missing repository"        "$DMR" -C "$T/e3" install
mkproj "$T/e4" '../evil = { path = "../core_utils" }'
check_not "invalid name"              "$DMR" -C "$T/e4" install
mkproj "$T/e5" 'x = { git = "--upload-pack=touch /tmp/pwned" }'
check_not "option injection in url"   "$DMR" -C "$T/e5" install
mkproj "$T/e6" "x = { git = \"file://$R/json\", tag = \"a\", branch = \"b\" }"
check_not "two refs"                  "$DMR" -C "$T/e6" install
mkproj "$T/e7" 'x = { path = "../a", git = "u" }'
check_not "two sources"               "$DMR" -C "$T/e7" install
mkproj "$T/e8" 'x = { path = "../core_utils"'
check_not "unterminated table"        "$DMR" -C "$T/e8" install
mkproj "$T/e9" 'x = { pth = "../core_utils" }'
check_not "unknown key"               "$DMR" -C "$T/e9" install
mkproj "$T/e10" 'dup = { path = "../core_utils" }' 'dup = { path = "../core_utils" }'
check_not "duplicated dependency"     "$DMR" -C "$T/e10" install
check_not "no package.toml"           "$DMR" -C "$T" install

mkproj "$T/u1" "mine = { path = \"../core_utils\" }"
mkdir -p "$T/u1/lib/mine" && printf 'x\n' > "$T/u1/lib/mine/own.dx"
check_not "refuses to replace an unmanaged dir" "$DMR" -C "$T/u1" install
check "unmanaged dir intact"          test -f "$T/u1/lib/mine/own.dx"

# conflict: the same name from two different sources
mkrepo "$R/a" a "json = { git = \"file://$R/json\", branch = \"main\" }"
mkproj "$T/c1" "a = { git = \"file://$R/a\" }" "json = { git = \"file://$R/json\", tag = \"v1.0.0\" }"
check_not "conflicting sources"       "$DMR" -C "$T/c1" install

# a directory with the name of an old package, but the user's, is not removed
mkproj "$T/k1" "core_utils = { path = \"../core_utils\" }"
"$DMR" -C "$T/k1" install >/dev/null 2>&1
unlink "$T/k1/lib/core_utils" && mkdir "$T/k1/lib/core_utils" && printf 'x\n' > "$T/k1/lib/core_utils/own.dx"
mkproj "$T/k1" 'zzz = { path = "../core_utils" }'
"$DMR" -C "$T/k1" install >/dev/null 2>&1
check "user dir with an old name is kept" test -f "$T/k1/lib/core_utils/own.dx"

# the bad lock never reaches the file system
mkproj "$T/l1" "core_utils = { path = \"../core_utils\" }"
mkdir -p "$T/keep" && printf 'x\n' > "$T/keep/f"
printf '[[package]]\nname = "../keep"\nsource = "path+x"\n' > "$T/l1/package.lock"
"$DMR" -C "$T/l1" install >/dev/null 2>&1
check "tampered lock: outside dir intact" test -f "$T/keep/f"

# ---- init
mkdir -p "$T/my app"
check "init creates package.toml"     "$DMR" -C "$T/my app" init
check "init writes a valid name"      contains "$T/my app/package.toml" 'name = "my_app"'
check_not "init does not overwrite"   "$DMR" -C "$T/my app" init
check "init manifest is installable"  "$DMR" -C "$T/my app" install
mkdir -p "$T/empty"
"$DMR" -C "$T/empty" install > "$T/empty.out" 2>&1
check "missing manifest suggests init" contains "$T/empty.out" "dmr init"

# ---- add
mkdir -p "$T/ad" && "$DMR" -C "$T/ad" init >/dev/null
check "add a git url with a tag"      "$DMR" -C "$T/ad" add "file://$R/json" --tag v1.0.0
check "add wrote the dependency"      contains "$T/ad/package.toml" "json = { git = \"file://$R/json\", tag = \"v1.0.0\" }"
check_not "add twice is an error"     "$DMR" -C "$T/ad" add "file://$R/json"
check "add a path"                    "$DMR" -C "$T/ad" add "../core_utils"
check "add path name from directory"  contains "$T/ad/package.toml" 'core_utils = { path = "../core_utils" }'
check "add --name and --branch"       "$DMR" -C "$T/ad" add "file://$R/http" --name web --branch main
check "add --name written"            contains "$T/ad/package.toml" "web = { git = \"file://$R/http\", branch = \"main\" }"
check_not "add: tag and branch"       "$DMR" -C "$T/ad" add "file://$R/a" --tag x --branch y
check_not "add: junk spec"            "$DMR" -C "$T/ad" add "nonsense"
check_not "add: option as spec"       "$DMR" -C "$T/ad" add "--upload-pack=x"
check "install after add works"       "$DMR" -C "$T/ad" install
check "installed json"                test -f "$T/ad/lib/json/main.dx"
check "installed web"                 test -f "$T/ad/lib/web/main.dx"
# the section is kept in place: a key after another section is not touched
mkdir -p "$T/ad2" && printf '[dependencies]\n\n[package]\nname = "x"\n' > "$T/ad2/package.toml"
"$DMR" -C "$T/ad2" add "file://$R/json" --tag v1.0.0 >/dev/null 2>&1
check "add keeps the dependencies section" "$DMR" -C "$T/ad2" install
# install <spec> adds and installs
mkdir -p "$T/ad3" && "$DMR" -C "$T/ad3" init >/dev/null
check "install <spec>"                "$DMR" -C "$T/ad3" install "file://$R/json" --tag v1.0.0
check "install <spec> installs"       test -f "$T/ad3/lib/json/main.dx"
# no manifest
mkdir -p "$T/ad4"
check_not "add without manifest"      "$DMR" -C "$T/ad4" add "file://$R/json"
# a manifest without the section gets one
mkdir -p "$T/ad5" && printf '[package]\nname = "x"\nversion = "1"\n' > "$T/ad5/package.toml"
check "add creates the section"       "$DMR" -C "$T/ad5" add "file://$R/json"
check "add section is valid"          "$DMR" -C "$T/ad5" install

# a failed install <spec> leaves package.toml as it was (offline: a file:// url)
mkdir -p "$T/ad6" && "$DMR" -C "$T/ad6" init >/dev/null && cp "$T/ad6/package.toml" "$T/ad6.before"
check_not "install <spec> that fails"  "$DMR" -C "$T/ad6" install "file://$T/does-not-exist"
check "failed install restores the manifest" cmp -s "$T/ad6/package.toml" "$T/ad6.before"
# a header with a trailing comment is still the [dependencies] section
mkdir -p "$T/ad7" && printf '[package]\nname = "x"\n\n[dependencies] # runtime\n' > "$T/ad7/package.toml"
"$DMR" -C "$T/ad7" add "file://$R/json" --tag v1.0.0 >/dev/null 2>&1
check "header with a comment: one section" test "$(grep -c '^\[dependencies\]' "$T/ad7/package.toml")" -eq 1
check "header with a comment: installs"    "$DMR" -C "$T/ad7" install
# the same source under another name is refused
check_not "same source, other name"   "$DMR" -C "$T/ad7" add "file://$R/json" --name j2 --tag v1.0.0

# ---- graph: golden file
cp -r "$FIX/graph" "$T/g" && mkdir -p "$T/g"
"$DMR" -C "$T/g" graph >/dev/null 2>&1
check "graph exits 0"                 test -f "$T/g/.drax-cpg/cpg.dgraph"
check "graph matches the golden file" diff -u "$FIX/graph.expected.cpg" "$T/g/.drax-cpg/cpg.dgraph"
cp "$T/g/.drax-cpg/cpg.dgraph" "$T/g1"
"$DMR" -C "$T/g" graph >/dev/null 2>&1
check "graph is deterministic"        cmp -s "$T/g/.drax-cpg/cpg.dgraph" "$T/g1"
# protected calls (f?(...)) and the safe pipe (|?>) are calls of f with the right arity
mkdir -p "$T/pc"
cat > "$T/pc/main.dx" <<'EOP'
fun add(a, b) do a + b end
fun use() do
    r = add?(1, 2)
    1 |?> add(2)
    1 |?> add?(2)
end
use()
EOP
"$DMR" -C "$T/pc" graph >/dev/null 2>&1
check "graph: f?() and |?> are calls"  test "$(grep -c '^c f2 f1 ' "$T/pc/.drax-cpg/cpg.dgraph")" -eq 3
# a symlink cycle must not hang the graph
mkdir -p "$T/cyc/sub" && printf 'x = 1\n' > "$T/cyc/sub/a.dx" && ln -s .. "$T/cyc/sub/loop"
timeout 20 "$DMR" -C "$T/cyc" graph >/dev/null 2>&1
check "symlink cycle terminates"      test $? -eq 0

check "lang graph has the modules"    contains "$T/g/.drax-cpg/lang.dgraph" '^mod List '
check "lang graph has precedence"     contains "$T/g/.drax-cpg/lang.dgraph" '^op.factor'

printf '%d checks, %d failed\n' "$N" "$FAILS"
[ "$FAILS" -eq 0 ]
