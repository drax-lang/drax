#!/bin/bash
# HTTP server tests: a real server (server.dx) and real clients (curl, raw sockets).
#
#   bash tests/drax/http/run-http-tests.sh        (after make)
#
# Skipped (exit 0) when curl or bash /dev/tcp are not available.

HERE=$(cd "$(dirname "$0")" && pwd)
DRAX="${DRAX:-$HERE/../../../bin/drax}"

if ! command -v curl >/dev/null 2>&1; then echo "http tests skipped: curl not found"; exit 0; fi

T=$(mktemp -d "${TMPDIR:-/tmp}/drax-http.XXXXXX") || exit 1
SRV_PID=""
cleanup() {
  [ -n "$SRV_PID" ] && kill "$SRV_PID" 2>/dev/null
  rm -rf "$T"
}
trap cleanup EXIT INT TERM

PORT=$((20000 + RANDOM % 20000))
URL="http://127.0.0.1:$PORT"
FAILS=0
N=0

ok()   { N=$((N + 1)); }
fail() { N=$((N + 1)); FAILS=$((FAILS + 1)); printf 'FAIL: %s\n' "$1"; }
is()   { # is "name" expected actual
  if [ "$2" = "$3" ]; then ok; else fail "$1 (expected '$2', got '$3')"; fi
}
has()  { # has "name" file pattern
  if grep -q -- "$3" "$2"; then ok; else fail "$1 (no '$3' in: $(head -c 200 "$2" | tr '\r\n' '  '))"; fi
}

# raw TCP: send stdin, print everything until the server closes (or 3 s)
raw() {
  exec 3<>"/dev/tcp/127.0.0.1/$PORT" || return 1
  cat >&3
  timeout 3 cat <&3
  exec 3>&-
}

# ---- start the server
HTTP_TEST_PORT=$PORT timeout 120 "$DRAX" "$HERE/server.dx" > "$T/server.log" 2>&1 &
SRV_PID=$!
for _ in $(seq 1 50); do
  curl -s -m 1 "$URL/" >/dev/null 2>&1 && break
  sleep 0.1
done

# ---- basics
is "GET /" "hello" "$(curl -s -m 5 "$URL/")"
is "status line and content-length" "200 5" "$(curl -s -m 5 -o /dev/null -w '%{http_code} %{size_download}' "$URL/")"
is "404" "404" "$(curl -s -m 5 -o /dev/null -w '%{http_code}' "$URL/nope")"
is "method, query and header" "GET a=1&b=2 hi" "$(curl -s -m 5 -H 'X-Test: hi' "$URL/info?a=1&b=2")"
is "repeated headers are joined" "GET  a, b" "$(curl -s -m 5 -H 'X-Test: a' -H 'X-Test: b' "$URL/info")"
is "POST" "POST  none" "$(curl -s -m 5 -X POST -d 'x' "$URL/info")"

# ---- keep-alive: the second request reuses the connection
is "keep-alive reuses the connection" "1 0" "$(curl -s -m 5 -o /dev/null -o /dev/null -w '%{num_connects} ' "$URL/" "$URL/" | tr -s ' ' ' ' | sed 's/ $//')"

# ---- bodies
is "small body" "len=5" "$(curl -s -m 5 -d 'hello' "$URL/len")"
head -c 102400 /dev/zero | tr '\0' 'a' > "$T/100k"
is "body bigger than one read (100 KB)" "len=102400" "$(curl -s -m 5 --data-binary @"$T/100k" "$URL/len")"
head -c 900000 /dev/zero | tr '\0' 'b' > "$T/900k"
is "body of 900 KB" "len=900000" "$(curl -s -m 5 --data-binary @"$T/900k" "$URL/len")"
is "Expect: 100-continue" "len=10240" "$(head -c 10240 /dev/zero | tr '\0' c | curl -s -m 5 -H 'Expect: 100-continue' --data-binary @- "$URL/len")"

# binary bodies (NUL bytes) survive in both directions
printf 'a\0b\0\377c\r\n\0' > "$T/bin"
curl -s -m 5 --data-binary @"$T/bin" "$URL/echo" > "$T/bin.out"
if cmp -s "$T/bin" "$T/bin.out"; then ok; else fail "binary echo"; fi
is "binary length" "len=9" "$(curl -s -m 5 --data-binary @"$T/bin" "$URL/len")"

# ---- limits and malformed requests
is "body above the limit: 413" "413" "$(head -c 1500000 /dev/zero | curl -s -m 10 -o /dev/null -w '%{http_code}' --data-binary @- "$URL/len")"
is "header above the limit: 431" "431" "$(curl -s -m 5 -o /dev/null -w '%{http_code}' -H "X-Big: $(head -c 20000 /dev/zero | tr '\0' a)" "$URL/")"
printf 'GARBAGE\r\n\r\n' | raw > "$T/r1"; has "garbage: 400" "$T/r1" "HTTP/1.1 400"
printf 'GET / HTTP/1.1\nHost: x\n\n' | raw > "$T/r2"; has "bare LF: 400 (no wait)" "$T/r2" "HTTP/1.1 400"
printf 'POST / HTTP/1.1\r\nContent-Length: 3\r\nContent-Length: 4\r\n\r\nabc' | raw > "$T/r3"; has "two different content-length: 400" "$T/r3" "HTTP/1.1 400"
printf 'POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n0\r\n\r\n' | raw > "$T/r4"; has "chunked: 501" "$T/r4" "HTTP/1.1 501"

