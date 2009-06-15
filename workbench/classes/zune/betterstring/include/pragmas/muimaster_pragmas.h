#ifndef __STORM__

#pragma libcall MUIMasterBase MUI_NewObjectA 1e 9802
#ifdef __SASC_60
#pragma tagcall MUIMasterBase MUI_NewObject 1e 9802
#endif
#pragma libcall MUIMasterBase MUI_DisposeObject 24 801
#pragma libcall MUIMasterBase MUI_RequestA 2a BA9821007
#ifdef __SASC_60
#pragma tagcall MUIMasterBase MUI_Request 2a BA9821007
#endif
#pragma libcall MUIMasterBase MUI_AllocAslRequest 30 8002
#ifdef __SASC_60
#pragma tagcall MUIMasterBase MUI_AllocAslRequestTags 30 8002
#endif
#pragma libcall MUIMasterBase MUI_AslRequest 36 9802
#ifdef __SASC_60
#pragma tagcall MUIMasterBase MUI_AslRequestTags 36 9802
#endif
#pragma libcall MUIMasterBase MUI_FreeAslRequest 3c 801
#pragma libcall MUIMasterBase MUI_Error 42 0
#pragma libcall MUIMasterBase MUI_SetError 48 001
#pragma libcall MUIMasterBase MUI_GetClass 4e 801
#pragma libcall MUIMasterBase MUI_FreeClass 54 801
#pragma libcall MUIMasterBase MUI_RequestIDCMP 5a 0802
#pragma libcall MUIMasterBase MUI_RejectIDCMP 60 0802
#pragma libcall MUIMasterBase MUI_Redraw 66 0802
#pragma libcall MUIMasterBase MUI_CreateCustomClass 6c B0A9805
#pragma libcall MUIMasterBase MUI_DeleteCustomClass 72 801
#pragma libcall MUIMasterBase MUI_MakeObjectA 78 8002
#ifdef __SASC_60
#pragma tagcall MUIMasterBase MUI_MakeObject 78 8002
#endif
#pragma libcall MUIMasterBase MUI_Layout 7e 43210806
#pragma libcall MUIMasterBase MUI_ObtainPen 9c 09803
#pragma libcall MUIMasterBase MUI_ReleasePen a2 0802
#pragma libcall MUIMasterBase MUI_AddClipping a8 3210805
#pragma libcall MUIMasterBase MUI_RemoveClipping ae 9802
#pragma libcall MUIMasterBase MUI_AddClipRegion b4 9802
#pragma libcall MUIMasterBase MUI_RemoveClipRegion ba 9802
#pragma libcall MUIMasterBase MUI_BeginRefresh c0 0802
#pragma libcall MUIMasterBase MUI_EndRefresh c6 0802

#ifdef __MAXON__
#pragma amicall(MUIMasterBase,0x1e,MUI_NewObjectA(a0,a1))
#pragma amicall(MUIMasterBase,0x24,MUI_DisposeObject(a0))
#pragma amicall(MUIMasterBase,0x2a,MUI_RequestA(d0,d1,d2,a0,a1,a2,a3))
#pragma amicall(MUIMasterBase,0x30,MUI_AllocAslRequest(d0,a0))
#pragma amicall(MUIMasterBase,0x36,MUI_AslRequest(a0,a1))
#pragma amicall(MUIMasterBase,0x3c,MUI_FreeAslRequest(a0))
#pragma amicall(MUIMasterBase,0x42,MUI_Error())
#pragma amicall(MUIMasterBase,0x48,MUI_SetError(d0))
#pragma amicall(MUIMasterBase,0x4e,MUI_GetClass(a0))
#pragma amicall(MUIMasterBase,0x54,MUI_FreeClass(a0))
#pragma amicall(MUIMasterBase,0x5a,MUI_RequestIDCMP(a0,d0))
#pragma amicall(MUIMasterBase,0x60,MUI_RejectIDCMP(a0,d0))
#pragma amicall(MUIMasterBase,0x66,MUI_Redraw(a0,d0))
#endif

#else

#ifndef _INCLUDE_PRAGMA_MUIMASTER_LIB_H
#include <pragma/muimaster_lib.h>
#endif

#endif
