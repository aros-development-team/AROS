/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#include "misc.h"

typedef struct setnode
{
    char *secname;
    int   off_setname;
    long  pri;
    struct setnode *next;
} setnode;

static setnode *new_setnode(const char *name, setnode *next, int off, long pri){
   setnode *n;

   if (!(n          = calloc(1, sizeof(setnode))) ||
       !(n->secname = strdup(name))
      )
   {
	fatal("new_setnode()", strerror(errno));
   }

   n->off_setname = off;
   n->pri = pri;
   n->next = next;

   return n;
}

static setnode *get_setnode(setnode **list, const char *name, int off, long pri)
{
    setnode **curr = list;

    while (*curr)
    {
	if (strcmp((*curr)->secname, name) == 0)
	{
	    do
	    {
 	        if ((*curr)->pri == pri)
	            return *curr;

	        if ((*curr)->pri > pri)
	            break;

                curr = &(*curr)->next;

            } while (*curr && strcmp((*curr)->secname, name) == 0);

	    break;
	}

	curr = &(*curr)->next;
    }

    return (*curr = new_setnode(name, *curr, off, pri));
}

void emit_sets(setnode *setlist, FILE *out)
{
    char setname_big[201];
    int i;

    while (setlist)
    {
        setnode *oldnode = setlist;
        i = 0;

        do
        {
            setname_big[i] = toupper(setlist->secname[setlist->off_setname + i]);
        } while (setlist->secname[setlist->off_setname + i++]);

        fprintf
        (
            out,
            "    __%s_LIST__ = .;\n"
            "    LONG((__%s_END__ - __%s_LIST__) / %d - 2)\n",
	    setname_big, setname_big, setname_big, sizeof(long)
	);

	do
	{
	    fprintf
	    (
	        out,
		"    KEEP(*(%s.%ld))\n",
		setlist->secname, setlist->pri
	    );

	    setlist = setlist->next;
	} while (setlist && (strcmp(oldnode->secname, setlist->secname) == 0));


	fprintf
	(
	    out,
            "    KEEP(*(%s))\n"
            "    LONG(0)\n"
            "    __%s_END__ = .;\n",
            oldnode->secname, setname_big
        );
    }
}


void parse_secname(const char *secname, setnode **setlist_ptr)
{
    char  *idx;
    int    off;
    long   pri = 0;

    if (strncmp(secname, ".aros.set.", 10) == 0)
        off = 10;
    else
    if (strncmp(secname, ".ctors", 5) == 0)
        off = 1;
    else
    if (strncmp(secname, ".dtors", 5) == 0)
        off = 1;
    else
	return;

    idx = strchr(secname + off, '.');
    if (idx)
    {
        *idx = '\0';
	pri  = strtol(&idx[1], NULL, 10);
    }

    get_setnode(setlist_ptr, secname, off, pri);
}

