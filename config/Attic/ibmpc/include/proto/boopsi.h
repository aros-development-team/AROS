#ifndef PROTO_BOOPSI_H
#define PROTO_BOOPSI_H

#ifndef INLINE_MACROS_H
#include <inline/macros.h>
#endif /* !INLINE_MACROS_H */

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
	LP1NR(0x14, AddClass, struct IClass *, classPtr, \
	, BOOPSI_BASE_NAME)

#define DisposeObject(object) \
	LP1NR(0x18, DisposeObject, APTR, object, \
	, BOOPSI_BASE_NAME)

#define FindClass(classID) \
	LP1(0x1c, struct IClass *, FindClass, ClassID, classID, \
	, BOOPSI_BASE_NAME)

#define FreeClass(classPtr) \
	LP1(0x20, BOOL, FreeClass, struct IClass *, classPtr, \
	, BOOPSI_BASE_NAME)

#define GetAttr(attrID, object, storagePtr) \
	LP3(0x24, ULONG, GetAttr, ULONG, attrID, Object *, object, IPTR *, storagePtr, \
	, BOOPSI_BASE_NAME)

#define MakeClass(classID, superClassID, superClassPtr, instanceSize, flags) \
	LP5(0x28, struct IClass *, MakeClass, UBYTE *, classID, UBYTE *, superClassID, struct IClass *, superClassPtr, ULONG, instanceSize, ULONG, flags, \
	, BOOPSI_BASE_NAME)

#define NewObjectA(classPtr, classID, tagList) \
	LP3(0x2c, APTR, NewObjectA, struct IClass *, classPtr, UBYTE *, classID, struct TagItem *, tagList, \
	, BOOPSI_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define NewObject(a0, a1, tags...) \
	({ULONG _tags[] = { tags }; NewObjectA((a0), (a1), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define NextObject(objectPtrPtr) \
	LP1(0x30, APTR, NextObject, APTR, objectPtrPtr, \
	, BOOPSI_BASE_NAME)

#define RemoveClass(classPtr) \
	LP1NR(0x34, RemoveClass, struct IClass *, classPtr, \
	, BOOPSI_BASE_NAME)

#define SetAttrsA(object, tagList) \
	LP2(0x38, ULONG, SetAttrsA, APTR, object, struct TagItem *, tagList, \
	, BOOPSI_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define SetAttrs(a0, tags...) \
	({ULONG _tags[] = { tags }; SetAttrsA((a0), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#endif /* !PROTO_BOOPSI_H */
