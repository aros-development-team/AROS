/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    Desc: The query function called by rexxmast to find the library functions
    Lang: English
*/

#include <rexx/rexxcall.h>
#include <rexx/errors.h>
#include <aros/debug.h>
#include "rexxsupport_intern.h"
#include "rxfunctions.h"

#include <string.h>
#include <stdlib.h>

/* In arexxfunc the function names with the corresponding function
 * to call are stored.
 * UBYTE ** has to be filled with a value pointing to an argstring
 * created from the rexxsyslib.library.
 */
struct arexxfunc {
  const char *commandname;
  UBYTE minargs, maxargs;
  LONG (*f)(struct Library *, struct RexxMsg *, UBYTE **);
};

/* The following function list has to be sorted on name */
struct arexxfunc funcs[] = {
    { "ALLOCMEM", 1, 2, rxsupp_allocmem },
    { "FREEMEM", 2, 2, rxsupp_freemem }
};
#define FUNCCOUNT (sizeof(funcs)/sizeof(struct arexxfunc))

int comparefunc(const void *name, const void *func)
{
    return strcmp((const char *)name, ((const struct arexxfunc *)func)->commandname);
}

AROS_AREXXLIBQUERYFUNC(ArexxDispatch, msg,
		       struct Library *, RexxSupportBase, 5, RexxSupport)
{
    struct arexxfunc *func;
    UBYTE *argstring = NULL;
    LONG rc;
    UBYTE n = msg->rm_Action & RXARGMASK;
  
    func = bsearch(ARG0(msg), funcs, FUNCCOUNT, sizeof(struct arexxfunc), comparefunc);
    if (func == NULL)
    {
        ReturnRexxQuery(1, NULL);
    }
    else if (n < func->minargs || n > func->maxargs)
    {
        kprintf("Wrong number of arguments %d\n", n);
        ReturnRexxQuery(ERR10_018, NULL);
    }
    else 
    {
        rc = func->f(RexxSupportBase, msg, &argstring);
        ReturnRexxQuery(rc, argstring);
    }
}
