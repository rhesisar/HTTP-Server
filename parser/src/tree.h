#ifndef _TREE_H_
#define _TREE_H_

#include "api.h"

typedef struct node {
	char *rulename;
	char *val;
	int len;
	struct node *child;
	struct node *sibling;
} Node;

extern struct node *root;

void createnode(Node **n, char *rulename, char *val, int len, Node *child,
				Node *sibling);
_Token *searchNodes(Node *start, char *rulename);
char *getNodeRulename(Node *node, int *len);
char *getNodeVal(Node *node, int *len);
void freeList(_Token **r);
void freeTree(Node *root);

#endif