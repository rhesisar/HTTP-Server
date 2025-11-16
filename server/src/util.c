#include <stdio.h>
#include <stdlib.h>

void *
emalloc(size_t size)
{
	void *p = NULL;

	if ((p = malloc(size)) == NULL) {
		perror("malloc");
		exit(1);
	}
	return p;
}

void
error(char *err)
{
	perror(err);
	exit(EXIT_FAILURE);
}
