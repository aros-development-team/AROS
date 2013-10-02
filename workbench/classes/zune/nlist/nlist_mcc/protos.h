/***************************************************************************

 NList.mcc - New List MUI Custom Class
 Registered MUI class, Serial Number: 1d51 0x9d510030 to 0x9d5100A0
                                           0x9d5100C0 to 0x9d5100FF

 Copyright (C) 1996-2001 by Gilles Masson
 Copyright (C) 2001-2013 by NList Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 NList classes Support Site:  http://www.sf.net/projects/nlist-classes

 $Id$

***************************************************************************/

/* NList_mcc.c */

extern void release_pen(Object *obj, IPTR *pen);
extern void obtain_pen(Object *obj, IPTR *pen, struct MUI_PenSpec *ps);
extern IPTR mNL_New(struct IClass *cl,Object *obj,struct opSet *msg);
extern IPTR mNL_Dispose(struct IClass *cl,Object *obj,Msg msg);
extern IPTR mNL_Setup(struct IClass *cl,Object *obj,struct MUIP_Setup *msg);
extern IPTR mNL_Cleanup(struct IClass *cl,Object *obj,struct MUIP_Cleanup *msg);

/* NList_mcc0.c */

/*
**	Dispatcher
*/

/* NList_mcc1.c */

extern IPTR mNL_AskMinMax(struct IClass *cl,Object *obj,struct MUIP_AskMinMax *msg);
extern IPTR mNL_Notify(struct IClass *cl,Object *obj,struct MUIP_Notify *msg);
extern IPTR mNL_Set(struct IClass *cl,Object *obj,Msg msg);
extern IPTR mNL_Get(struct IClass *cl,Object *obj,struct opGet *msg);

/* NList_mcc2.c */

extern IPTR mNL_HandleEvent(struct IClass *cl,Object *obj,struct MUIP_HandleInput *msg);
extern IPTR mNL_CreateDragImage(struct IClass *cl,Object *obj,struct MUIP_CreateDragImage *msg);
extern IPTR mNL_DeleteDragImage(struct IClass *cl,Object *obj,struct MUIP_DeleteDragImage *msg);
extern BOOL  NL_Prop_First_Adjust(struct NLData *data);
extern IPTR mNL_Trigger(struct IClass *cl,Object *obj,Msg msg);

/* NList_mcc3.c */

extern void NL_SetObjInfos(struct NLData *data,BOOL setall);
extern IPTR mNL_Draw(struct IClass *cl,Object *obj,struct MUIP_Draw *msg);
extern IPTR mNL_DropDraw(struct IClass *cl,Object *obj,struct MUIP_NList_DropDraw *msg);

/* NList_mcc4.c */

extern BOOL DontDoColumn(struct NLData *data,LONG ent,WORD column);
extern void ParseColumn(struct NLData *data,WORD column,IPTR mypen);
extern void WidthColumn(struct NLData *data,WORD column,WORD updinfo);
extern void AllParseColumns(struct NLData *data);
extern void FreeAffInfo(struct NLData *data);
extern BOOL NeedAffInfo(struct NLData *data,WORD niask);
extern void NL_GetDisplayArray(struct NLData *data,SIPTR ent);
extern void FindCharInColumn(struct NLData *data,LONG ent,WORD column,WORD xoffset,WORD *charxoffset,WORD *charnum);
extern void NL_DoWrapAll(struct NLData *data,BOOL force,BOOL update);
extern void AllWidthColumns(struct NLData *data);
extern void NL_SetColsAdd(struct NLData *data,LONG ent,WORD addimages);
extern void NL_SetColsRem(struct NLData *data,LONG ent);

/* NList_mcc5.c */

