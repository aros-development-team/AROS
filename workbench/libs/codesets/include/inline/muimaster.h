/* Automatically generated header! Do not edit! */

#ifndef _INLINE_MUIMASTER_H
#define _INLINE_MUIMASTER_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif /* !__INLINE_MACROS_H */

#ifndef MUIMASTER_BASE_NAME
#define MUIMASTER_BASE_NAME MUIMasterBase
#endif /* !MUIMASTER_BASE_NAME */

#define MUI_AddClipRegion(mri, region) \
	LP2(0xb4, APTR, MUI_AddClipRegion, struct MUI_RenderInfo *, mri, a0, struct Region *, region, a1, \
	, MUIMASTER_BASE_NAME)

#define MUI_AddClipping(mri, l, t, w, h) \
	LP5(0xa8, APTR, MUI_AddClipping, struct MUI_RenderInfo *, mri, a0, WORD, l, d0, WORD, t, d1, WORD, w, d2, WORD, h, d3, \
	, MUIMASTER_BASE_NAME)

#define MUI_AllocAslRequest(type, tags) \
	LP2(0x30, APTR, MUI_AllocAslRequest, unsigned long, type, d0, struct TagItem *, tags, a0, \
	, MUIMASTER_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define MUI_AllocAslRequestTags(a0, tags...) \
	({ULONG _tags[] = { tags }; MUI_AllocAslRequest((a0), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define MUI_AslRequest(req, tags) \
	LP2(0x36, BOOL, MUI_AslRequest, APTR, req, a0, struct TagItem *, tags, a1, \
	, MUIMASTER_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define MUI_AslRequestTags(a0, tags...) \
	({ULONG _tags[] = { tags }; MUI_AslRequest((a0), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define MUI_BeginRefresh(mri, flags) \
	LP2(0xc0, BOOL, MUI_BeginRefresh, struct MUI_RenderInfo *, mri, a0, ULONG, flags, d0, \
	, MUIMASTER_BASE_NAME)

#define MUI_CreateCustomClass(base, supername, supermcc, datasize, dispatcher) \
	LP5(0x6c, struct MUI_CustomClass *, MUI_CreateCustomClass, struct Library *, base, a0, char *, supername, a1, struct MUI_CustomClass *, supermcc, a2, int, datasize, d0, APTR, dispatcher, a3, \
	, MUIMASTER_BASE_NAME)

#define MUI_DeleteCustomClass(mcc) \
	LP1(0x72, BOOL, MUI_DeleteCustomClass, struct MUI_CustomClass *, mcc, a0, \
	, MUIMASTER_BASE_NAME)

#define MUI_DisposeObject(obj) \
	LP1NR(0x24, MUI_DisposeObject, Object *, obj, a0, \
	, MUIMASTER_BASE_NAME)

#define MUI_EndRefresh(mri, flags) \
	LP2NR(0xc6, MUI_EndRefresh, struct MUI_RenderInfo *, mri, a0, ULONG, flags, d0, \
	, MUIMASTER_BASE_NAME)

#define MUI_Error() \
	LP0(0x42, LONG, MUI_Error, \
	, MUIMASTER_BASE_NAME)

#define MUI_FreeAslRequest(req) \
	LP1NR(0x3c, MUI_FreeAslRequest, APTR, req, a0, \
	, MUIMASTER_BASE_NAME)

#define MUI_FreeClass(cl) \
	LP1NR(0x54, MUI_FreeClass, struct IClass *, cl, a0, \
	, MUIMASTER_BASE_NAME)

#define MUI_GetClass(name) \
	LP1(0x4e, struct IClass *, MUI_GetClass, char *, name, a0, \
	, MUIMASTER_BASE_NAME)

#define MUI_Layout(obj, l, t, w, h, flags) \
	LP6(0x7e, BOOL, MUI_Layout, Object *, obj, a0, LONG, l, d0, LONG, t, d1, LONG, w, d2, LONG, h, d3, ULONG, flags, d4, \
	, MUIMASTER_BASE_NAME)

#define MUI_MakeObjectA(type, params) \
	LP2(0x78, Object *, MUI_MakeObjectA, LONG, type, d0, ULONG *, params, a0, \
	, MUIMASTER_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define MUI_MakeObject(a0, tags...) \
	({ULONG _tags[] = { tags }; MUI_MakeObjectA((a0), (ULONG *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define MUI_NewObjectA(class, tags) \
	LP2(0x1e, Object *, MUI_NewObjectA, char *, class, a0, struct TagItem *, tags, a1, \
	, MUIMASTER_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define MUI_NewObject(a0, tags...) \
	({ULONG _tags[] = { tags }; MUI_NewObjectA((a0), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define MUI_ObtainPen(mri, spec, flags) \
	LP3(0x9c, LONG, MUI_ObtainPen, struct MUI_RenderInfo *, mri, a0, struct MUI_PenSpec *, spec, a1, ULONG, flags, d0, \
	, MUIMASTER_BASE_NAME)

#define MUI_Redraw(obj, flags) \
	LP2NR(0x66, MUI_Redraw, Object *, obj, a0, ULONG, flags, d0, \
	, MUIMASTER_BASE_NAME)

#define MUI_RejectIDCMP(obj, flags) \
	LP2NR(0x60, MUI_RejectIDCMP, Object *, obj, a0, ULONG, flags, d0, \
	, MUIMASTER_BASE_NAME)

#define MUI_ReleasePen(mri, pen) \
	LP2NR(0xa2, MUI_ReleasePen, struct MUI_RenderInfo *, mri, a0, LONG, pen, d0, \
	, MUIMASTER_BASE_NAME)

#define MUI_RemoveClipRegion(mri, region) \
	LP2NR(0xba, MUI_RemoveClipRegion, struct MUI_RenderInfo *, mri, a0, APTR, region, a1, \
	, MUIMASTER_BASE_NAME)

#define MUI_RemoveClipping(mri, h) \
	LP2NR(0xae, MUI_RemoveClipping, struct MUI_RenderInfo *, mri, a0, APTR, h, a1, \
	, MUIMASTER_BASE_NAME)

#define MUI_RequestA(app, win, flags, title, gadgets, format, params) \
	LP7(0x2a, LONG, MUI_RequestA, APTR, app, d0, APTR, win, d1, LONGBITS, flags, d2, char *, title, a0, char *, gadgets, a1, char *, format, a2, APTR, params, a3, \
	, MUIMASTER_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define MUI_Request(a0, a1, a2, a3, a4, a5, tags...) \
	({ULONG _tags[] = { tags }; MUI_RequestA((a0), (a1), (a2), (a3), (a4), (a5), (APTR)_tags);})
#endif /* !NO_INLINE_STDARG */

#define MUI_RequestIDCMP(obj, flags) \
	LP2NR(0x5a, MUI_RequestIDCMP, Object *, obj, a0, ULONG, flags, d0, \
	, MUIMASTER_BASE_NAME)

#define MUI_SetError(errnum) \
	LP1(0x48, LONG, MUI_SetError, LONG, errnum, d0, \
	, MUIMASTER_BASE_NAME)

#endif /* !_INLINE_MUIMASTER_H */
