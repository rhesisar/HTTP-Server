#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "fastcgi.h"
#include "phptohtml.h"
#include "util.h"

static int createSocket(int port);
static size_t readSocket(int fd, char *buf, size_t len);
static void readData(int fd, FCGI_Header *h, size_t *len);
static void writeSocket(int fd, FCGI_Header *h, unsigned int len);
static void writeLen(int len, char **p);
static int addNameValuePair(FCGI_Header *h, char *name, char *value);
static void sendBeginRequest(int fd, unsigned short requestId,
							 unsigned short role, unsigned char flags);

static const char *
find_body_start(const char *buf, size_t n)
{
	if (n < 4)
		return NULL;
	for (size_t i = 0; i + 3 < n; ++i) {
		if (buf[i] == '\r'
			&& buf[i + 1] == '\n'
			&& buf[i + 2] == '\r'
			&& buf[i + 3] == '\n')
			return buf + i + 4;
	}
	return NULL;
}

void
phptohtml(char *phpfile)
{
	int fd = -1;
	FILE *fpout = NULL;
	size_t len = 0;
	char abs_path[PATH_MAX];
	int saw_headers = 0;

	FCGI_Header h;
	printf("***BEGIN PHPTOHTML***\n");
	fd = createSocket(9000);
	sendBeginRequest(fd, 10, FCGI_RESPONDER, FCGI_KEEP_CONN);
	h.version = FCGI_VERSION_1;
	h.type = FCGI_PARAMS;
	h.requestId = htons(10);
	h.contentLength = 0;
	h.paddingLength = 0;

	/* php-fpm needs an absolute SCRIPT_FILENAME */
	if (!realpath(phpfile, abs_path)) {
		perror("realpath");
		if (fd >= 0)
			close(fd);
		return;
	}
	addNameValuePair(&h, "REQUEST_METHOD", "GET");
	addNameValuePair(&h, "SCRIPT_FILENAME", abs_path);
	addNameValuePair(&h, "QUERY_STRING", "");
	addNameValuePair(&h, "CONTENT_LENGTH", "0");
	addNameValuePair(&h, "CONTENT_TYPE", "");
	addNameValuePair(&h, "SERVER_PROTOCOL", "HTTP/1.1");
	addNameValuePair(&h, "GATEWAY_INTERFACE", "CGI/1.1");

	/* Send params record with content, then an empty params to terminate, then
	 * empty STDIN */
	writeSocket(
		fd, &h, FCGI_HEADER_SIZE + (h.contentLength) + (h.paddingLength));
	h.contentLength = 0;
	h.paddingLength = 0;
	writeSocket(fd,
				&h,
				FCGI_HEADER_SIZE
					+ (h.contentLength)
					+ (h.paddingLength)); /* FCGI_PARAMS end */
	h.type = FCGI_STDIN;
	writeSocket(fd,
				&h,
				FCGI_HEADER_SIZE
					+ (h.contentLength)
					+ (h.paddingLength)); /* FCGI_STDIN end */
	if ((fpout = fopen(PHP_RESULT_FILE, "wb")) == NULL) {
		perror("fopen PHP_RESULT_FILE");
		close(fd);
		return;
	}
	do {
		readData(fd, &h, &len);
		printf("read %ld\n", len);
		printf("version: %d\n", h.version);
		printf("type: %d\n", h.type);
		printf("requestId: %d\n", h.requestId);
		printf("length: %d\n", h.contentLength);
		printf("pad: %d\n", h.paddingLength);
		printf("data: %.*s\n", h.contentLength, h.contentData);

		if (h.type == FCGI_STDOUT && h.contentLength > 0) {
			const char *p = h.contentData;
			size_t n = (size_t)h.contentLength;
			if (!saw_headers) {
				const char *body = find_body_start(p, n);
				if (body) {
					saw_headers = 1;
					size_t body_len = (size_t)(p + n - body);
					if (body_len > 0)
						fwrite(body, 1, body_len, fpout);
				}
				/* If only headers arrived, wait for next chunk */
			} else {
				fwrite(p, 1, n, fpout);
			}
		}
		/* Ignore FCGI_STDERR and others for this POC */
	} while ((len != 0) && (h.type != FCGI_END_REQUEST));
	fflush(fpout);
	fclose(fpout);
	if (fd >= 0)
		close(fd);
	printf("***END PHPTOHTML***\n");
}

