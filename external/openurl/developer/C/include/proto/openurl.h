#ifndef PROTO_OPENURL_H
#define PROTO_OPENURL_H

/***************************************************************************

 openurl.library - universal URL display and browser launcher library
 Copyright (C) 1998-2005 by Troels Walsted Hansen, et al.
 Copyright (C) 2005-2013 by openurl.library Open Source Team

 This library is free software; it has been placed in the public domain
 and you can freely redistribute it and/or modify it. Please note, however,
 that some components may be under the LGPL or GPL license.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

 openurl.library project: http://sourceforge.net/projects/openurllib/

 $Id$

***************************************************************************/

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
  #ifdef __AROS__
   #include <defines/openurl.h>
  #else
   #ifndef __PPC__
    #include <inline/openurl.h>
   #else
    #include <ppcinline/openurl.h>
   #endif /* __PPC__ */
  #endif /* __AROS__ */
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
