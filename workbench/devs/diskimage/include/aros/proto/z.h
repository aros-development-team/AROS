/* Automatically generated header (sfdc 1.4)! Do not edit! */

#ifndef PROTO_Z_H
#define PROTO_Z_H

#include <clib/z_protos.h>

#ifndef _NO_INLINE
# if defined(__GNUC__)
#  ifdef __AROS__
#   include <defines/z.h>
#  else
#   include <inline/z.h>
#  endif
# else
#  include <pragmas/z_pragmas.h>
# endif
#endif /* _NO_INLINE */

#ifdef __amigaos4__
# include <interfaces/z.h>
# ifndef __NOGLOBALIFACE__
   extern struct ZIFace *IZ;
# endif /* __NOGLOBALIFACE__*/
#endif /* !__amigaos4__ */
#ifndef __NOLIBBASE__
  extern struct Library *
# ifdef __CONSTLIBBASEDECL__
   __CONSTLIBBASEDECL__
# endif /* __CONSTLIBBASEDECL__ */
  ZBase;
#endif /* !__NOLIBBASE__ */

#endif /* !PROTO_Z_H */
