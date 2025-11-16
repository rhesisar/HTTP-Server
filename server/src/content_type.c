#include <magic.h>
#include <stdio.h>
#include <string.h>

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
	size_t ext_len = strlen(".css");
	if (len >= ext_len && !strcmp(filename + len - ext_len, ".css")) {
		return strdup("text/css; charset=us-ascii");
	}

	if ((magic_cookie = magic_open(MAGIC_MIME)) == NULL) {
		error("magic_open");
	}
	if (magic_load(magic_cookie, NULL) != 0) {
		magic_close(magic_cookie);
		error("magic_load");
	}
	magic_full = magic_file(magic_cookie, filename);
	r = strdup(magic_full);
	magic_close(magic_cookie);
	return r;
}
