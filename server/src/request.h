#ifndef _REQUEST_H_
#define _REQUEST_H_

#include <netinet/in.h>

#ifndef MAXCLIENT
#define MAXCLIENT 10
#endif

/* One HTTP request read from a client connection.
 * - clientId is the connected socket fd (used directly by write* functions)
 * - buf is NULL-terminated for convenience
 * - len is the number of bytes read (without the terminating NULL)
 * - clientAddress is allocated and filled by getRequest(); freeRequest() frees it
 */
typedef struct message {
    unsigned int           clientId;       /* socket fd of the client */
    char                  *buf;            /* request bytes (heap, NULL-terminated) */
    unsigned int           len;            /* number of valid bytes in buf */
    struct sockaddr_in    *clientAddress;  /* peer address (heap) */
} message;

/* Blocking accept + read of a single HTTP request on the given TCP port.
 * Returns a heap-allocated message on success, or NULL on error.
 * The first call lazily creates the listening socket.
 */
message *getRequest(short int port);

/* Free a message returned by getRequest(), including buf and clientAddress. */
void freeRequest(message *r);

/* Write the full r->buf to r->clientId */
void sendReponse(message *r);

/* Stream write bytes directly to the client socket */
void writeDirectClient(int i, char *buf, unsigned int len);

/* End-of-write hook to mirror historical APIs. No-op in this implementation. */
void endWriteDirectClient(int i);

/* Shutdown and close the client socket. Safe to call once per connection. */
void requestShutdownSocket(int i);

#endif /* _REQUEST_H_ */
