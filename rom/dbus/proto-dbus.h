/* Automatically generated header! Do not edit! */

#ifndef PROTO_DBUS_H
#define PROTO_DBUS_H

#include <clib/dbus_protos.h>

#ifndef _NO_INLINE
# if defined(__GNUC__)
#  ifdef __AROS__
#   include <defines/dbus.h>
#  else
#   include <inline/dbus.h>
#  endif
# else
#  include <pragmas/dbus_pragmas.h>
# endif
#endif /* _NO_INLINE */

#ifdef __amigaos4__
# include <interfaces/dbus.h>
# ifndef __NOGLOBALIFACE__
   extern struct DBUSIFace *IDBUS;
# endif /* __NOGLOBALIFACE__*/
#else /* !__amigaos4__ */
# ifndef __NOLIBBASE__
   extern struct Library *
#  ifdef __CONSTLIBBASEDECL__
    __CONSTLIBBASEDECL__
#  endif /* __CONSTLIBBASEDECL__ */
   DBUSBase;
# endif /* !__NOLIBBASE__ */
#endif /* !__amigaos4__ */

#endif /* !PROTO_DBUS_H */
