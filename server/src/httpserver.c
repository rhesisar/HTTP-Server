#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>

// for librequest
#include "request.h"

// for parser
#include "httpparser.h" // this will declare internal type used by the parser
#include "api.h"

#include "semantics.h"
#include "content_type.h"
#include "util.h"
#include "conf.h"
#include "phptohtml.h"

#define CONNECTION "Connection: "
#define CONTENT_LENGTH "Content-Length: "
#define CONTENT_TYPE "Content-Type: "
#define DEFAULT_TYPE "application/octet-stream"
#define CRLF "\r\n"

static char * buildtarget(Request *req);

char * const status[] = {
	[200] = "HTTP/1.1 200 OK",
	[400] = "HTTP/1.1 400 Bad Request",
	[404] = "HTTP/1.1 400 Not Found",
	[501] = "HTTP/1.1 501 Not Implemented",
	[505] = "HTTP/1.1 505 HTTP Version Not Supported"
};

int main(int argc, char *argv[])
{
	while (1) {
		message *request = NULL;
		_Token *root = NULL;
		Request *req = NULL;
		int fi = -1;
		struct stat st;
		char *body = NULL;
		char *length = NULL;
		char *target = NULL;
		char *type = NULL;
	
		// On attend la reception d'une requete HTTP, request pointera vers une ressource allouée par librequest.
		printf("Waiting for request...\n");
		if ((request = getRequest(PORT)) == NULL)
			error("getRequest");

		// Affichage de debug
		printf("#########################################\nReceived request from client %d\n", request->clientId);
		printf("Client [%d] [%s:%d]\n", request->clientId, inet_ntoa(request->clientAddress->sin_addr), htons(request->clientAddress->sin_port));
		printf("Request is as follows:\n%.*s\n", request->len, request->buf);

		if (!parseur(request->buf, request->len)) {
			printf("Invalid request syntax\n");
			writeDirectClient(request->clientId, status[400], strlen(status[400]));
			writeDirectClient(request->clientId, CRLF, strlen(CRLF));
			writeDirectClient(request->clientId, CRLF, strlen(CRLF));
			endWriteDirectClient(request->clientId);
			printf("Closing connection\n");
			requestShutdownSocket(request->clientId);
			// Free the parse tree for this request
    		root = getRootTree();
    		purgeTree(root);
		} else {
			printf("Valid request syntax\n");
			root = getRootTree();
			req = semantics(root);
			printf("Valid request semantics\n");
			target = buildtarget(req);
			printf("Fetching requested resource: %s\n", target);
			/* Open file and save size */
			if ((fi = open(target, O_RDONLY)) == -1) {
					if (errno == EACCES) {
						req->status = 403;
					} else if (errno == ENOENT) {
				req->status = 404;
					} else {
						error("open target");
					}
        	} else {
				if (fstat(fi, &st) == -1) /* To obtain file size */
					error("fstat");
				if ((body = mmap(NULL, st.st_size, PROT_WRITE, MAP_PRIVATE, fi, 0)) == NULL )
					error("mmap");
			}
			printf("%.*s\n", (int)strlen(status[req->status]), status[req->status]);
			writeDirectClient(request->clientId, status[req->status], strlen(status[req->status]));
			writeDirectClient(request->clientId, CRLF, strlen(CRLF));
			/* Error */
			if (req->status != 200) {
				writeDirectClient(request->clientId, CRLF, strlen(CRLF));
				endWriteDirectClient(request->clientId);
				requestShutdownSocket(request->clientId);
			/* Valid */
			} else {
				printf("%s%.*s\n", CONNECTION, (int)strlen(connections[req->connection]), connections[req->connection]);
				writeDirectClient(request->clientId, CONNECTION, strlen(CONNECTION));
				writeDirectClient(request->clientId, connections[req->connection], strlen(connections[req->connection]));
				writeDirectClient(request->clientId, CRLF, strlen(CRLF));
				switch (req->method) {
				case GET:
					writeDirectClient(request->clientId, CONTENT_LENGTH, strlen(CONTENT_LENGTH));
					length = emalloc((int)log10(st.st_size) + 2);
						snprintf(length, 10, "%lld", (long long)st.st_size);
					printf("%s%.*s\n", CONTENT_LENGTH, (int)strlen(length), length);
					writeDirectClient(request->clientId, length, strlen(length));
					writeDirectClient(request->clientId, CRLF, strlen(CRLF));
					free(length);

					writeDirectClient(request->clientId, CONTENT_TYPE, strlen(CONTENT_TYPE));
					type = content_type(target);
					printf("%s%.*s\n", CONTENT_TYPE, (int)strlen(type), type);
					writeDirectClient(request->clientId, type, strlen(type));
					writeDirectClient(request->clientId, CRLF, strlen(CRLF));

					writeDirectClient(request->clientId, CRLF, strlen(CRLF));
					writeDirectClient(request->clientId, body, st.st_size);
					endWriteDirectClient(request->clientId);
					break;
				case HEAD:
					writeDirectClient(request->clientId, CRLF, strlen(CRLF));
					endWriteDirectClient(request->clientId);
					break;
				default:
					if (req->connection == CLOSE) {
						printf("Closing connection.\n");
						requestShutdownSocket(request->clientId);
					}
					}
					if (body && st.st_size > 0)
						munmap(body, st.st_size);
					if (fi != -1)
						close(fi);
				}
			}
			purgeTree(root);
		}
		// on ne se sert plus de request a partir de maintenant, on peut donc liberer...
		freeRequest(request);
		if (req)
		free(req);
		if (type)
		free(type);
		if (target)
		free(target);
	}
	error("HTTP server ended unexpectedly");
}


static char *
buildtarget(Request *req)
{
	int i, j, k;
	char *target;

	if (req->host == -1) {
		req->host = DFLT_HOST;
	}
	if (req->target[0] == '\0') {
		free(req->target);
		req->target = strdup(DFLT_TARG);
	}
	target = emalloc(strlen(SITES_FOLDER) + strlen("/") + strlen(hosts[req->host]) + strlen("/") + strlen(req->target) + 1);
	for (i = 0; i < strlen(SITES_FOLDER); i++) {
		target[i] = SITES_FOLDER[i];
	}
	target[i++] = '/';
	for (j = 0; j < strlen(hosts[req->host]); j++) {
		target[i + j] = hosts[req->host][j];
	}
	target[i + j] = '/';
	j++;
	for (k = 0; k < strlen(req->target); k++) {
		target[i + j + k] = req->target[k];
	}
	target[i + j + k] = '\0';

	if (!strcmp(target + strlen(target) - strlen(".php"), ".php")) {
		printf("php file detected: %s", target);
        phptohtml(target);
		free(target);
		target = strdup(PHP_RESULT_FILE);
    }
	free(req->target);
	return target;
}
