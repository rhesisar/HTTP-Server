# HTTP/1.1 Server and Parser (C)

Small HTTP/1.1 stack in C composed of two parts:

- `server/` - single-process HTTP server with request parsing, semantic validation, static file serving, and `.php` execution through php-fpm (FastCGI).
- `parser/` - hand-written recursive-descent parser built from HTTP/1.1 ABNF, returning a structured request and utilities.

The code aims to be compact and explicit.

---

## Features

- C99, POSIX sockets, no external deps for the core server.
- Request line + headers parsing from ABNF (`parser/src/syntax.c`), exposed to the server via `server/src/httpparser.h`.
- Semantic checks (`server/src/semantics.c`): required `Host`, method and version, body rules, etc.
- Static file serving with extension to MIME mapping (`server/src/content_type.c`).
- `.php` support via a tiny FastCGI client that talks to php-fpm on `127.0.0.1:9000` (`server/src/phptohtml.c`).
- Code-defined virtual hosts in `server/src/conf.c` mapped to folders under `server/www/`. No external config files.

---

## Repository layout

```bash
server/
  src/
    httpserver.c        # accept loop and response writer
    request.c/.h        # request model
    semantics.c/.h      # HTTP validity rules
    content_type.c/.h   # file extension -> MIME
    phptohtml.c/.h      # minimal FastCGI client
    fastcgi.h           # FastCGI protocol structs
    conf.c/.h           # vhost table and constants
    util.c/.h           # helpers
    httpparser.h        # interface to parser module
  www/
    site1.fr/
      index.html
      hello.php
  Makefile
parser/
  src/
    api.c/.h            # parse entry points
    syntax.c/.h         # ABNF -> recursive-descent
    tree.c/.h           # simple AST helpers
    util.c/.h
    main.c              # dev driver
  tests/
  allrfc.abnf
  Makefile
```

---

## Build

```bash
# build the server
cd server
make

# optional: build only the parser module
cd ../parser
make
```

---

## Run the server

The repository already contains a default vhost and tiny docroot:

- Hostname: `site1.fr`
- Docroot: `server/www/site1.fr/` with `index.html` and `hello.php`

Start the binary and fetch a static file:

```bash
cd server
make run
curl -i -H "Host: site1.fr" http://127.0.0.1:8080/
```

Expected: `HTTP/1.1 200 OK` with the content of `index.html`.

---

## PHP through FastCGI (php-fpm)

Requirements:

- macOS: `brew install php && brew services start php` (php-fpm listens on `127.0.0.1:9000` by default)
- Debian/Ubuntu: `sudo apt install php-fpm`, set `listen = 127.0.0.1:9000` in the pool config, then restart the service

With php-fpm running:

```bash
curl -i -H "Host: site1.fr" http://127.0.0.1:8080/hello.php
```

Implementation detail: the FastCGI client writes php-fpm output to `/tmp/httpserver_php_result.html`, which the server streams back. Content type is inferred via `content_type.c`.

---

## Virtual hosts

Virtual hosts are defined in code.

1. Add or modify an entry in `server/src/conf.c` (hostname and local docroot).
2. If needed, adjust declarations in `server/src/conf.h`.
3. Create the matching folder under `server/www/` and rebuild.

Example:

```bash
# code change in conf.c adds: "www.example.com" -> "./www/www.example.com"
mkdir -p server/www/www.example.com
echo "example" > server/www/www.example.com/index.html

cd server && make
make run
curl -i -H "Host: www.example.com" http://127.0.0.1:8080/
```

---

## Quick checks

Local benchmark against a static file:

```bash
ab -n 2000 -c 50 http://127.0.0.1:8080/index.html
```

Rebuild with sanitizers:

```bash
cd server
make clean
CFLAGS="-fsanitize=address,undefined -O1 -fno-omit-frame-pointer -g" make
./http-server
```

---

## Troubleshooting

- `connect failed: Connection refused` during FastCGI: php-fpm not running or not listening on `127.0.0.1:9000`.
- `Primary script unknown` or `Status: 404 Not Found` from php-fpm: `SCRIPT_FILENAME` built in `phptohtml.c` does not point to an existing file under the selected vhost docroot. Check `conf.c` mapping and the file path.
- Empty `/tmp/httpserver_php_result.html`: the PHP script produced only headers or nothing on stdout; also verify write permissions on `/tmp`.

---

## Notes

- Single-process, blocking I/O, one request per connection.
- Minimal path and security handling. Not intended for public Internet exposure as-is.
- Extend MIME types in `content_type.c` as needed.
