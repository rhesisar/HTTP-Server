# HTTP Server

Minimal HTTP/1.1 server and parser written in C.
The server/ component listens on TCP 8080, serves static files from /var/www, does simple virtual-host routing via the Host header (edit server/src/conf.{h,c}), detects MIME types with libmagic, and can run PHP via FastCGI.
The parser/ implements the HTTP ABNF grammar and includes 10k fuzz tests. Build with make in each subfolder.
