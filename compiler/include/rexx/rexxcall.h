/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Macro's to make calling arexx library querying functions portable
    Lang: English
*/

#ifndef REXX_REXXCALL_H
#define REXX_REXXCALL_H

#include <exec/types.h>

/* Some macro's to make ARexx portable to non-m68k platforms */
#define RexxCallQueryLibFunc(rexxmsg, libbase, offset, retargstringptr) \
  ({ \
    int _offset=abs(offset)/6; \
    AROS_LC2(ULONG, RexxCallQueryLibFunc, \
	       AROS_LCA(struct RexxMsg *, rexxmsg, A0), \
	       AROS_LCA(STRPTR *, retargstringptr, A1), \
	       struct Library *, libbase, _offset, rexxcall); \
  })

#define AROS_AREXXLIBQUERYFUNC(f,m,lt,l,o,p) \
  AROS_LH2(ULONG, f, \
	     AROS_LHA(struct RexxMsg *, m, A0), \
	     AROS_LHA(STRPTR *, _retargstringptr, A1), \
	     lt, l, o, p) { AROS_LIBFUNC_INIT
#define AROS_AREXXLIBQUERYFUNC_END \
    AROS_LIBFUNC_EXIT }

#define ReturnRexxQuery(rc,arg) \
  ({ *_retargstringptr = arg; \
     return rc; \
  })

#endif
