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

#ifndef _CLIB_KERNEL_
extern __env_item *__env_list;
#endif

__env_item *__env_getvar(const char *varname, int valuesize);
void __env_delvar(const char *varname);

#endif /* ___ENV_H */
