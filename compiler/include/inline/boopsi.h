/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _INLINE_BOOPSI_H
#define _INLINE_BOOPSI_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif /* !__INLINE_MACROS_H */

#ifndef BOOPSI_BASE_NAME
#define BOOPSI_BASE_NAME BOOPSIBase
#endif /* !BOOPSI_BASE_NAME */

#ifdef AddClass
#   undef AddClass
#endif
#ifdef DisposeObject
#   undef DisposeObject
#endif
#ifdef FreeClass
#   undef FreeClass
#endif
#ifdef GetAttr
#   undef GetAttr
#endif
#ifdef MakeClass
#   undef MakeClass
#endif
#ifdef NewObjectA
#   undef NewObjectA
#endif
#ifdef NextObject
#   undef NextObject
#endif
#ifdef RemoveClass
#   undef RemoveClass
#endif
#ifdef SetAttrsA
#   undef SetAttrsA
#endif

#define AddClass(classPtr) \
	LP1NR(0x1e, AddClass, struct IClass *, classPtr, a0, \
	, BOOPSI_BASE_NAME)

#define DisposeObject(object) \
	LP1NR(0x24, DisposeObject, APTR, object, a0, \
	, BOOPSI_BASE_NAME)

#define FindClass(classID) \
	LP1(0x2a, struct IClass *, FindClass, ClassID, classID, a0, \
	, BOOPSI_BASE_NAME)

#define FreeClass(classPtr) \
	LP1(0x30, BOOL, FreeClass, struct IClass *, classPtr, a0, \
	, BOOPSI_BASE_NAME)

#define GetAttr(attrID, object, storagePtr) \
	LP3(0x36, ULONG, GetAttr, ULONG, attrID, d0, Object *, object, a0, IPTR *, storagePtr, a1, \
	, BOOPSI_BASE_NAME)

#define MakeClass(classID, superClassID, superClassPtr, instanceSize, flags) \
	LP5(0x3c, struct IClass *, MakeClass, UBYTE *, classID, a0, UBYTE *, superClassID, a1, struct IClass *, superClassPtr, a2, ULONG, instanceSize, d0, ULONG, flags, d1, \
	, BOOPSI_BASE_NAME)

#define NewObjectA(classPtr, classID, tagList) \
	LP3(0x42, APTR, NewObjectA, struct IClass *, classPtr, a0, UBYTE *, classID, a1, struct TagItem *, tagList, a2, \
	, BOOPSI_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define NewObject(a0, a1, tags...) \
	({ULONG _tags[] = { tags }; NewObjectA((a0), (a1), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define NextObject(objectPtrPtr) \
	LP1(0x48, APTR, NextObject, APTR, objectPtrPtr, a0, \
	, BOOPSI_BASE_NAME)

#define RemoveClass(classPtr) \
	LP1NR(0x4e, RemoveClass, struct IClass *, classPtr, a0, \
	, BOOPSI_BASE_NAME)

#define SetAttrsA(object, tagList) \
	LP2(0x54, ULONG, SetAttrsA, APTR, object, a0, struct TagItem *, tagList, a1, \
	, BOOPSI_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define SetAttrs(a0, tags...) \
	({ULONG _tags[] = { tags }; SetAttrsA((a0), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#endif /* !_INLINE_BOOPSI_H */
