/*
	$Id$
*/

#include "listfunctions.h"

struct Node *getNumNode(struct List *list, int num) {
struct Node *node;

	node = list->lh_Head;
	while (num)
	{
		node = node->ln_Succ;
		num--;
	}
	return num ? 0 : node;
}

