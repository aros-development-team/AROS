/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$
    
    Code to handle list of strings
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stringlist.h"

struct stringlist *slist_append(struct stringlist **list, const char *s)
{
    struct stringlist **listit = list;
    struct stringlist *node = malloc(sizeof(struct stringlist));
    
    if (node == NULL)
    {
	fprintf(stderr, "Out of memory!\n");
	exit(20);
    }
    
    while (*listit != NULL) listit = &(*listit)->next;
    
    *listit = node;
    node->next = NULL;
    node->s = strdup(s);
    if (node->s == NULL)
    {
	fprintf(stderr, "Out of memory!\n");
	exit(20);
    }

    return node;
}

struct stringlist *slist_prepend(struct stringlist **list, const char *s)
{
    struct stringlist **listit = list;
    struct stringlist *node = malloc(sizeof(struct stringlist));
    
    if (node == NULL)
    {
	fprintf(stderr, "Out of memory!\n");
	exit(20);
    }
    
    node->next = *listit;
    node->s = strdup(s);
    if (node->s == NULL)
    {
	fprintf(stderr, "Out of memory!\n");
	exit(20);
    }
    *listit = node;

    return node;
}

int slist_remove(struct stringlist **list, struct stringlist *node)
{
    struct stringlist **listit = list;

    while(*listit != NULL && *listit != node) listit = &(*listit)->next;
    
    if (*listit == node)
    {
	*listit = (*listit)->next;
	free(node->s);
	free(node);
	return 1;
    }
    else
	return 0;
}
