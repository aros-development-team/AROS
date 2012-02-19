/*
    Copyright © 2008 The AROS Development Team. All rights reserved.
    $Id$

    Desc: Test AVL balanced tree interface.
    Lang: english
*/
#define  DEBUG  1
#include <aros/debug.h>

#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/exall.h>
#include <dos/datetime.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <utility/tagitem.h>
#include <utility/utility.h>

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static const char version[] = "$VER: avltest.c 45.0 (25.2.2008)\n";

#define MAX(x, y) ((x)>(y) ? (x) : (y))
#define RCOUNT (100000)

struct testnode {
	struct AVLNode node;

	int key;
};

void *mempool;
struct AVLNode *root;
int *shuffle;

AROS_UFH2S(LONG, nodecmp_int,
	  AROS_UFHA(const struct AVLNode *, avlnode1,  A0),
	  AROS_UFHA(const struct AVLNode *, avlnode2,  A1))
{
	AROS_USERFUNC_INIT;

	const struct testnode *n1 = (const struct testnode *)avlnode1;
	const struct testnode *n2 = (const struct testnode *)avlnode2;

	return n1->key - n2->key;

	AROS_USERFUNC_EXIT;
}

static LONG nodecmp_int2(const struct AVLNode * avlnode1, const struct AVLNode *avlnode2)
{
	const struct testnode *n1 = (const struct testnode *)avlnode1;
	const struct testnode *n2 = (const struct testnode *)avlnode2;

	fflush(stdout);

	return n1->key - n2->key;
}

AROS_UFH2S(LONG, keycmp_int,
	  AROS_UFHA(const struct AVLNode *, avlnode,  A0),
	  AROS_UFHA(AVLKey,		    avlkey,   A1))
{
	AROS_USERFUNC_INIT;

	int key = (int)(IPTR)avlkey;
	const struct testnode *n = (const struct testnode *)avlnode;

	return n->key - key;

	AROS_USERFUNC_EXIT;
}


static int checkbalance(struct AVLNode *root)
{
    int left, right;

    if (root == NULL)
	return 0;

    left = checkbalance(root->avl_link[0]);
    right = checkbalance(root->avl_link[1]);

    if (right- left != root->avl_balance) {	    
	printf("** Tree not balanced at %p\n", root);
	DeletePool(mempool);
	exit(1);

    }

    return MAX(left, right)+1;
}

static void dumporder(struct AVLNode *root)
{
    struct AVLNode *scan = AVL_FindFirstNode(root);

    printf("keys in order:\n");
    while (scan != NULL) {
	struct testnode *s = (struct testnode *)scan;

	printf(" %d\n", s->key);
	scan = AVL_FindNextNodeByAddress(scan);
    }
}

static void dumprorder(struct AVLNode *root)
{
    struct AVLNode *scan = AVL_FindLastNode(root);

    printf("keys in reverse order:\n");
    while (scan != NULL) {
	struct testnode *s = (struct testnode *)scan;

	printf(" %d\n", s->key);
	scan = AVL_FindPrevNodeByAddress(scan);
    }
}

struct testnode *newnode()
{
    struct testnode *node = AllocPooled(mempool, sizeof(struct testnode));

    if (node == NULL) {
	fprintf(stdout, "AllocPooled failed\n");
	fflush(stdout);
	DeletePool(mempool);
	free(shuffle);
	exit(EXIT_FAILURE);
    }

    return node;
}

struct AVLNode *resetTree()
{
    if (mempool)
	DeletePool(mempool);
    mempool = CreatePool(0, sizeof(struct testnode)*32, sizeof(struct testnode)*32);

    if (mempool == NULL) {
	fprintf(stdout, "CreatePool failed\n");
	fflush(stdout);
	exit(EXIT_FAILURE);
    }

    return NULL;
}

