#ifndef ___ENV_H
#define ___ENV_H

/*
    Copyright © 1995-2023, The AROS Development Team. All rights reserved.
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

void __env_delvar(const char *varname);

int __env_get_environ(char **environ, int size);

#endif /* ___ENV_H */
