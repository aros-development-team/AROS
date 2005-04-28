/* Automatically generated header! Do not edit! */

#ifndef _PPCINLINE_MUIMASTER_H
#define _PPCINLINE_MUIMASTER_H

#ifndef __PPCINLINE_MACROS_H
#include <ppcinline/macros.h>
#endif /* !__PPCINLINE_MACROS_H */

#ifndef MUIMASTER_BASE_NAME
#define MUIMASTER_BASE_NAME MUIMasterBase
#endif /* !MUIMASTER_BASE_NAME */

#define MUI_ObtainPen(__p0, __p1, __p2) \
	LP3(156, LONG , MUI_ObtainPen, \
		struct MUI_RenderInfo *, __p0, a0, \
		struct MUI_PenSpec *, __p1, a1, \
		ULONG , __p2, d0, \
		, MUIMASTER_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define MUI_RemoveClipping(__p0, __p1) \
	LP2NR(174, MUI_RemoveClipping, \
		struct MUI_RenderInfo *, __p0, a0, \
		APTR , __p1, a1, \
		, MUIMASTER_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define MUI_ReleasePen(__p0, __p1) \
	LP2NR(162, MUI_ReleasePen, \
		struct MUI_RenderInfo *, __p0, a0, \
		LONG , __p1, d0, \
		, MUIMASTER_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define MUI_EndRefresh(__p0, __p1) \
	LP2NR(198, MUI_EndRefresh, \
		struct MUI_RenderInfo *, __p0, a0, \
		ULONG , __p1, d0, \
		, MUIMASTER_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define MUI_AllocAslRequest(__p0, __p1) \
	LP2(48, APTR , MUI_AllocAslRequest, \
		unsigned long , __p0, d0, \
		struct TagItem *, __p1, a0, \
		, MUIMASTER_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define MUI_DisposeObject(__p0) \
	LP1NR(36, MUI_DisposeObject, \
		Object *, __p0, a0, \
		, MUIMASTER_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define MUI_SetError(__p0) \
	LP1(72, LONG , MUI_SetError, \
		LONG , __p0, d0, \
		, MUIMASTER_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define MUI_RemoveClipRegion(__p0, __p1) \
	LP2NR(186, MUI_RemoveClipRegion, \
		struct MUI_RenderInfo *, __p0, a0, \
		APTR , __p1, a1, \
		, MUIMASTER_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define MUI_Layout(__p0, __p1, __p2, __p3, __p4, __p5) \
	LP6(126, BOOL , MUI_Layout, \
		Object *, __p0, a0, \
		LONG , __p1, d0, \
		LONG , __p2, d1, \
		LONG , __p3, d2, \
		LONG , __p4, d3, \
		ULONG , __p5, d4, \
		, MUIMASTER_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define MUI_NewObjectA(__p0, __p1) \
	LP2(30, Object *, MUI_NewObjectA, \
		char *, __p0, a0, \
		struct TagItem *, __p1, a1, \
		, MUIMASTER_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define MUI_Redraw(__p0, __p1) \
	LP2NR(102, MUI_Redraw, \
		Object *, __p0, a0, \
		ULONG , __p1, d0, \
		, MUIMASTER_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define MUI_AslRequest(__p0, __p1) \
	LP2(54, BOOL , MUI_AslRequest, \
		APTR , __p0, a0, \
		struct TagItem *, __p1, a1, \
		, MUIMASTER_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define MUI_BeginRefresh(__p0, __p1) \
	LP2(192, BOOL , MUI_BeginRefresh, \
		struct MUI_RenderInfo *, __p0, a0, \
		ULONG , __p1, d0, \
		, MUIMASTER_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define MUI_CreateCustomClass(__p0, __p1, __p2, __p3, __p4) \
	LP5(108, struct MUI_CustomClass *, MUI_CreateCustomClass, \
		struct Library *, __p0, a0, \
		char *, __p1, a1, \
		struct MUI_CustomClass *, __p2, a2, \
		int , __p3, d0, \
		APTR , __p4, a3, \
		, MUIMASTER_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define MUI_RequestA(__p0, __p1, __p2, __p3, __p4, __p5, __p6) \
	LP7(42, LONG , MUI_RequestA, \
		APTR , __p0, d0, \
		APTR , __p1, d1, \
		LONGBITS , __p2, d2, \
		char *, __p3, a0, \
		char *, __p4, a1, \
		char *, __p5, a2, \
		APTR , __p6, a3, \
		, MUIMASTER_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define MUI_Error() \
	LP0(66, LONG , MUI_Error, \
		, MUIMASTER_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define MUI_AddClipRegion(__p0, __p1) \
	LP2(180, APTR , MUI_AddClipRegion, \
		struct MUI_RenderInfo *, __p0, a0, \
		struct Region *, __p1, a1, \
		, MUIMASTER_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define MUI_RequestIDCMP(__p0, __p1) \
	LP2NR(90, MUI_RequestIDCMP, \
		Object *, __p0, a0, \
		ULONG , __p1, d0, \
		, MUIMASTER_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define MUI_FreeClass(__p0) \
	LP1NR(84, MUI_FreeClass, \
		struct IClass *, __p0, a0, \
		, MUIMASTER_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define MUI_AddClipping(__p0, __p1, __p2, __p3, __p4) \
	LP5(168, APTR , MUI_AddClipping, \
		struct MUI_RenderInfo *, __p0, a0, \
		WORD , __p1, d0, \
		WORD , __p2, d1, \
		WORD , __p3, d2, \
		WORD , __p4, d3, \
		, MUIMASTER_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define MUI_GetClass(__p0) \
	LP1(78, struct IClass *, MUI_GetClass, \
		char *, __p0, a0, \
		, MUIMASTER_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define MUI_RejectIDCMP(__p0, __p1) \
	LP2NR(96, MUI_RejectIDCMP, \
		Object *, __p0, a0, \
		ULONG , __p1, d0, \
		, MUIMASTER_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define MUI_FreeAslRequest(__p0) \
	LP1NR(60, MUI_FreeAslRequest, \
		APTR , __p0, a0, \
		, MUIMASTER_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define MUI_MakeObjectA(__p0, __p1) \
	LP2(120, Object *, MUI_MakeObjectA, \
		LONG , __p0, d0, \
		ULONG *, __p1, a0, \
		, MUIMASTER_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define MUI_DeleteCustomClass(__p0) \
	LP1(114, BOOL , MUI_DeleteCustomClass, \
		struct MUI_CustomClass *, __p0, a0, \
		, MUIMASTER_BASE_NAME, 0, 0, 0, 0, 0, 0)

#ifdef USE_INLINE_STDARG

#include <stdarg.h>

#define MUI_AslRequestTags(__p0, ...) \
	({ULONG _tags[] = { __VA_ARGS__ }; \
	MUI_AslRequest(__p0, (struct TagItem *)_tags);})

#define MUI_AllocAslRequestTags(__p0, ...) \
	({ULONG _tags[] = { __VA_ARGS__ }; \
	MUI_AllocAslRequest(__p0, (struct TagItem *)_tags);})

#define MUI_MakeObject(__p0, ...) \
	({ULONG _tags[] = { __VA_ARGS__ }; \
	MUI_MakeObjectA(__p0, (ULONG *)_tags);})

#define MUI_NewObject(__p0, ...) \
	({ULONG _tags[] = { __VA_ARGS__ }; \
	MUI_NewObjectA(__p0, (struct TagItem *)_tags);})

#define MUI_Request(__p0, __p1, __p2, __p3, __p4, __p5, ...) \
	({ULONG _tags[] = { __VA_ARGS__ }; \
	MUI_RequestA(__p0, __p1, __p2, __p3, __p4, __p5, (APTR )_tags);})

#endif

#endif /* !_PPCINLINE_MUIMASTER_H */
