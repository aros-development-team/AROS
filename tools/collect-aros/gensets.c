/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct node
{
    char *name;
    struct node *next;
    union
    {
    	int pri;
	struct
	{
	    struct node * symlist;
	    int numsyms;
 	} s;
    } u;
} node;



node *new_node(char *name){
   node *n;

   if (!(n       = calloc(1, sizeof(node))) ||
       !(n->name = strdup(name))
      )
   {
	perror("Internal Error - gensets");
	exit(1);
   }

   return n;
}

node *get_node(node **list, char *name)
{
    node **curr = list;

    while (*curr)
    {
	if (strcmp((*curr)->name, name) == 0)
	    return *curr;

	curr = &((*curr)->next);
    }

    return (*curr = new_node(name));
}


void add_sym(node **setlist, char *setname, char *symname, int pri)
{
    node *set = get_node(setlist, setname);
    node **curr = &(set->u.s.symlist);
    node *oldsym;

    set->u.s.numsyms++;

    while (*curr && (*curr)->u.pri <= pri)
	curr = &((*curr)->next);

    oldsym = *curr;
    *curr = new_node(symname);

    (*curr)->next = oldsym;
    (*curr)->u.pri = pri;
}

int emit_sets(node *setlist, FILE *out)
{
    int ret = 0;
    node *currsym;

    while (setlist)
    {
	ret = 1;

	currsym = setlist->u.s.symlist;

	while (currsym)
	{
	    fprintf(out, "extern int %s;\n", currsym->name);
	    currsym = currsym->next;
	}

	fprintf(out, "void *%s[]=\n{\n", setlist->name);
	fprintf(out, "\t(void *)%d", setlist->u.s.numsyms);

	currsym = setlist->u.s.symlist;

	while (currsym)
	{
	    fprintf(out, ", &%s", currsym->name);
	    currsym = currsym->next;
	}

   	fprintf(out, ", (void *)0\n};\n\n");

	setlist = setlist->next;
    }

    return ret;
}

int gensets(FILE *in, FILE *out)
{
    char sym[201];
    node *setlist = NULL;

    while (fscanf(in, " %200s ", sym)>0)
    {
	char *idx, *idx2, c;
	int pri=0, num;

	if (strncmp(sym, "__aros_set_", 11))
	    continue;

	idx = sym + 11;

	if (sscanf(idx, "%d%c%n", &pri, &c, &num)<2 || c!='_')
	{
	    fprintf(stderr, "Error: malformed symbolset name %s\n"
	                    "The wrong part starts with %s\n", sym, idx);
	    exit(1);
	}

	idx += num;

	idx2 = strstr(idx, "_element_");
	if (!idx2)
	{
	    fprintf(stderr, "Error: malformed symbolset name %s\n"
	                    "The wrong part starts with %s\n", sym, idx);
	    exit(1);
	}

	*idx2='\0'; /*terminate the set string*/

	idx2 += 9;

        if (strstr(idx2, "_element_"))
	{
	    fprintf(stderr, "Illegal symbol name containing \"_element_\"\n");
	    exit(1);
	}

	add_sym(&setlist, idx, idx2, pri);
    }

    if (ferror(stdin))
    {
	perror("Internal error - gensets");
    	exit(1);
    }

    return emit_sets(setlist, out);
}
