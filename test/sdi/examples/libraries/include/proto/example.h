/* Automatically generated header! Do not edit! */

#ifndef PROTO_EXAMPLE_H
#define PROTO_EXAMPLE_H

#include <clib/example_protos.h>

#ifndef _NO_INLINE
# if defined(__GNUC__)
#  ifdef __AROS__
#   include <defines/example.h>
#  else
#   include <inline/example.h>
#  endif
# else
#  include <pragmas/example_pragmas.h>
# endif
#endif /* _NO_INLINE */

#ifdef __amigaos4__
# include <interfaces/example.h>
# ifndef __NOGLOBALIFACE__
   extern struct ExampleIFace *IExample;
# endif /* __NOGLOBALIFACE__*/
#else /* !__amigaos4__ */
# ifndef __NOLIBBASE__
   extern struct Library *
#  ifdef __CONSTLIBBASEDECL__
    __CONSTLIBBASEDECL__
#  endif /* __CONSTLIBBASEDECL__ */
   ExampleBase;
# endif /* !__NOLIBBASE__ */
#endif /* !__amigaos4__ */

#endif /* !PROTO_EXAMPLE_H */
