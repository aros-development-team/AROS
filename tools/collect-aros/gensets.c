/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

typedef struct node
{
    char *secname;
    int   off_setname;
    struct node *next;
} node;



node *new_node(char *name, int off){
   node *n;

   if (!(n          = calloc(1, sizeof(node))) ||
       !(n->secname = strdup(name))
      )
   {
	perror("Internal Error - gensets");
	exit(1);
   }

   n->off_setname = off;

   return n;
}

node *get_node(node **list, char *name, int off)
{
    node **curr = list;

    while (*curr)
    {
	if (strcmp((*curr)->secname, name) == 0)
	    return *curr;

	curr = &((*curr)->next);
    }

    return (*curr = new_node(name, off));
}

int emit_sets(node *setlist, FILE *out)
{
    char setname_big[201];
    int i;

    while (setlist)
    {
        i = 0;

        do
        {
            setname_big[i] = toupper(setlist->secname[setlist->off_setname + i]);
        } while (setlist->secname[setlist->off_setname + i++]);

        fprintf
        (
            out,
            "    __%s_LIST__ = .;\n"
            "    LONG((__%s_END__ - __%s_LIST__) / %d - 2)\n"
            "    KEEP(*(SORT(%s.*)))\n"
            "    KEEP(*(%s))\n"
            "    LONG(0)\n"
            "    __%s_END__ = .;\n",
	    setname_big, setname_big, setname_big, sizeof(long),
            setlist->secname, setlist->secname, setname_big
        );

        setlist = setlist->next;
    }

    return 1;
}


/*
    This routine is slow, but does the work and it's the simplest to write down.
    All this will get integrated into the linker anyway, so there's no point
    in doing optimizations
*/

int gensets(FILE *in, FILE *out)
{
    char sec[201];
    node *setlist = NULL;

    while (fscanf(in, " %200s ", sec)>0)
    {
        char *idx;
	int   off;

	if (strncmp(sec, ".aros.set.", 10) == 0)
	    off = 10;
	else
	if (strncmp(sec, ".ctors", 5) == 0)
            off = 1;
	else
	if (strncmp(sec, ".dtors", 5) == 0)
            off = 1;
        else
	    continue;

        idx = strchr(sec + off, '.');
        if (idx)
            *idx = '\0';

	get_node(&setlist, sec, off);
    }

    return emit_sets(setlist, out);
}