extern void NL_SetCols(struct NLData *data);
extern LONG NL_DoNotifies(struct NLData *data,LONG which);
extern void NL_UpdateScrollersValues(struct NLData *data);
extern ULONG NL_UpdateScrollers(struct NLData *data,BOOL force);
extern void NL_DrawQuietBG(struct NLData *data,LONG dowhat,LONG bg);
extern void NL_Select(struct NLData *data,LONG dowhat,LONG ent,BYTE sel);
extern void ScrollVert(struct NLData *data,WORD dy,LONG LPVisible);
extern void ScrollHoriz(struct NLData *data,WORD dx,LONG LPVisible);
extern LONG  NL_ColToColumn(struct NLData *data,LONG col);
extern LONG  NL_ColumnToCol(struct NLData *data,LONG column);
extern LONG  NL_SetCol(struct NLData *data,LONG column,LONG col);
extern LONG  NL_ColWidth(struct NLData *data,LONG col,LONG width);
extern BYTE *NL_Columns(struct NLData *data,BYTE *columns);
extern IPTR mNL_ColToColumn(struct IClass *cl,Object *obj,struct MUIP_NList_ColToColumn *msg);
extern IPTR mNL_ColumnToCol(struct IClass *cl,Object *obj,struct MUIP_NList_ColumnToCol *msg);
extern IPTR mNL_SetColumnCol(struct IClass *cl,Object *obj,struct MUIP_NList_SetColumnCol *msg);
extern IPTR mNL_List_ColWidth(struct IClass *cl,Object *obj,struct MUIP_NList_ColWidth *msg);
extern IPTR mNL_ContextMenuBuild(struct IClass *cl,Object *obj,struct MUIP_ContextMenuBuild *msg);
extern IPTR mNL_ContextMenuChoice(struct IClass *cl,Object *obj,struct MUIP_ContextMenuChoice *msg);

/* NList_mcc6.c */

extern void DrawBackground(Object *obj, LONG left, LONG top, LONG width, LONG height, LONG xoff, LONG yoff);
extern WORD DrawTitle(struct NLData *data,LONG minx,LONG maxx,WORD hfirst);
extern void DrawOldLine(struct NLData *data,LONG ent,LONG minx,LONG maxx,WORD hfirst);
extern WORD DrawLines(struct NLData *data,LONG e1,LONG e2,LONG minx,LONG maxx,WORD hfirst,WORD hmax,WORD small,BOOL do_extrems,WORD not_all);
extern LONG DrawText(struct NLData *data,LONG ent,LONG x,LONG y,LONG minx,LONG maxx,ULONG mypen,LONG dxpermit,BOOL forcepen);
extern LONG DrawDragText(struct NLData *data,BOOL draw);
extern void DisposeDragRPort(struct NLData *data);
extern struct RastPort *CreateDragRPort(struct NLData *data,LONG numlines,LONG first,LONG last);

/* NList_func.c */

extern void NL_SegChanged(struct NLData *data,LONG ent1,LONG ent2);
extern void NL_Changed(struct NLData *data,LONG ent);
extern void NL_UnSelectAll(struct NLData *data,LONG untouch_ent);
extern void UnSelectCharSel(struct NLData *data,BOOL redraw);
extern void SelectFirstPoint(struct NLData *data,WORD x,WORD y);
extern void SelectSecondPoint(struct NLData *data,WORD x,WORD y);
extern BOOL NL_List_First(struct NLData *data,LONG lf,struct TagItem *tag);
extern BOOL NL_List_Active(struct NLData *data,LONG la,struct TagItem *tag,LONG newactsel,LONG acceptsame,ULONG flags);
extern BOOL NL_List_Horiz_First(struct NLData *data,LONG hf,struct TagItem *tag);
extern ULONG NL_List_SelectChar(struct NLData *data,LONG pos,LONG seltype,LONG *state);
extern ULONG NL_List_Select(struct NLData *data,LONG pos,LONG pos2,LONG seltype,LONG *state);
extern ULONG NL_List_TestPosOld(struct NLData *data,LONG x,LONG y,struct MUI_List_TestPos_Result *res);
extern ULONG NL_List_TestPos(struct NLData *data,LONG x,LONG y,struct MUI_NList_TestPos_Result *res);

