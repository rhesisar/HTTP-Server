#include <magic.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "util.h"

char *
file_content_type(char *filename)
{
	const char *magic_full;
	magic_t magic_cookie;
	char *r;

	/* MAGIC_MIME tells magic to return a mime of the file,
	   but you can specify different things	*/
	size_t len = strlen(filename);
	size_t css_len = strlen(".css");
	if (len >= css_len && !strcasecmp(filename + len - css_len, ".css")) {
		return strdup("text/css; charset=us-ascii");
	}
	size_t html_len = strlen(".html");
	if (len >= html_len && !strcasecmp(filename + len - html_len, ".html")) {
		return strdup("text/html; charset=us-ascii");
	}

	if ((magic_cookie = magic_open(MAGIC_MIME)) == NULL) {
		error("magic_open");
	}
	if (magic_load(magic_cookie, NULL) != 0) {
		magic_close(magic_cookie);
		error("magic_load");
	}
	if ((magic_full = magic_file(magic_cookie, filename)) == NULL) {
		magic_close(magic_cookie);
		return strdup("application/octet-stream; charset=binary");
	}
	r = strdup(magic_full);
	magic_close(magic_cookie);
	return r;
}
