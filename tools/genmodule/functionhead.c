/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$
    
    The code for storing information of functions present in the module
*/
#include <string.h>
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
	funchead->novararg = 0;
	funchead->priv= 0;
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
    const char *arg, const char *reg
)
{
    struct functionarg **argptr = &funchead->arguments;
    
    while ((*argptr) != NULL) argptr = &(*argptr)->next;
    
    *argptr = malloc(sizeof(struct functionarg));
    if (*argptr != NULL)
    {
	(*argptr)->next = NULL;
	(*argptr)->arg  = (arg == NULL) ? NULL : strdup(arg);
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

char *getargtype(const struct functionarg *funcarg)
{
    char *s, *begin, *end;
    unsigned int brackets = 0, i;

    begin = s = strdup(funcarg->arg);
    
    /* Count the [] at the end of the argument */
    end = begin+strlen(begin);
    while (isspace(*(end-1))) end--;
    while (*(end-1)==']')
    {
	brackets++;
	end--;
	while (isspace(*(end-1))) end--;
	if (*(end-1)!='[')
	{
	    free(s);
	    return NULL;
	}
	end--;
	while (isspace(*(end-1))) end--;
    }
			
    /* Skip over the argument name */
    while (!isspace(*(end-1)) && *(end-1)!='*') end--;

    /* Add * for the brackets */
    while (isspace(*(end-1))) end--;
    for (i=0; i<brackets; i++)
    {
	*end='*';
	end++;
    }
    *end='\0';

    return s;
}

char *getargname(const struct functionarg *funcarg)
{
    char *s, *begin, *end;
    int len;
    
    /* Count the [] at the end of the argument */
    end = funcarg->arg+strlen(funcarg->arg);
    while (isspace(*(end-1))) end--;
    while (*(end-1)==']')
    {
	end--;
	while (isspace(*(end-1))) end--;
	if (*(end-1)!='[')
	    return NULL;
	end--;
	while (isspace(*(end-1))) end--;
    }
			
    /* Go to the beginning of the argument name */
    begin = end;
    while (!isspace(*(begin-1)) && *(begin-1)!='*') begin--;

    /* Copy the name */
    len = end - begin;
    s = malloc(len+1);
    strncpy(s, begin, len);
    s[len] = '\0';

    return s;
}