int main(int argc, char **argv)
{
    int i;

    shuffle = malloc(sizeof(shuffle[0]) * RCOUNT);
    if (shuffle == NULL)
	return EXIT_FAILURE;

    root = resetTree();

    for (i =0;i<10;i++) {
	struct testnode *n;

	n = newnode();
	n->key = i;

	printf("Add node %d\n", i);
	AVL_AddNode(&root, &n->node, (AVLNODECOMP)nodecmp_int2);
    }

    dumporder(root);
    dumprorder(root);

    printf("remove by key\n");
    for (i = 0;i<10;i++) {
	AVL_RemNodeByKey(&root, (AVLKey)(IPTR)i, keycmp_int);
    }

    dumporder(root);
    dumprorder(root);

    printf("find next node by key\n");
    root = resetTree();
    for (i =0;i<20;i+=2) {
	struct testnode *n;

	n = newnode();
	n->key = i;

	printf("Add node %d\n", i);
	AVL_AddNode(&root, &n->node, nodecmp_int);
    }

    for (i =0;i<20;i++) {
	struct testnode *n = (struct testnode *)AVL_FindNextNodeByKey(root, (AVLKey)(IPTR)i, keycmp_int);
	int next;

	printf("next node %d = %d\n", i, n ? n->key : -1);

	if ((i % 2) == 0)
	    next = i;
	else
	    next = i + 1;

	if ((n != NULL && n->key != next)
	    || (n == NULL && i != 19))
	    printf("next node by key is wrong!  got %d expected %d\n", (n == NULL ? -1 : n->key), i+1);
    }

    root = resetTree();
    root = NULL;
    printf("insert reverse order\n");
    for (i =9;i>=0;i--) {
	struct testnode *n;

	n = newnode();
	n->key = i;

	AVL_AddNode(&root, &n->node, nodecmp_int);
    }

    dumporder(root);
    dumprorder(root);

    printf("remove by key\n");
    for (i = 0;i<10;i++) {
	AVL_RemNodeByKey(&root, (AVLKey)(IPTR)i, keycmp_int);
    }

    dumporder(root);
    dumprorder(root);

    // root should now be empty
    if (root != NULL) {
	printf("tree not empty!?");
    }

    root = resetTree();
    srandom(0);
    printf("insert random order\n");
    for (i = 0;i<RCOUNT;i++)
	shuffle[i] = i;
    for (i = 0;i<RCOUNT;i++) {
	int f = random() % RCOUNT;
	int t = random() % RCOUNT;
	int tmp;

	tmp = shuffle[f];
	shuffle[f] = shuffle[t];
	shuffle[t] = tmp;
    }
    //for (i=0;i<RCOUNT;i++) {
    //printf(" %d\n", shuffle[i]);
    //}

    for (i=0;i<RCOUNT;i++) {
	struct testnode *n;

	n = newnode();
	n->key = shuffle[i];

	AVL_AddNode(&root, &n->node, nodecmp_int);
    }
    
    //dumporder(root);
    //dumprorder(root);

    srandom(1);
    root = resetTree();

    printf("remove random order\n");
    for (i = 0;i<RCOUNT;i++) {
	struct testnode *n;

	shuffle[i] = i;

	n = newnode();
	n->key = i;

	AVL_AddNode(&root, &n->node, nodecmp_int);
	if ((i % 256) == 0)
	    checkbalance(root);
    }
    for (i = 0;i<RCOUNT;i++) {
	int f = random() % RCOUNT;
	int t = random() % RCOUNT;
	int tmp;
	
	tmp = shuffle[f];
	shuffle[f] = shuffle[t];
	shuffle[t] = tmp;
    }

    for (i=0;i<RCOUNT;i++) {
	AVL_RemNodeByKey(&root, (AVLKey)(IPTR)shuffle[i], keycmp_int);
	if ((i % 256) == 0)
	    checkbalance(root);
    }

    DeletePool(mempool);
    free(shuffle);

    return EXIT_SUCCESS;
}

