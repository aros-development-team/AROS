/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    Internal functions for environment variables handling.
*/

#include <stdlib.h>
#include <strings.h>
#include <proto/exec.h>

#include "__env.h"

static __env_item *__env_newvar(const char *name, int valuesize)
{
    __env_item *item;

    item = malloc(sizeof(__env_item));
    if (!item) goto err1;

    item->name = strdup(name);
    if (!item->name) goto err2;

    item->value = malloc(valuesize);
    if (!item->value) goto err3;

    item->next = NULL;

    return item;

err3:
    free(item->name);
err2:
    free(item);
err1:
    return NULL;
}

static __env_item **internal_findvar(register const char *name)
{

   __env_item **curr;

   for (
       curr = &__env_list;
       *curr && strcmp((*curr)->name, name);
       curr = &((*curr)->next)
   );

  return curr;
}

/*
  Allocates space for a variable with name 'name' returning a pointer to it.
  If a variable with this name already exists then returns a pointer to that
  variable.

  Returns NULL on error.
*/
__env_item *__env_getvar(const char *name, int valuesize)
{
    register __env_item **curr;

    curr = internal_findvar(name);

    if (*curr)
    {
        if (strlen((*curr)->value) < valuesize)
        {
	    free((*curr)->value);
	    (*curr)->value = malloc(valuesize);

	    if (!(*curr)->value)
	    {
	        __env_item *tmp = (*curr)->next;

	        free(*curr);
	        *curr = tmp;
   	    }
	}
    }
    else 
    {
	*curr = __env_newvar(name, valuesize);
    }

    return *curr;
}

void __env_delvar(const char *name)
{
    register __env_item **curr;

    curr = internal_findvar(name);

    if (*curr)
    {
	register __env_item *tmp = *curr;

	*curr = (*curr)->next;

	free(tmp->name);
	free(tmp->value);
	free(tmp);
    }
}
