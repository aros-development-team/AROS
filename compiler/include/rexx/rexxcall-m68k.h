/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Macro's to make calling arexx library querying functions portable
    Lang: English
*/

#ifndef AROS_REXXCALL_H
#define AROS_REXXCALL_H

/* Some macro's to make ARexx portable to non-m68k platforms */
#define RexxCallQueryLibFunc(rexxmsg, libbase, offset, retargstringptr) \
  ({ \
    int _offset=abs(offset)/6; \
    AROS_LVO_CALL2(ULONG, \
	       AROS_LCA(struct RexxMsg *, rexxmsg, A0), \
	       AROS_LCA(STRPTR *, retargstringptr, A1), \
	       struct Library *, libbase, _offset, rexxcall); \
  })

#define AROS_AREXXLIBQUERYFUNC(f,m,lt,l,o,p) \
  AROS_LH2(ULONG, f, \
	     AROS_LCA(struct RexxMsg *, m, A0), \
	     AROS_LCA(STRPTR *, _retargstringptr, A1), \
	     lt, l, o, p)

#warning FIXME: retargstringptr has to be returned in A0 also, asm has to be added
#define ReturnRexxQuery(rc,arg) \
  *_retargstringptr = arg; \
  return rc;

#endif
