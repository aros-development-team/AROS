/* Automatically generated header (sfdc 1.4)! Do not edit! */

#ifndef PROTO_DISKIMAGE_H
#define PROTO_DISKIMAGE_H

#include <clib/diskimage_protos.h>

#ifndef _NO_INLINE
# if defined(__GNUC__)
#  ifdef __AROS__
#   include <defines/diskimage.h>
#  else
#   include <inline/diskimage.h>
#  endif
# else
#  include <pragmas/diskimage_pragmas.h>
# endif
#endif /* _NO_INLINE */

#ifdef __amigaos4__
# include <interfaces/diskimage.h>
# ifndef __NOGLOBALIFACE__
   extern struct DiskImageIFace *IDiskImage;
# endif /* __NOGLOBALIFACE__*/
#endif /* !__amigaos4__ */
#ifndef __NOLIBBASE__
  extern struct Library *
# ifdef __CONSTLIBBASEDECL__
   __CONSTLIBBASEDECL__
# endif /* __CONSTLIBBASEDECL__ */
  DiskImageBase;
#endif /* !__NOLIBBASE__ */

#endif /* !PROTO_DISKIMAGE_H */