# ---- partial requests
exec 3<>"/dev/tcp/127.0.0.1/$PORT"
printf 'GET / HT' >&3; sleep 0.2
printf 'TP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n' >&3
timeout 3 cat <&3 > "$T/r5"; exec 3>&-
has "request in pieces" "$T/r5" "hello"

# ---- pipelining: two requests in one write
printf 'GET / HTTP/1.1\r\nHost: x\r\n\r\nGET /nope HTTP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n' | raw > "$T/r6"
is "pipelining: both answered" "1 1" "$(grep -c 'HTTP/1.1 200' "$T/r6") $(grep -c 'HTTP/1.1 404' "$T/r6")"

# ---- a client that leaves in the middle of a big answer must not kill the server.
# Two pipelined requests: the answer of the first one fails (connection reset),
# and the second one is written to a socket that is already dead: that write
# is a SIGPIPE for a program that does not ignore it.
for _ in 1 2 3; do
  exec 3<>"/dev/tcp/127.0.0.1/$PORT"
  printf 'GET /big HTTP/1.1\r\nHost: x\r\n\r\nGET / HTTP/1.1\r\nHost: x\r\n\r\n' >&3
  head -c 100 <&3 > /dev/null
  exec 3>&-
  sleep 0.3
done
is "alive after clients that left (SIGPIPE)" "hello" "$(curl -s -m 5 "$URL/")"

# ---- an idle connection only holds the server until the timeout (700 ms)
exec 4<>"/dev/tcp/127.0.0.1/$PORT"
START=$(date +%s)
R=$(curl -s -m 8 "$URL/")
ELAPSED=$(( $(date +%s) - START ))
exec 4>&-
is "idle connection does not block forever" "hello" "$R"
if [ "$ELAPSED" -le 4 ]; then ok; else fail "idle connection held the server for ${ELAPSED}s"; fi

# ---- a closed client in the middle of a request
exec 3<>"/dev/tcp/127.0.0.1/$PORT"; printf 'GET / HT' >&3; exec 3>&-
is "alive after a half request" "hello" "$(curl -s -m 5 "$URL/")"

# ---- errors in the language (not crashes)
cat > "$T/e1.dx" <<EOF
server = TCPServer.new({ host: '127.0.0.1', port: '$PORT' })
EOF
"$DRAX" "$T/e1.dx" > "$T/e1.out" 2>&1; E1=$?
has "port in use is an error" "$T/e1.out" "fail to bind"
if [ "$E1" -ne 0 ]; then ok; else fail "port in use: exit code"; fi

cat > "$T/e2.dx" <<EOF
server = TCPServer.new({ host: '127.0.0.1', port: '0' })
TCPServer.receive(server)
EOF
"$DRAX" "$T/e2.dx" > "$T/e2.out" 2>&1
has "a server is not a connection" "$T/e2.out" "Expected a connection"

cat > "$T/e3.dx" <<EOF
server = TCPServer.new({ host: '127.0.0.1', port: '0' })
conn = TCPServer.accept(1)
EOF
"$DRAX" "$T/e3.dx" > "$T/e3.out" 2>&1
has "accept needs a server" "$T/e3.out" "Expected a server"

cat > "$T/e4.dx" <<EOF
TCPServer.new({ port: 'abc' })
EOF
"$DRAX" "$T/e4.dx" > "$T/e4.out" 2>&1
has "invalid port" "$T/e4.out" "invalid port"

cat > "$T/e5.dx" <<EOF
print(Http.response(200, { 'x-a': "a\r\nSet-Cookie: evil=1" }, 'x'))
EOF
"$DRAX" "$T/e5.dx" > "$T/e5.out" 2>&1
has "header injection is refused" "$T/e5.out" "invalid header value"

cat > "$T/e6.dx" <<EOF
print(Http.response(99, {}, 'x'))
EOF
"$DRAX" "$T/e6.dx" > "$T/e6.out" 2>&1
has "invalid status is refused" "$T/e6.out" "invalid status"

cat > "$T/e7.dx" <<EOF
r = Http.response(204, {}, nil)
print(String.length(r))
EOF
is "204 has no content-length (27 bytes)" "27" "$("$DRAX" "$T/e7.dx" 2>&1 | tr -d '\n')"

# ---- the server stops cleanly
is "quit" "bye" "$(curl -s -m 5 "$URL/quit")"
for _ in $(seq 1 30); do kill -0 "$SRV_PID" 2>/dev/null || break; sleep 0.1; done
if kill -0 "$SRV_PID" 2>/dev/null; then fail "the server did not stop"; else ok; fi
has "server log" "$T/server.log" "server stopped"
SRV_PID=""

printf 'http: %d checks, %d failed\n' "$N" "$FAILS"
[ "$FAILS" -eq 0 ]
