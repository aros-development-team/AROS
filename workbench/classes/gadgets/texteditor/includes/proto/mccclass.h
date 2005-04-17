#ifndef PROTO_MCCCLASS_H
#define PROTO_MCCCLASS_H

/*
**	$Id: mccclass.h,v 1.1 2005/04/10 08:23:37 damato Exp $
**	Includes Release 50.1
**
**	Prototype/inline/pragma header file combo
**
**	(C) Copyright 2003-2004 Amiga, Inc.
**	    All Rights Reserved
*/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

/****************************************************************************/

#ifndef __NOLIBBASE__
//extern struct Library * MCCClassBase;
#endif /* __NOLIBBASE__ */

/****************************************************************************/

#ifdef __amigaos4__
 #include <interfaces/mccclass.h>
 #ifdef __USE_INLINE__
  #include <inline4/mccclass.h>
 #endif /* __USE_INLINE__ */
 #ifndef CLIB_MCCCLASS_PROTOS_H
  #define CLIB_MCCCLASS_PROTOS_H 1
 #endif /* CLIB_MCCCLASS_PROTOS_H */
 #ifndef __NOGLOBALIFACE__
//	extern struct MCCClassIFace *IMCCClass;
 #endif /* __NOGLOBALIFACE__ */
#else /* __amigaos4__ */
 #ifndef CLIB_MCCCLASS_PROTOS_H
  #include <clib/mccclass_protos.h>
 #endif /* CLIB_MCCCLASS_PROTOS_H */
 #if defined(__GNUC__)
  #ifndef __PPC__
   #include <inline/mccclass.h>
  #else
   #include <ppcinline/mccclass.h>
  #endif /* __PPC__ */
 #elif defined(__VBCC__)
  #ifndef __PPC__
   #include <inline/mccclass_protos.h>
  #endif /* __PPC__ */
 #else
  #include <pragmas/mccclass_pragmas.h>
 #endif /* __GNUC__ */
#endif /* __amigaos4__ */

/****************************************************************************/

#endif /* PROTO_MCCCLASS_H */
