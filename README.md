# HTTP Server

Minimal HTTP/1.1 static file server with a formal HTTP parser.

Supports `GET`/`HEAD`, virtual hosts via `Host`, MIME detection with `libmagic`, and optional PHP execution through `FastCGI` (`php-fpm` on `127.0.0.1:9000`).

## What it does
- **Parses and validates requests**: method, request-target, version (`HTTP/1.0` / `HTTP/1.1`), `Host` (required on 1.1), `Connection`, `Content-Length`.
- **Normalizes paths**: percent-decoding + dot-segment removal to avoid traversal, then maps to a vhost directory.
- **Virtual hosts**: serves files from `SITES_FOLDER/<host>/<target>`.
- **Defaults**: empty path --> `index.html` (configurable).
- **Content types**: detected via `libmagic`.
- **HEAD support**: same headers as `GET`, no body.
- **PHP via FastCGI**: `.php` requests are sent to `php-fpm` and the generated HTML is served.

## Build & Run
```
cd server
make          # builds ./http-server (links libparser + librequest + libmagic)
make run      # exports LD_LIBRARY_PATH and starts the server in the background
# ...
make stop     # stops the server
make clean
```

## Default configuration

- Port: 8080 (`server/src/conf.h` --> `PORT`)
- Document root: `/var/www` (`SITES_FOLDER`)
- Default file: `index.html` (`DFLT_TARG`)
- Virtual hosts: edit `server/src/conf.c` (e.g., `site1.fr`, `www.toto.com`, ...)
- Tip: create directories like `/var/www/site1.fr/` and drop `index.html` inside.

## Usage examples
### `GET`
`curl -i http://localhost:8080/`

### `HEAD`
`curl -I http://localhost:8080/assets/app.css`

### Targeting a specific vhost (matches `server/src/conf.c`)
`curl -H 'Host: site1.fr' http://localhost:8080/`

### PHP (requires `php-fpm` listening on `127.0.0.1:9000`)
`curl -H 'Host: site1.fr' http://localhost:8080/info.php`

## Project layout
```
server/
  Makefile               # build, run (sets LD_LIBRARY_PATH), stop
  README.md              # (old minimal readme)
  src/
    httpserver.c         # socket loop, response building, file serving
    semantics.c,h        # HTTP semantics: method/host/connection/version/target checks
    conf.c,h             # PORT, SITES_FOLDER, DFLT_TARG, vhost list
    content_type.c,h     # libmagic-based MIME detection
    phptohtml.c,h        # FastCGI client --> php-fpm (127.0.0.1:9000), emits tmp.html
    fastcgi.h            # FastCGI protocol structs/utilities
    util.c,h             # emalloc(), error(), helpers
    api.h, httpparser.h  # parser API interfaces
  api/
    libparser/           # HTTP ABNF parser (shared lib, prebuilt .so)
    librequest-0.5/      # request I/O helpers (shared lib, headers)
parser/
  Makefile               # build test binary (parser)
  README.txt             # how to run parser tests
  allrfc.abnf            # ABNF grammar
  src/                   # syntax tree + API
  tests/                 # 10,000 fuzzed HTTP samples (test0.txt ... test9999.txt)
```

## Notes
Separation of concerns: ABNF parser (library) --> semantics layer --> server/IO.

Robustness: fail-fast errors, strict header/grammar checks, consistent Content-Length.

Security-minded: percent-decoding and dot-segment removal before filesystem access, vhost-scoped doc roots.

Minimal deps: single make build, requires `libmagic` (`file/libmagic` package). PHP is optional.

## Configuration quick refs
Change port/root/default file: `server/src/conf.h`

Edit vhosts: `server/src/conf.c`

MIME detection toggles: `server/src/content_type.c`

PHP result file: `server/src/phptohtml.h` (`PHP_RESULT_FILE`)
