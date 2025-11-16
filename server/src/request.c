#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "request.h"

#ifndef BACKLOG
#define BACKLOG MAXCLIENT
#endif

static int listen_fd = -1;

static int
init_server(short int port)
{
	struct sockaddr_in addr;
	int opt = 1;

	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_fd < 0) {
		perror("socket");
		return -1;
	}

	if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))
		< 0) {
		perror("setsockopt");
		// not fatal in practice, keep going
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons((unsigned short)port);

	if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind");
		close(listen_fd);
		listen_fd = -1;
		return -1;
	}

	if (listen(listen_fd, BACKLOG) < 0) {
		perror("listen");
		close(listen_fd);
		listen_fd = -1;
		return -1;
	}

	return 0;
}

// Little helper to find the end of HTTP headers: \r\n\r\n
static int
headers_complete(const char *buf, size_t len)
{
	if (len < 4)
		return 0;

	for (size_t i = 0; i <= len - 4; ++i) {
		if (buf[i] == '\r'
			&& buf[i + 1] == '\n'
			&& buf[i + 2] == '\r'
			&& buf[i + 3] == '\n') {
			return 1;
		}
	}
	return 0;
}

message *
getRequest(short int port)
{
	int client_fd;
	struct sockaddr_in client_addr;
	socklen_t addrlen = sizeof(client_addr);

	// First call: create listening socket.
	if (listen_fd < 0) {
		if (init_server(port) < 0) {
			return NULL;
		}
	}

	// Wait for a client connection.
	client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &addrlen);
	if (client_fd < 0) {
		perror("accept");
		return NULL;
	}

	// Read one HTTP request from the client.
	size_t cap = 4096;
	size_t len = 0;
	char *buf = (char *)malloc(cap);
	if (!buf) {
		perror("malloc");
		close(client_fd);
		return NULL;
	}

	// We only need to support GET/HEAD; reading until \r\n\r\n is enough.
	while (!headers_complete(buf, len)) {
		ssize_t n = recv(client_fd, buf + len, cap - len, 0);
		if (n < 0) {
			// read error
			perror("recv");
			free(buf);
			close(client_fd);
			return NULL;
		}
		if (n == 0) {
			// client closed connection before sending full request
			free(buf);
			close(client_fd);
			return NULL;
		}

		len += (size_t)n;

		// grow buffer if needed
		if (len + 1 >= cap) {
			cap *= 2;
			char *nbuf = (char *)realloc(buf, cap);
			if (!nbuf) {
				perror("realloc");
				free(buf);
				close(client_fd);
				return NULL;
			}
			buf = nbuf;
		}
	}

	// Null-terminate for convenience
	buf[len] = '\0';

	message *m = (message *)malloc(sizeof(message));
	if (!m) {
		perror("malloc message");
		free(buf);
		close(client_fd);
		return NULL;
	}

	m->buf = buf;
	m->len = (unsigned int)len;
	// clientId = socket fd
	m->clientId = (unsigned int)client_fd;

	m->clientAddress = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
	if (!m->clientAddress) {
		perror("malloc clientAddress");
		free(buf);
		free(m);
		close(client_fd);
		return NULL;
	}
	*(m->clientAddress) = client_addr;

	return m;
}

void
freeRequest(message *r)
{
	if (!r)
		return;

	if (r->buf)
		free(r->buf);
	if (r->clientAddress)
		free(r->clientAddress);
	free(r);
}

// Simple implementation using writeDirectClient on r->clientId.
void
sendReponse(message *r)
{
	if (!r)
		return;

	int fd = (int)r->clientId;
	size_t off = 0;

	while (off < r->len) {
		ssize_t n = send(fd, r->buf + off, r->len - off, 0);
		if (n <= 0)
			break;
		off += (size_t)n;
	}
}

// Experimental streaming write: send directly to the socket fd (clientId).
void
writeDirectClient(int i, char *buf, unsigned int len)
{
	int fd = i; // clientId == socket fd in this implementation.
	size_t off = 0;

	while (off < len) {
		ssize_t n = send(fd, buf + off, len - off, 0);
		if (n <= 0)
			break;
		off += (size_t)n;
	}
}

// Nothing buffered: nothing special to do here.
// We keep it to satisfy the original API.
void
endWriteDirectClient(int i)
{
	(void)i;
	// no-op
}

// Ask to close the TCP connection for this client.
void
requestShutdownSocket(int i)
{
	int fd = i; // clientId == socket fd
	if (fd < 0)
		return;

	shutdown(fd, SHUT_RDWR);
	close(fd);
}
