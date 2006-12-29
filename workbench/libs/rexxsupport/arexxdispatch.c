/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
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
    { "ALLOCMEM" , 1, 2, rxsupp_allocmem  },
    { "BADDR"    , 1, 1, rxsupp_baddr     },
    { "CLOSEPORT", 1, 1, rxsupp_closeport },
    { "DELAY"    , 1, 1, rxsupp_delay     },
    { "DELETE"   , 1, 1, rxsupp_delete    },
    { "FORBID"   , 0, 0, rxsupp_forbid    },
    { "FREEMEM"  , 2, 2, rxsupp_freemem   },
    { "GETARG"   , 1, 2, rxsupp_getarg    },
    { "GETPKT"   , 1, 1, rxsupp_getpkt    },
    { "MAKEDIR"  , 1, 1, rxsupp_makedir   },
    { "NEXT"     , 1, 2, rxsupp_next      },
    { "NULL"     , 0, 0, rxsupp_null      },
    { "OFFSET"   , 2, 2, rxsupp_offset    },
    { "OPENPORT" , 1, 1, rxsupp_openport  },
    { "PERMIT"   , 0, 0, rxsupp_permit    },
    { "RENAME"   , 2, 2, rxsupp_rename    },
    { "REPLY"    , 1, 2, rxsupp_reply     },
    { "SHOWDIR"  , 1, 3, rxsupp_showdir   },
    { "SHOWLIST" , 1, 3, rxsupp_showlist  },
    { "STATEF"   , 1, 1, rxsupp_statef    },
    { "TYPEPKT"  , 1, 2, rxsupp_typepkt   },
    { "WAITPKT"  , 1, 1, rxsupp_waitpkt   }
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
        ReturnRexxQuery(ERR10_018, NULL);
    }
    else 
    {
        rc = func->f(RexxSupportBase, msg, &argstring);
        ReturnRexxQuery(rc, argstring);
    }
}
AROS_AREXXLIBQUERYFUNC_END
