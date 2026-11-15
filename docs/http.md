# HTTP server

`TCPServer` and `Http` build servers with code that reads from top to bottom:
every call blocks until it is done, and the next line runs after it. There are
no callbacks.

```
server = TCPServer.new({ port: '8080' })

fun serve(server) do
    conn = TCPServer.accept(server)           # waits for a client
    req = Http.read_request(conn)             # a whole request, as a frame

    if (req != nil) do
        TCPServer.send(conn, Http.response(200, { 'content-type': 'text/plain' }, 'hello'))
    end

    TCPServer.disconnect(conn)
    serve(server)                             # tail call: runs forever in constant memory
end

serve(server)
```

## The server and the connections are different values

`TCPServer.new` returns the **server**. `TCPServer.accept(server)` returns a
**new connection** for each client, with its own socket and its own buffer, so
many connections can be open at the same time (for example one per keep-alive
client). Using one where the other is expected is an error, not a crash:
`Expected a server as argument` / `Expected a connection as argument`.

Sockets are closed by `disconnect` / `close`, and also by the garbage
collector when the value is not used anymore.

## TCPServer

| Call | Result |
|------|--------|
| `TCPServer.new(options)` | the server, or an error (`fail to bind`, `invalid port`, ...) |
| `TCPServer.accept(server)` | a connection, blocks until a client arrives |
| `TCPServer.receive(conn)` | the bytes that arrived (one read, any bytes), or `nil` when the client left or the timeout ended |
| `TCPServer.send(conn, string)` | `true`, or `false` when the client is gone or too slow. Any bytes, the length is the string length. |
| `TCPServer.disconnect(conn)` | closes the connection |
| `TCPServer.close(server)` | stops listening |

### Options of `TCPServer.new`

Numbers can be given as numbers or as strings.

| Option | Default | |
|--------|---------|---|
| `port` | `'5000'` | `0` to `65535` (`0`: the system picks one) |
| `host` | every interface | an IPv4 address or a name; `'*'` and `'0.0.0.0'` mean every interface |
| `timeout_ms` | `10000` | read/write timeout of each connection, `0` for none |
| `backlog` | system maximum | connections waiting to be accepted |
| `max_header_bytes` | `16384` | request line + headers |
| `max_body_bytes` | `1048576` | request body (1 MB) |
| `reuse_port` | `false` | `SO_REUSEPORT`: several processes can listen on the same port |

An invalid option is an error: `server error, invalid option 'port'!`.

## Http

### `Http.read_request(conn)`

Reads one complete request (head and body, in as many reads as needed) and
returns a frame:

| Key | |
|-----|---|
| `method` | `"GET"` |
| `path` | `"/users"` (without the query) |
| `query` | `"a=1&b=2"`, `""` when there is none |
| `version` | `"1.1"` or `"1.0"` |
| `headers` | a frame with lower case names: `Frame.get_or_else(req.headers, 'content-type', '')` |
| `body` | a string (any bytes) |
| `keep_alive` | `true` when the connection can serve another request |

Repeated headers are joined with `", "`.

- It returns `nil` when the client closed the connection or the timeout ended.
- An invalid request returns `{ error: "...", status: 400 }` instead: answer it
  and disconnect. The `error` is `bad_request` (400), `header_too_large` (431),
  `body_too_large` (413) or `unsupported_transfer_encoding` (501).
- Several requests that arrive together (pipelining) are returned one by one.
- `Expect: 100-continue` is answered automatically.

### `Http.response(status, headers, body)`

Returns the text to send: status line, your headers, `Content-Length`
(always computed from the body) and the body.

```
Http.response(200, { 'content-type': 'application/json' }, '{"ok": true}')
Http.response(204, {}, nil)
```

It is an error to use an invalid status (`100` to `599`), a header name with
invalid characters, a header value with a line break (header injection), or a
body on `1xx`, `204` and `304`. `Content-Length` is not added to those statuses.

Keep-alive: `Http.response` does not add `Connection`; HTTP/1.1 keeps the
connection open by default. To close it, send `{ 'connection': 'close' }` and
disconnect.

## What it protects against

| | |
|---|---|
| Write on a client that left | `SIGPIPE` is ignored: `send` returns `false` and the server goes on |
| A slow or silent client | read and write timeout (`timeout_ms`) |
| Request smuggling | strict parsing: CRLF only, no NUL, no line folding, `Content-Length` only digits and never repeated with a different value, `Transfer-Encoding` refused |
| Big requests | limits for the head, the number of headers (100) and the body |
| Refused big body | the answer (`413`) reaches the client: the server reads and drops what the client still sends before closing |
| Header injection | `Http.response` refuses CR, LF and NUL in names and values |
| A failed `accept` (descriptors, aborted client) | the server stays open; the error is returned to the program |
| Binary data | the body is a string with a length: `NUL` bytes are kept |

## Because it is sequential

One program is one VM, and it answers one connection at a time. A client that
connects and says nothing holds the server until `timeout_ms`; keep it low
when the server is public. To use more cores today, start several processes
with `reuse_port: true`: the system spreads the connections among them.

## Not here yet

- HTTPS/TLS (put a proxy in front for now).
- `Transfer-Encoding: chunked` in requests (answered with `501`) and in responses.
- Serving many connections from one process without blocking (an event loop
  behind the same sequential calls) and worker processes started by the
  language itself.
- The Windows code path was updated with the same structure but was not compiled
  or run.

## Tests

`tests/c_tests/tests_http_parser.c` tests the parser alone, and
`tests/drax/http/run-http-tests.sh` runs a real server
(`tests/drax/http/server.dx`) against `curl` and raw sockets. Both run in
`make test` (the second is skipped without `curl`).
