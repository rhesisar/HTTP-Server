#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "httpparser.h"
#include "api.h"
#include "semantics.h"
#include "util.h"
#include "conf.h"

static void initreq(Request *req);
static int method(Request *req, _Token *root);
static int request_target(Request *req, _Token *root);
static int http_version(Request *req, _Token *root);
static int host(Request *req, _Token *root);
static int content_length(Request *req, _Token *root);
static int connection(Request *req, _Token *root);

void static pct_normalize(char *target);
void static remove_dot_segments(char *target);

char * const methods[] = {
	[GET] = "GET",
	[HEAD] = "HEAD"
};
char *const versions[] = {
	[HTTP1_0] = "HTTP/1.0",
	[HTTP1_1] = "HTTP/1.1"
};
char *const connections[] = {
	[KEEP_ALIVE] = "keep-alive",
	[CLOSE] = "close"
};

Request *
semantics(_Token *root)
{
	Request *req;

	req = emalloc(sizeof(Request));
	initreq(req);
	req->host = -1;

	if (method(req, root)
	|| request_target(req, root)
	|| http_version(req, root)
	|| host(req, root)
	|| content_length(req, root)
	|| connection(req, root))
		return req;
	return req;
}


static void
initreq(Request *req)
{
	req->host = -1;
	req->target = NULL;
	req->status = 200;
	req->connection = CLOSE;
}


static int
method(Request *req, _Token *root)
{
	int i;
	_Token *tok;
	Node mthd;

	if ((tok = searchTree(root, "method")) == NULL) {
		req->status = 400;
		return 1;
	}
	mthd.value = getElementValue(tok->node, &mthd.len);
	for (i = 0; i < N_METHODS; i++) {
		if (!strncmp(mthd.value, methods[i], mthd.len)) {
			req->method = i;
			purgeElement(&tok);
			return 0;
		}
	}
    req->status = 501;
	purgeElement(&tok);
	return 1;
}


static int
request_target(Request *req, _Token *root)
{
	_Token *tok;
	Node target;

	if ((tok = searchTree(root, "absolute_path")) == NULL) {
		req->status = 400;
		return 1;
	}
	target.value = getElementValue(tok->node, &target.len);
	req->target = emalloc((target.len + 1) * sizeof(char));
	strncpy(req->target, target.value, target.len);
	req->target[target.len] = '\0';
	pct_normalize(req->target);
	remove_dot_segments(req->target);
	purgeElement(&tok);
	return 0;
}


static int
http_version(Request *req, _Token *root)
{
	_Token *tok;
	Node version;

	if ((tok = searchTree(root, "HTTP_version")) == NULL) {
		req->status = 400;
		return 1;
	}
	version.value = getElementValue(tok->node, &version.len);
	if (!strncmp(version.value, versions[HTTP1_0], version.len)) {
		req->version = HTTP1_0;
	} else if (!strncmp(version.value, versions[HTTP1_1], version.len)) {
		req->version = HTTP1_1;
	} else {
		req->status = 505;
		return 1;
	}
	purgeElement(&tok);
	return 0;
}


static int
host(Request *req, _Token *root)
{
	_Token *tok;
	Node host;
	int i;

	if (((tok = searchTree(root, "host")) == NULL && req->version == HTTP1_1)
	|| tok->next != NULL) {
		req->status = 400;
		return 1;
	}
	purgeElement(&tok);

	if ((tok = searchTree(root, "reg_name")) != NULL) {
		host.value = getElementValue(tok->node, &host.len);
		for (i = 0; i < N_HOSTS; i++) {
			if (!strncmp(host.value, hosts[i], host.len)) {
				req->host = i;
				break;
			}
		}
	}
	purgeElement(&tok);
	return 0;
}


static int
content_length(Request *req, _Token *root)
{
	_Token *tok;
	Node coding, length1, length2;

	if ((tok = searchTree(root, "transfer_coding"))) {
		coding.value = getElementValue(tok->node, &coding.len);
		if (strncmp(coding.value, CHUNKED, coding.len)) {
			req->status = 400;
			return 1;
		}
		if ((tok = searchTree(root, "Content_Length"))) {
			req->status = 400;
			return 1;
		}
		purgeElement(&tok);
	} else {
		if ((tok = searchTree(root, "Content_Length"))) {
			while (tok->next != NULL) {
				length1.value = getElementValue(tok->node, &length1.len);
				length2.value = getElementValue(tok->node, &length2.len);
				if (length1.len != length2.len
				|| strncmp(length1.value, length2.value, length1.len)) {
					req->status = 400;
					return 1;
				}
				tok = tok->next;
			}
			purgeElement(&tok);
		}
	}
	return 0;
}


