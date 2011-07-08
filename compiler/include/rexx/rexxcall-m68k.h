/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Macro's to make calling arexx library querying functions portable
    Lang: English
*/

#ifndef AROS_REXXCALL_M68K_H
#define AROS_REXXCALL_M68K_H

/* Some macro's to make ARexx portable to non-m68k platforms */
#define RexxCallQueryLibFunc(rexxmsg, libbase, offset, retargstringptr) \
  ({ \
    int _offset=abs(offset)/6; \
    AROS_LVO_CALL2(ULONG, \
	       AROS_LCA(struct RexxMsg *, rexxmsg, A0), \
	       AROS_LCA(STRPTR *, retargstringptr, A1), \
	       struct Library *, libbase, _offset, rexxcall); \
  })

/* ARexx Query Functions return their arguments in BOTH
 * A0 and D0.
 */
#define AROS_AREXXLIBQUERYFUNC(f,m,lt,l,o,p) \
  AROS_UFP3(ULONG, p##_##f##_wrapper, \
	     AROS_UFPA(struct RexxMsg *, m, A0), \
	     AROS_UFPA(STRPTR *, _retargstringptr, A1), \
	     AROS_UFHA(lt, l, A6)); \
  asm ( ".text\n" \
  	".global " #p "_" #o "_" #f "\n" \
  	#p "_" #o "_" #f ":\n" \
  	"jsr " #p "_" #f "_wrapper \n" \
  	"move.l %d0,%a0 \n" \
  	"rts\n" ); \
  AROS_UFH3(ULONG, p##_##f##_wrapper, \
	     AROS_UFHA(struct RexxMsg *, m, A0), \
	     AROS_UFHA(STRPTR *, _retargstringptr, A1), \
	     AROS_UFHA(lt, l, A6)) { AROS_USERFUNC_INIT 
#define AROS_AREXXLIBQUERYFUNC_END \
    AROS_USERFUNC_EXIT }

#define ReturnRexxQuery(rc,arg) \
  *_retargstringptr = arg; \
  return rc;

#endif
