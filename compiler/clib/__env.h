#ifndef ___ENV_H
#define ___ENV_H

/*
    Copyright 2001 AROS - The Amiga Research OS
    $Id$

    Desc: internal header file for environment variables handling
    Lang: english
*/

typedef struct __env_item
{
    struct __env_item *next;
	char *name;
	char *value;
} __env_item;

extern __env_item *__env_list;

__env_item *__env_getvar(char *varname, int valuesize);
void __env_delvar(char *varname);

#endif /* ___ENV_H */
