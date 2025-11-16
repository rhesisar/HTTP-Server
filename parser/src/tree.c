#include <stdlib.h>
#include <string.h>

#include "tree.h"
#include "util.h"

static void appendTokenList(_Token **l, _Token *app);

Node *root;

void
createnode(Node **n, char *rulename, char *val, int len, Node *child,
		   Node *sibling)
{
	*n = emalloc(sizeof(Node));
	(*n)->rulename = rulename;
	(*n)->val = val;
	(*n)->len = len;
	(*n)->child = child;
	(*n)->sibling = sibling;
}

_Token *
searchNodes(Node *start, char *rulename)
{
	_Token *l, *childList, *siblingList;

	l = childList = siblingList = NULL;

	if (start == NULL)
		return NULL;
	if (!rulename || !strcmp(start->rulename, rulename)) {
		l = emalloc(sizeof(_Token));
		l->node = start;
		l->next = NULL;
	}
	childList = searchNodes(start->child, rulename);
	siblingList = searchNodes(start->sibling, rulename);
	appendTokenList(&l, childList);
	appendTokenList(&l, siblingList);
	return l;
}

static void
appendTokenList(_Token **l, _Token *app)
{
	_Token *cur;

	if (app == NULL)
		return;
	if (*l == NULL)
		*l = app;
	else {
		cur = *l;
		while (cur->next != NULL) {
			cur = cur->next;
		}
		cur->next = app;
	}
}

char *
getNodeRulename(Node *node, int *len)
{
	if (len != NULL)
		*len = strlen(node->rulename);
	return node->rulename;
}

char *
getNodeVal(Node *node, int *len)
{
	if (len != NULL)
		*len = node->len;
	return node->val;
}

void
freeList(_Token **r)
{
	_Token *cur;

	cur = *r;

	while (cur != NULL) {
		*r = (*r)->next;
		free(cur);
		cur = *r;
	}
}

void
freeTree(Node *root)
{
	if (root == NULL)
		return;
	freeTree(root->child);
	freeTree(root->sibling);
	free(root);
}
