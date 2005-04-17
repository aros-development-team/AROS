#ifndef _VBCCINLINE_MUIMASTER_H
#define _VBCCINLINE_MUIMASTER_H

Object * __MUI_NewObjectA(__reg("a0") char * a0arg, __reg("a1") struct TagItem * tags, __reg("a6") void *)="\tjsr\t-30(a6)";
#define MUI_NewObjectA(a0arg, tags) __MUI_NewObjectA((a0arg), (tags), MUIMasterBase)

VOID __MUI_DisposeObject(__reg("a0") Object * obj, __reg("a6") void *)="\tjsr\t-36(a6)";
#define MUI_DisposeObject(obj) __MUI_DisposeObject((obj), MUIMasterBase)

LONG __MUI_RequestA(__reg("d0") APTR app, __reg("d1") APTR win, __reg("d2") LONGBITS flags, __reg("a0") char * title, __reg("a1") char * gadgets, __reg("a2") char * format, __reg("a3") APTR params, __reg("a6") void *)="\tjsr\t-42(a6)";
#define MUI_RequestA(app, win, flags, title, gadgets, format, params) __MUI_RequestA((app), (win), (flags), (title), (gadgets), (format), (params), MUIMasterBase)

APTR __MUI_AllocAslRequest(__reg("d0") unsigned long type, __reg("a0") struct TagItem * tags, __reg("a6") void *)="\tjsr\t-48(a6)";
#define MUI_AllocAslRequest(type, tags) __MUI_AllocAslRequest((type), (tags), MUIMasterBase)

BOOL __MUI_AslRequest(__reg("a0") APTR req, __reg("a1") struct TagItem * tags, __reg("a6") void *)="\tjsr\t-54(a6)";
#define MUI_AslRequest(req, tags) __MUI_AslRequest((req), (tags), MUIMasterBase)

VOID __MUI_FreeAslRequest(__reg("a0") APTR req, __reg("a6") void *)="\tjsr\t-60(a6)";
#define MUI_FreeAslRequest(req) __MUI_FreeAslRequest((req), MUIMasterBase)

LONG __MUI_Error(__reg("a6") void *)="\tjsr\t-66(a6)";
#define MUI_Error() __MUI_Error(MUIMasterBase)

LONG __MUI_SetError(__reg("d0") LONG errnum, __reg("a6") void *)="\tjsr\t-72(a6)";
#define MUI_SetError(errnum) __MUI_SetError((errnum), MUIMasterBase)

struct IClass * __MUI_GetClass(__reg("a0") char * name, __reg("a6") void *)="\tjsr\t-78(a6)";
#define MUI_GetClass(name) __MUI_GetClass((name), MUIMasterBase)

VOID __MUI_FreeClass(__reg("a0") struct IClass * cl, __reg("a6") void *)="\tjsr\t-84(a6)";
#define MUI_FreeClass(cl) __MUI_FreeClass((cl), MUIMasterBase)

VOID __MUI_RequestIDCMP(__reg("a0") Object * obj, __reg("d0") ULONG flags, __reg("a6") void *)="\tjsr\t-90(a6)";
#define MUI_RequestIDCMP(obj, flags) __MUI_RequestIDCMP((obj), (flags), MUIMasterBase)

VOID __MUI_RejectIDCMP(__reg("a0") Object * obj, __reg("d0") ULONG flags, __reg("a6") void *)="\tjsr\t-96(a6)";
#define MUI_RejectIDCMP(obj, flags) __MUI_RejectIDCMP((obj), (flags), MUIMasterBase)

VOID __MUI_Redraw(__reg("a0") Object * obj, __reg("d0") ULONG flags, __reg("a6") void *)="\tjsr\t-102(a6)";
#define MUI_Redraw(obj, flags) __MUI_Redraw((obj), (flags), MUIMasterBase)

struct MUI_CustomClass * __MUI_CreateCustomClass(__reg("a0") struct Library * base, __reg("a1") char * supername, __reg("a2") struct MUI_CustomClass * supermcc, __reg("d0") int datasize, __reg("a3") APTR dispatcher, __reg("a6") void *)="\tjsr\t-108(a6)";
#define MUI_CreateCustomClass(base, supername, supermcc, datasize, dispatcher) __MUI_CreateCustomClass((base), (supername), (supermcc), (datasize), (dispatcher), MUIMasterBase)