static int
connection(Request *req, _Token *root)
{
	_Token *tok;
	Node connection;
	int i;

	if ((tok = searchTree(root, "connection_option"))) {
		do {
			connection.value = getElementValue(tok->node, &connection.len);
			if (connection.len == strlen(connections[CLOSE])) {
				for (i = 0; i < connection.len; i++) {
					if (tolower(connection.value[i]) != connections[CLOSE][i])
						break;
				}
				if (i == connection.len) {
					req->connection = CLOSE;
					break;
				}
			} else if (connection.len == strlen(connections[KEEP_ALIVE])) {
				for (i = 0; i < connection.len; i++) {
					if (tolower(connection.value[i]) != connections[KEEP_ALIVE][i])
						break;
				}
				if (i == connection.len) {
					req->connection = KEEP_ALIVE;
					break;
				}
			}
			tok = tok->next;
		}
		while (tok->next != NULL);
		purgeElement(&tok);
	}
	if (req->connection != CLOSE && req->version == HTTP1_1) {
		req->connection = KEEP_ALIVE;
	}
	if (req->connection != CLOSE && req->connection != KEEP_ALIVE) {
		req->connection = CLOSE;
	}
	req->status = 200;
	return 0;
}


static void
pct_normalize(char *target)
{
	int i;
	char buf[3];
	int len;

	buf[2] = '\0';
	len = strlen(target);

	/* While room for pct-encoded */
	for (i = 0; i + 2 < len; i++) {
		if (target[i] == '%'
		&& isxdigit(target[i + 1])
		&& isxdigit(target[i + 2])) {
			/* Format string for strtol */
			strncpy(buf, target + i + 1, 2);
			target[i] = strtol(buf, NULL, 16);
			/* Move target to left */
			memmove(target + i + 1, target + i + 3, len - i - 3);
			len -= 2;
			target[len] = '\0';
		}
	}
}


/* See RFC 3986 Section 5.2.4.*/
static void
remove_dot_segments(char *target)
{
	int i, j;
	char *buf;
	int len;

	len = strlen(target);
	buf = emalloc(len * sizeof(char));

	buf[0] = '\0';
	i = j = 0;
	while (i < len) {
		/* A */
		if (i <= len - strlen("../")
		&& target[i] == '.'
		&& target[i + 1] == '.'
		&& target[i + 2] == '/') {
			i += strlen("../");
		} else if (i <= len - strlen("./")
		&& target[i] == '.'
		&& target[i + 1] == '/') {
			i += strlen("./");
		/* B */
		} else if (i <= len - strlen("/./")
		&& target[i] == '/'
		&& target[i + 1] == '.'
		&& target[i + 2] == '/') {
			i += strlen("/.");
		} else if (i <= len - strlen("/.")
		&& target[i] == '/'
		&& target[i + 1] == '.'
		&& i + 2 == len ) {
			i += strlen("/");
			target[i + 1] = '/';
		/* C */
		} else if (i <= len - strlen("/../")
		&& target[i] == '/'
		&& target[i + 1] == '.'
		&& target[i + 2] == '.'
		&& target[i + 3] == '/') {
			i += strlen("/..");
			if (j != 0) {
				while (buf[j] != '/') {
					j--;
				}
			}
		} else if (i <= len - strlen("/..")
		&& target[i] == '/'
		&& target[i + 1] == '.'
		&& target[i + 2] == '.'
		&& i + 3 == len) {
			i += strlen("/.");
			target[i + 2] = '/';
			if (j != 0) {
				while (buf[j] != '/') {
					j--;
				}
			}
		/* D */
		} else if ( i <= len - strlen(".")
		&& target[i] == '.'
		&& i + 1 == len){
			i += strlen(".");
		} else if (i <= len - strlen("..")
		&& target[i] == '.'
		&& target[i + 1] == '.'
		&& i + 2 == len) {
			i += strlen("..");
		/* E */
		} else {
			do {
				buf[j] = target[i];
				j++;
				i++;
			} while (i < len && target[i] != '/');
		}
	}
	/* Replace by interpreted string */
	memmove(target, buf + 1, j - 1);
	free(buf);
	target[j - 1] = '\0';
}
