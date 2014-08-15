#ifndef PROTO_CODESETS_H
#define PROTO_CODESETS_H

/***************************************************************************

 codesets.library - Amiga shared library for handling different codesets
 Copyright (C) 2001-2005 by Alfonso [alfie] Ranieri <alforan@tin.it>.
 Copyright (C) 2005-2014 codesets.library Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 codesets.library project: http://sourceforge.net/projects/codesetslib/

 $Id$

***************************************************************************/

#ifndef LIBRARIES_CODESETS_H
#include <libraries/codesets.h>
#endif

/****************************************************************************/

#ifndef __NOLIBBASE__
 #ifndef __USE_BASETYPE__
  extern struct Library * CodesetsBase;
 #else
  extern struct Library * CodesetsBase;
 #endif /* __USE_BASETYPE__ */
#endif /* __NOLIBBASE__ */

/****************************************************************************/

#ifdef __amigaos4__
 #include <interfaces/codesets.h>
 #ifdef __USE_INLINE__
  #include <inline4/codesets.h>
 #endif /* __USE_INLINE__ */
 #ifndef CLIB_CODESETS_PROTOS_H
  #define CLIB_CODESETS_PROTOS_H 1
 #endif /* CLIB_CODESETS_PROTOS_H */
 #ifndef __NOGLOBALIFACE__
  extern struct CodesetsIFace *ICodesets;
 #endif /* __NOGLOBALIFACE__ */
#else /* __amigaos4__ */
 #ifndef CLIB_CODESETS_PROTOS_H
  #include <clib/codesets_protos.h>
 #endif /* CLIB_CODESETS_PROTOS_H */
 #if defined(__GNUC__)
  #ifdef __AROS__
   #include <defines/codesets.h>
  #else
   #ifndef __PPC__
    #include <inline/codesets.h>
   #else
    #include <ppcinline/codesets.h>
   #endif /* __PPC__ */
  #endif /* __AROS__ */
 #elif defined(__VBCC__)
  #ifndef __PPC__
   #include <inline/codesets_protos.h>
  #endif /* __PPC__ */
 #else
  #include <pragmas/codesets_pragmas.h>
 #endif /* __GNUC__ */
#endif /* __amigaos4__ */

/****************************************************************************/

#endif /* PROTO_CODESETS_H */