BOOL __MUI_DeleteCustomClass(__reg("a0") struct MUI_CustomClass * mcc, __reg("a6") void *)="\tjsr\t-114(a6)";
#define MUI_DeleteCustomClass(mcc) __MUI_DeleteCustomClass((mcc), MUIMasterBase)

Object * __MUI_MakeObjectA(__reg("d0") LONG type, __reg("a0") ULONG * params, __reg("a6") void *)="\tjsr\t-120(a6)";
#define MUI_MakeObjectA(type, params) __MUI_MakeObjectA((type), (params), MUIMasterBase)

BOOL __MUI_Layout(__reg("a0") Object * obj, __reg("d0") LONG l, __reg("d1") LONG t, __reg("d2") LONG w, __reg("d3") LONG h, __reg("d4") ULONG flags, __reg("a6") void *)="\tjsr\t-126(a6)";
#define MUI_Layout(obj, l, t, w, h, flags) __MUI_Layout((obj), (l), (t), (w), (h), (flags), MUIMasterBase)

LONG __MUI_ObtainPen(__reg("a0") struct MUI_RenderInfo * mri, __reg("a1") struct MUI_PenSpec * spec, __reg("d0") ULONG flags, __reg("a6") void *)="\tjsr\t-156(a6)";
#define MUI_ObtainPen(mri, spec, flags) __MUI_ObtainPen((mri), (spec), (flags), MUIMasterBase)

VOID __MUI_ReleasePen(__reg("a0") struct MUI_RenderInfo * mri, __reg("d0") LONG pen, __reg("a6") void *)="\tjsr\t-162(a6)";
#define MUI_ReleasePen(mri, pen) __MUI_ReleasePen((mri), (pen), MUIMasterBase)

APTR __MUI_AddClipping(__reg("a0") struct MUI_RenderInfo * mri, __reg("d0") WORD l, __reg("d1") WORD t, __reg("d2") WORD w, __reg("d3") WORD h, __reg("a6") void *)="\tjsr\t-168(a6)";
#define MUI_AddClipping(mri, l, t, w, h) __MUI_AddClipping((mri), (l), (t), (w), (h), MUIMasterBase)

VOID __MUI_RemoveClipping(__reg("a0") struct MUI_RenderInfo * mri, __reg("a1") APTR h, __reg("a6") void *)="\tjsr\t-174(a6)";
#define MUI_RemoveClipping(mri, h) __MUI_RemoveClipping((mri), (h), MUIMasterBase)

APTR __MUI_AddClipRegion(__reg("a0") struct MUI_RenderInfo * mri, __reg("a1") struct Region * region, __reg("a6") void *)="\tjsr\t-180(a6)";
#define MUI_AddClipRegion(mri, region) __MUI_AddClipRegion((mri), (region), MUIMasterBase)

VOID __MUI_RemoveClipRegion(__reg("a0") struct MUI_RenderInfo * mri, __reg("a1") APTR region, __reg("a6") void *)="\tjsr\t-186(a6)";
#define MUI_RemoveClipRegion(mri, region) __MUI_RemoveClipRegion((mri), (region), MUIMasterBase)

BOOL __MUI_BeginRefresh(__reg("a0") struct MUI_RenderInfo * mri, __reg("d0") ULONG flags, __reg("a6") void *)="\tjsr\t-192(a6)";
#define MUI_BeginRefresh(mri, flags) __MUI_BeginRefresh((mri), (flags), MUIMasterBase)

VOID __MUI_EndRefresh(__reg("a0") struct MUI_RenderInfo * mri, __reg("d0") ULONG flags, __reg("a6") void *)="\tjsr\t-198(a6)";
#define MUI_EndRefresh(mri, flags) __MUI_EndRefresh((mri), (flags), MUIMasterBase)

#endif /*  _VBCCINLINE_MUIMASTER_H  */