extern IPTR mNL_List_GetEntry(struct IClass *cl,Object *obj,struct  MUIP_NList_GetEntry *msg);
extern IPTR mNL_List_GetEntryInfo(struct IClass *cl,Object *obj,struct  MUIP_NList_GetEntryInfo *msg);
extern IPTR mNL_List_Jump(struct IClass *cl,Object *obj,struct  MUIP_NList_Jump *msg);
extern IPTR mNL_List_SetActive(struct IClass *cl,Object *obj,struct MUIP_NList_SetActive *msg);
extern IPTR mNL_List_Select(struct IClass *cl,Object *obj,struct  MUIP_NList_Select *msg);
extern IPTR mNL_List_TestPos(struct IClass *cl,Object *obj,struct MUIP_NList_TestPos *msg);
extern IPTR mNL_List_TestPosOld(struct IClass *cl,Object *obj,struct MUIP_List_TestPos *msg);
extern IPTR mNL_List_Redraw(struct IClass *cl,Object *obj,struct MUIP_NList_Redraw *msg);
extern IPTR mNL_List_RedrawEntry(struct IClass *cl,Object *obj,struct MUIP_NList_RedrawEntry *msg);
extern IPTR mNL_List_NextSelected(struct IClass *cl,Object *obj,struct MUIP_NList_NextSelected *msg);
extern IPTR mNL_List_PrevSelected(struct IClass *cl,Object *obj,struct MUIP_NList_PrevSelected *msg);
extern IPTR mNL_List_GetSelectInfo(struct IClass *cl,Object *obj,struct MUIP_NList_GetSelectInfo *msg);
extern IPTR mNL_List_DoMethod(struct IClass *cl,Object *obj,struct MUIP_NList_DoMethod *msg);
extern IPTR mNL_List_GetPos(struct IClass *cl,Object *obj,struct MUIP_NList_GetPos *msg);

/* NList_func2.c */

extern LONG  NL_GetSelects(struct NLData *data,LONG ent);
extern BOOL  NL_InsertTmpLine(struct NLData *data,LONG pos);
extern void  NL_DeleteTmpLine(struct NLData *data,LONG pos);
extern ULONG NL_List_Sort(struct NLData *data);
extern ULONG NL_List_Insert(struct NLData *data,APTR *entries,LONG count,LONG pos,LONG wrapcol,LONG align,ULONG flags);
extern ULONG NL_List_Replace(struct NLData *data,APTR entry,LONG pos,LONG wrapcol,LONG align);
extern ULONG NL_List_Clear(struct NLData *data);
extern ULONG NL_List_Remove(struct NLData *data,LONG pos);
extern ULONG NL_List_Exchange(struct NLData *data,LONG pos1,LONG pos2);

extern IPTR mNL_List_Sort(struct IClass *cl,Object *obj,struct  MUIP_NList_Sort *msg);
extern IPTR mNL_List_Sort2(struct IClass *cl,Object *obj,struct  MUIP_NList_Sort2 *msg);
extern IPTR mNL_List_Sort3(struct IClass *cl,Object *obj,struct  MUIP_NList_Sort3 *msg);
extern IPTR mNL_List_Insert(struct IClass *cl,Object *obj,struct  MUIP_NList_Insert *msg);
extern IPTR mNL_List_InsertSingle(struct IClass *cl,Object *obj,struct  MUIP_NList_InsertSingle *msg);
extern IPTR mNL_List_InsertWrap(struct IClass *cl,Object *obj,struct  MUIP_NList_InsertWrap *msg);
extern IPTR mNL_List_InsertSingleWrap(struct IClass *cl,Object *obj,struct  MUIP_NList_InsertSingleWrap *msg);
extern IPTR mNL_List_ReplaceSingle(struct IClass *cl,Object *obj,struct  MUIP_NList_ReplaceSingle *msg);
extern IPTR mNL_List_Exchange(struct IClass *cl,Object *obj,struct MUIP_NList_Exchange *msg);
extern IPTR mNL_List_Move(struct IClass *cl,Object *obj,struct MUIP_NList_Move *msg);
extern IPTR mNL_List_Clear(struct IClass *cl,Object *obj,struct  MUIP_NList_Clear *msg);
extern IPTR mNL_List_Remove(struct IClass *cl,Object *obj,struct MUIP_NList_Remove *msg);
extern IPTR mNL_DragQuery(struct IClass *cl,Object *obj,struct MUIP_DragQuery *msg);
extern IPTR mNL_DragBegin(struct IClass *cl,Object *obj,struct MUIP_DragBegin *msg);
extern IPTR mNL_DragReport(struct IClass *cl,Object *obj,struct MUIP_DragReport *msg);
extern IPTR mNL_DragFinish(struct IClass *cl,Object *obj,struct MUIP_DragFinish *msg);
extern IPTR mNL_DragDrop(struct IClass *cl,Object *obj,struct MUIP_DragDrop *msg);
extern IPTR mNL_DropType(struct IClass *cl,Object *obj,struct MUIP_NList_DropType *msg);
extern IPTR mNL_DropEntryDrawErase(struct IClass *cl,Object *obj,struct MUIP_NList_DropEntryDrawErase *msg);

