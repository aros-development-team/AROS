/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
    
    The code for storing information of functions present in the module
*/
#include "functionhead.h"

/* In funclist the information of all the functions of the module will be stored.
 * The list has to be sorted on the lvonum field
 */
struct functionhead *funclist = NULL;

/* In methlist the information of all the methods of the class will be 
 * stored. We (mis)use struct functionhead for this, but don't use certain
 * fields (like lvo and reg (in struct arglist)).
 */
struct functionhead *methlist;

struct functionhead *newfunctionhead(const char *name, const char *type, unsigned int lvo)
{
    struct functionhead *funchead = malloc(sizeof(struct functionhead));
    
    if (funchead != NULL)
    {
	funchead->next = NULL;
	funchead->name = (name == NULL) ? NULL : strdup(name);
	funchead->type = (type == NULL) ? NULL : strdup(type);
	funchead->lvo = lvo;
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

struct functionarg *funcaddarg(
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

struct functionalias *funcaddalias(struct functionhead *funchead, const char *alias)
{
    struct functionalias *funcalias = malloc(sizeof(struct functionalias));
    
    if (funcalias != NULL)
    {
	funcalias->next = funchead->aliases;
	funcalias->alias = strdup(alias);
	funchead->aliases = funcalias;
    }
    else
    {
	puts("Out of memory !");
	exit(20);
    }
    
    return funcalias;
}
