#ifndef PROTO_OPENURL_H
#define PROTO_OPENURL_H

/*
**  $VER: openurl.h 7.2 (1.12.2005)
**  Includes Release 7.2
**
**  SAS `C' style prototype/pragma header file combo
**
**  openurl.library - universal URL display and browser
**  launcher library
**
**  Written by Troels Walsted Hansen <troels@thule.no>
**  Placed in the public domain.
**
**  Developed by:
**  - Alfonso Ranieri <alforan@tin.it>
**  - Stefan Kost <ensonic@sonicpulse.de>
**  - Alexandre Balaban <alexandre@balaban.name>
**
*/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

/****************************************************************************/

#ifndef __NOLIBBASE__
 #ifndef __USE_BASETYPE__
extern struct Library *
#ifdef __CONSTLIBBASEDECL__
__CONSTLIBBASEDECL__
#endif /* __CONSTLIBBASEDECL__ */
OpenURLBase;

#else
  extern struct Library *
  #ifdef __CONSTLIBBASEDECL__
   __CONSTLIBBASEDECL__
  #endif /* __CONSTLIBBASEDECL__ */
  OpenURLBase;
 #endif /* __USE_BASETYPE__ */
#endif /* __NOLIBBASE__ */

/****************************************************************************/

#ifdef __amigaos4__
 #include <interfaces/openurl.h>
 #ifdef __USE_INLINE__
  #include <inline4/openurl.h>
 #endif /* __USE_INLINE__ */
 #ifndef CLIB_OPENURL_PROTOS_H
  #define CLIB_OPENURL_PROTOS_H 1
 #endif /* CLIB_OPENURL_PROTOS_H */
 #ifndef __NOGLOBALIFACE__
  extern struct OpenURLIFace *IOpenURL;
 #endif /* __NOGLOBALIFACE__ */
#else /* __amigaos4__ */
 #ifndef CLIB_OPENURL_PROTOS_H
  #include <clib/openurl_protos.h>
 #endif /* CLIB_OPENURL_PROTOS_H */
 #if defined(__GNUC__)
  #ifndef __PPC__
#include <inline/openurl.h>
  #else
   #include <ppcinline/openurl.h>
  #endif /* __PPC__ */
 #elif defined(__VBCC__)
  #ifndef __PPC__
   #include <inline/openurl_protos.h>
#endif /* __PPC__ */
#else
#include <pragmas/openurl_pragmas.h>
#endif /* __GNUC__ */
#endif /* __amigaos4__ */

/****************************************************************************/

#endif /* PROTO_OPENURL_H */

