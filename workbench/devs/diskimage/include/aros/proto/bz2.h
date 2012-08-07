/* Automatically generated header (sfdc 1.4)! Do not edit! */

#ifndef PROTO_BZ2_H
#define PROTO_BZ2_H

#include <clib/bz2_protos.h>

#ifndef _NO_INLINE
# if defined(__GNUC__)
#  ifdef __AROS__
#   include <defines/bz2.h>
#  else
#   include <inline/bz2.h>
#  endif
# else
#  include <pragmas/bz2_pragmas.h>
# endif
#endif /* _NO_INLINE */

#ifdef __amigaos4__
# include <interfaces/bz2.h>
# ifndef __NOGLOBALIFACE__
   extern struct BZ2IFace *IBZ2;
# endif /* __NOGLOBALIFACE__*/
#endif /* !__amigaos4__ */
#ifndef __NOLIBBASE__
  extern struct Library *
# ifdef __CONSTLIBBASEDECL__
   __CONSTLIBBASEDECL__
# endif /* __CONSTLIBBASEDECL__ */
  BZ2Base;
#endif /* !__NOLIBBASE__ */

#endif /* !PROTO_BZ2_H */
