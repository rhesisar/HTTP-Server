#include <stddef.h>

#include "api.h"
#include "syntax.h"

void *
getRootTree()
{
	return root;
}

_Token *
searchTree(void *start, char *name)
{
	if (start == NULL)
		return searchNodes(getRootTree(), name);
	return searchNodes(start, name);
}

char *
getElementTag(void *node, int *len)
{
	return getNodeRulename(node, len);
}

char *
getElementValue(void *node, int *len)
{
	return getNodeVal(node, len);
}

void
purgeElement(_Token **r)
{
	freeList(r);
}

void
purgeTree(void *root)
{
	freeTree(root);
}

int
parseur(char *req, int len)
{
	char *s_end;

	s_end = req + len - 1;
	return !http_message(&req, s_end);
}
