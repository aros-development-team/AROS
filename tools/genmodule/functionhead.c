/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$
    
    The code for storing information of functions present in the module
*/
#include "functionhead.h"

struct functionhead *newfunctionhead(const char *name, enum libcall libcall)
{
    struct functionhead *funchead = malloc(sizeof(struct functionhead));
    
    if (funchead != NULL)
    {
	funchead->next = NULL;
	funchead->name = strdup(name);
	funchead->type = NULL;
	funchead->libcall = libcall;
	funchead->lvo = 0;
	funchead->argcount = 0;
	funchead->arguments = NULL;
	funchead->aliases = NULL;
    }
    else
    {
	puts("Out of memory !");
	exit(20);
    }
    
    return funchead;
}

struct functionarg *funcaddarg
(
    struct functionhead *funchead,
    const char *name, const char *type, const char *reg
)
{
    struct functionarg **argptr = &funchead->arguments;
    
    while ((*argptr) != NULL) argptr = &(*argptr)->next;
    
    *argptr = malloc(sizeof(struct functionarg));
    if (*argptr != NULL)
    {
	(*argptr)->next = NULL;
	(*argptr)->name = (name == NULL) ? NULL : strdup(name);
	(*argptr)->type = (type == NULL) ? NULL : strdup(type);
	(*argptr)->reg  = (reg  == NULL) ? NULL : strdup(reg);
	
	funchead->argcount++;
    }
    else
    {
	puts("Out of memory !");
	exit(20);
    }
    
    return *argptr;
}

struct stringlist *funcaddalias(struct functionhead *funchead, const char *alias)
{
    return slist_append(&funchead->aliases, alias);
}

struct functions *functionsinit(void)
{
    struct functions *functions = malloc(sizeof(struct functions));
    
    if (functions == NULL)
    {
	fprintf(stderr, "Out of memory\n");
	exit (20);
    }
    functions->funclist = NULL;
    functions->methlist = NULL;
    
    return functions;
}
