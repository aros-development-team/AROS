#ifndef _INLINE_DATATYPES_H
#define _INLINE_DATATYPES_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef DATATYPES_BASE_NAME
#define DATATYPES_BASE_NAME DataTypesBase
#endif

#define AddDTObject(win, req, o, pos) \
	LP4(0x48, LONG, AddDTObject, struct Window *, win, a0, struct Requester *, req, a1, Object *, o, a2, long, pos, d0, \
	, DATATYPES_BASE_NAME)

#define DisposeDTObject(o) \
	LP1NR(0x36, DisposeDTObject, Object *, o, a0, \
	, DATATYPES_BASE_NAME)

#define DoAsyncLayout(o, gpl) \
	LP2(0x54, ULONG, DoAsyncLayout, Object *, o, a0, struct gpLayout *, gpl, a1, \
	, DATATYPES_BASE_NAME)

#define DoDTMethodA(o, win, req, msg) \
	LP4(0x5a, ULONG, DoDTMethodA, Object *, o, a0, struct Window *, win, a1, struct Requester *, req, a2, Msg, msg, a3, \
	, DATATYPES_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define DoDTMethod(a0, a1, a2, tags...) \
	({ULONG _tags[] = { tags }; DoDTMethodA((a0), (a1), (a2), (Msg)_tags);})
#endif /* !NO_INLINE_STDARG */

#define GetDTAttrsA(o, attrs) \
	LP2(0x42, ULONG, GetDTAttrsA, Object *, o, a0, struct TagItem *, attrs, a2, \
	, DATATYPES_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define GetDTAttrs(a0, tags...) \
	({ULONG _tags[] = { tags }; GetDTAttrsA((a0), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define GetDTMethods(object) \
	LP1(0x66, ULONG *, GetDTMethods, Object *, object, a0, \
	, DATATYPES_BASE_NAME)

#define GetDTString(id) \
	LP1(0x8a, STRPTR, GetDTString, unsigned long, id, d0, \
	, DATATYPES_BASE_NAME)

#define GetDTTriggerMethods(object) \
	LP1(0x6c, struct DTMethods *, GetDTTriggerMethods, Object *, object, a0, \
	, DATATYPES_BASE_NAME)

#define NewDTObjectA(name, attrs) \
	LP2(0x30, Object *, NewDTObjectA, APTR, name, d0, struct TagItem *, attrs, a0, \
	, DATATYPES_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define NewDTObject(a0, tags...) \
	({ULONG _tags[] = { tags }; NewDTObjectA((a0), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define ObtainDataTypeA(type, handle, attrs) \
	LP3(0x24, struct DataType *, ObtainDataTypeA, unsigned long, type, d0, APTR, handle, a0, struct TagItem *, attrs, a1, \
	, DATATYPES_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define ObtainDataType(a0, a1, tags...) \
	({ULONG _tags[] = { tags }; ObtainDataTypeA((a0), (a1), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define PrintDTObjectA(o, w, r, msg) \
	LP4(0x72, ULONG, PrintDTObjectA, Object *, o, a0, struct Window *, w, a1, struct Requester *, r, a2, struct dtPrint *, msg, a3, \
	, DATATYPES_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define PrintDTObject(a0, a1, a2, tags...) \
	({ULONG _tags[] = { tags }; PrintDTObjectA((a0), (a1), (a2), (struct dtPrint *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define RefreshDTObjectA(o, win, req, attrs) \
	LP4NR(0x4e, RefreshDTObjectA, Object *, o, a0, struct Window *, win, a1, struct Requester *, req, a2, struct TagItem *, attrs, a3, \
	, DATATYPES_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define RefreshDTObject(a0, a1, a2, tags...) \
	({ULONG _tags[] = { tags }; RefreshDTObjectA((a0), (a1), (a2), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define ReleaseDataType(dt) \
	LP1NR(0x2a, ReleaseDataType, struct DataType *, dt, a0, \
	, DATATYPES_BASE_NAME)

#define RemoveDTObject(win, o) \
	LP2(0x60, LONG, RemoveDTObject, struct Window *, win, a0, Object *, o, a1, \
	, DATATYPES_BASE_NAME)

#define SetDTAttrsA(o, win, req, attrs) \
	LP4(0x3c, ULONG, SetDTAttrsA, Object *, o, a0, struct Window *, win, a1, struct Requester *, req, a2, struct TagItem *, attrs, a3, \
	, DATATYPES_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define SetDTAttrs(a0, a1, a2, tags...) \
	({ULONG _tags[] = { tags }; SetDTAttrsA((a0), (a1), (a2), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#endif /* _INLINE_DATATYPES_H */