static size_t
readSocket(int fd, char *buf, size_t len)
{
	size_t readlen = 0;
	ssize_t nb = 0;

	if (len == 0)
		return 0;

	do {
		// try to read
		do {
			nb = read(fd, buf + readlen, len - readlen);
		} while (nb == -1 && errno == EINTR);
		if (nb > 0)
			readlen += nb;
	} while ((nb > 0) && (len != readlen));

	if (nb < 0)
		readlen = -1;
	return readlen;
}

static void
readData(int fd, FCGI_Header *h, size_t *len)
{
	size_t nb;
	*len = 0;

	nb = sizeof(FCGI_Header) - FASTCGILENGTH;
	if ((readSocket(fd, (char *)h, nb) == nb)) {
		h->requestId = htons(h->requestId);
		h->contentLength = htons(h->contentLength);
		*len += nb;
		nb = h->contentLength + h->paddingLength;
		if ((readSocket(fd, (char *)h->contentData, nb) == nb)) {
			*len += nb;
		} else {
			*len = 0;
		}
	}
}

static void
writeSocket(int fd, FCGI_Header *h, unsigned int len)
{
	int w;

	h->contentLength = htons(h->contentLength);
	h->paddingLength = htons(h->paddingLength);

	while (len) {
		// try to write
		do {
			w = write(fd, h, len);
		} while (w == -1 && errno == EINTR);
		len -= w;
	}
}

static void
writeLen(int len, char **p)
{
	if (len > 0x7F) {
		*((*p)++) = (len >> 24) & 0x7F;
		*((*p)++) = (len >> 16) & 0xFF;
		*((*p)++) = (len >> 8) & 0xFF;
		*((*p)++) = (len) & 0xFF;
	} else
		*((*p)++) = (len) & 0x7F;
}

static int
addNameValuePair(FCGI_Header *h, char *name, char *value)
{
	char *p;
	unsigned int nameLen = 0, valueLen = 0;

	if (name)
		nameLen = strlen(name);
	if (value)
		valueLen = strlen(value);

	if ((nameLen > FASTCGIMAXNVPAIR) || (valueLen > FASTCGIMAXNVPAIR))
		return -1;
	if ((h->contentLength
		 + ((nameLen > 0x7F) ? 4 : 1)
		 + ((valueLen > 0x7F) ? 4 : 1))
		> FASTCGILENGTH)
		return -1;

	p = (h->contentData) + h->contentLength;
	writeLen(nameLen, &p);
	writeLen(valueLen, &p);
	strncpy(p, name, nameLen);
	p += nameLen;
	if (value)
		strncpy(p, value, valueLen);
	h->contentLength += nameLen + ((nameLen > 0x7F) ? 4 : 1);
	h->contentLength += valueLen + ((valueLen > 0x7F) ? 4 : 1);
	return 0;
}

static void
sendBeginRequest(int fd, unsigned short requestId, unsigned short role,
				 unsigned char flags)
{
	FCGI_Header h;
	FCGI_BeginRequestBody *begin;

	h.version = FCGI_VERSION_1;
	h.type = FCGI_BEGIN_REQUEST;
	h.requestId = htons(requestId);
	h.contentLength = sizeof(FCGI_BeginRequestBody);
	h.paddingLength = 0;
	begin = (FCGI_BeginRequestBody *)&(h.contentData);
	begin->role = htons(role);
	begin->flags = flags;
	writeSocket(
		fd, &h, FCGI_HEADER_SIZE + (h.contentLength) + (h.paddingLength));
}

static int
createSocket(int port)
{
	int fd;
	struct sockaddr_in serv_addr;

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket creation failed\n");
		return (-1);
	}

	bzero(&serv_addr, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	inet_aton("127.0.0.1", &serv_addr.sin_addr);
	serv_addr.sin_port = htons(port);

	if (connect(fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		perror("connect failed\n");
		return (-1);
	}

	return fd;
}