/* NList_func3.c */
extern IPTR MyCallHookPkt(Object *obj,BOOL hdata,struct Hook *hook,APTR object,APTR message);
#ifdef __AROS__
#define MyCallHookPktA(obj, hook, ...) \
 ({ IPTR __args[] = { AROS_PP_VARIADIC_CAST2IPTR(__VA_ARGS__) }; \
    CallHookPkt(hook, obj, __args); })
#else
extern IPTR STDARGS VARARGS68K MyCallHookPktA(Object *obj, struct Hook *hook, ...);
#endif
extern LONG DeadKeyConvert(struct NLData *data,struct IntuiMessage *msg,STRPTR buf,LONG bufsize,struct KeyMap *kmap);
extern char *ltoa(ULONG val, char *buffer, int len);

extern void NL_Free_Format(struct NLData *data);
extern BOOL NL_Read_Format(struct NLData *data,char *strformat,BOOL oldlist);
extern SIPTR NL_CopyTo(struct NLData *data,LONG pos,char *filename,ULONG clipnum,APTR *entries,struct Hook *hook);

extern IPTR mNL_CopyToClip(struct IClass *cl,Object *obj,struct MUIP_NList_CopyToClip *msg);
extern IPTR mNL_CopyTo(struct IClass *cl,Object *obj,struct MUIP_NList_CopyTo *msg);

/* NList_func4.c */

extern BOOL NL_OnWindow(struct NLData *data,LONG x,LONG y);
extern struct NImgList *GetNImage(struct NLData *data,char *ImgName);
extern void DeleteNImages(struct NLData *data);
extern struct NImgList *GetNImage2(struct NLData *data,APTR imgobj);
extern void DeleteNImages2(struct NLData *data);
extern void GetNImage_Sizes(struct NLData *data);
extern void GetNImage_End(struct NLData *data);
extern void GetImages(struct NLData *data);

extern IPTR NL_CreateImage(struct NLData *data,Object *imgobj,ULONG flags);
extern ULONG NL_DeleteImage(struct NLData *data,APTR listimg);
extern ULONG NL_CreateImages(struct NLData *data);
extern ULONG NL_DeleteImages(struct NLData *data);
extern ULONG NL_UseImage(struct NLData *data,Object *imgobj,LONG imgnum,ULONG flags);
extern IPTR mNL_CreateImage(struct IClass *cl,Object *obj,struct MUIP_NList_CreateImage *msg);
extern IPTR mNL_DeleteImage(struct IClass *cl,Object *obj,struct MUIP_NList_DeleteImage *msg);
extern IPTR mNL_UseImage(struct IClass *cl,Object *obj,struct MUIP_NList_UseImage *msg);

/* Move.c */

extern void NL_Move(struct TypeEntry **dest, struct TypeEntry **src, LONG count, LONG newpos);
extern void NL_MoveD(struct TypeEntry **dest, struct TypeEntry **src, LONG count, LONG newpos);

// ClipboardServer.c
BOOL StartClipboardServer(void);
void ShutdownClipboardServer(void);
LONG StringToClipboard(ULONG unit, STRPTR str);
