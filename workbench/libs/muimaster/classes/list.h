/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _MUI_CLASSES_LIST_H
#define _MUI_CLASSES_LIST_H

/****************************************************************************/
/** List                                                                   **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_List[];
#else
#define MUIC_List "List.mui"
#endif

/* Methods */

#define MUIM_List_Clear                     0x8042ad89 /* V4  */
#define MUIM_List_CreateImage               0x80429804 /* V11 */
#define MUIM_List_DeleteImage               0x80420f58 /* V11 */
#define MUIM_List_Exchange                  0x8042468c /* V4  */
#define MUIM_List_GetEntry                  0x804280ec /* V4  */
#define MUIM_List_Insert                    0x80426c87 /* V4  */
#define MUIM_List_InsertSingle              0x804254d5 /* V7  */
#define MUIM_List_Jump                      0x8042baab /* V4  */
#define MUIM_List_Move                      0x804253c2 /* V9  */
#define MUIM_List_NextSelected              0x80425f17 /* V6  */
#define MUIM_List_Redraw                    0x80427993 /* V4  */
#define MUIM_List_Remove                    0x8042647e /* V4  */
#define MUIM_List_Select                    0x804252d8 /* V4  */
#define MUIM_List_Sort                      0x80422275 /* V4  */
#define MUIM_List_TestPos                   0x80425f48 /* V11 */
#define MUIM_List_Construct                 0x9d5100A1 /* ZV1 GM same like NList, PRIV for now! */
#define MUIM_List_Destruct                  0x9d5100A2 /* ZV1 GM same like NList, PRIV for now! */
#define MUIM_List_Compare                   0x9d5100A3 /* Zv1 GM same like NList, PRIV for now! */
#define MUIM_List_Display                   0x9d5100A4 /* Zv1 GM same like NList, PRIV for now! */

struct  MUIP_List_Clear                     { ULONG MethodID; };
struct  MUIP_List_CreateImage               { ULONG MethodID; Object *obj; ULONG flags; };
struct  MUIP_List_DeleteImage               { ULONG MethodID; APTR listimg; };
struct  MUIP_List_Exchange                  { ULONG MethodID; LONG pos1; LONG pos2; };
struct  MUIP_List_GetEntry                  { ULONG MethodID; LONG pos; APTR *entry; };
struct  MUIP_List_Insert                    { ULONG MethodID; APTR *entries; LONG count; LONG pos; };
struct  MUIP_List_InsertSingle              { ULONG MethodID; APTR entry; LONG pos; };
struct  MUIP_List_Jump                      { ULONG MethodID; LONG pos; };
struct  MUIP_List_Move                      { ULONG MethodID; LONG from; LONG to; };
struct  MUIP_List_NextSelected              { ULONG MethodID; LONG *pos; };
struct  MUIP_List_Redraw                    { ULONG MethodID; LONG pos; };
struct  MUIP_List_Remove                    { ULONG MethodID; LONG pos; };
struct  MUIP_List_Select                    { ULONG MethodID; LONG pos; LONG seltype; LONG *state; };
struct  MUIP_List_Sort                      { ULONG MethodID; };
struct  MUIP_List_TestPos                   { ULONG MethodID; LONG x; LONG y; struct MUI_List_TestPos_Result *res; };
struct  MUIP_List_Construct                 { ULONG MethodID; APTR entry; APTR pool; };
struct  MUIP_List_Destruct                  { ULONG MethodID; APTR entry; APTR pool; };
struct  MUIP_List_Compare                   { ULONG MethodID; APTR entry1; APTR entry2; LONG sort_type1; LONG sort_type2; };
struct  MUIP_List_Display                   { ULONG MethodID; APTR entry; LONG entry_pos; STRPTR *strings; STRPTR *preparses; };

/* Attributes */

#define MUIA_List_Active                    0x8042391c /* V4  isg LONG              */
#define MUIA_List_AdjustHeight              0x8042850d /* V4  i.. BOOL              */
#define MUIA_List_AdjustWidth               0x8042354a /* V4  i.. BOOL              */
#define MUIA_List_AutoVisible               0x8042a445 /* V11 isg BOOL              */
#define MUIA_List_CompareHook               0x80425c14 /* V4  is. struct Hook *     */
#define MUIA_List_ConstructHook             0x8042894f /* V4  is. struct Hook *     */
#define MUIA_List_DestructHook              0x804297ce /* V4  is. struct Hook *     */
#define MUIA_List_DisplayHook               0x8042b4d5 /* V4  is. struct Hook *     */
#define MUIA_List_DragSortable              0x80426099 /* V11 isg BOOL              */
#define MUIA_List_DropMark                  0x8042aba6 /* V11 ..g LONG              */
#define MUIA_List_Entries                   0x80421654 /* V4  ..g LONG              */
#define MUIA_List_First                     0x804238d4 /* V4  ..g LONG              */
#define MUIA_List_Format                    0x80423c0a /* V4  isg STRPTR            */
#define MUIA_List_InsertPosition            0x8042d0cd /* V9  ..g LONG              */
#define MUIA_List_MinLineHeight             0x8042d1c3 /* V4  i.. LONG              */
#define MUIA_List_MultiTestHook             0x8042c2c6 /* V4  is. struct Hook *     */
#define MUIA_List_Pool                      0x80423431 /* V13 i.. APTR              */
#define MUIA_List_PoolPuddleSize            0x8042a4eb /* V13 i.. ULONG             */
#define MUIA_List_PoolThreshSize            0x8042c48c /* V13 i.. ULONG             */
#define MUIA_List_Quiet                     0x8042d8c7 /* V4  .s. BOOL              */
#define MUIA_List_ShowDropMarks             0x8042c6f3 /* V11 isg BOOL              */
#define MUIA_List_SourceArray               0x8042c0a0 /* V4  i.. APTR              */
#define MUIA_List_Title                     0x80423e66 /* V6  isg char *            */
#define MUIA_List_Visible                   0x8042191f /* V4  ..g LONG              */

#define MUIV_List_Active_Off -1
#define MUIV_List_Active_Top -2
#define MUIV_List_Active_Bottom -3
#define MUIV_List_Active_Up -4
#define MUIV_List_Active_Down -5
#define MUIV_List_Active_PageUp -6
#define MUIV_List_Active_PageDown -7
#define MUIV_List_ConstructHook_String -1
#define MUIV_List_CopyHook_String -1
#define MUIV_List_CursorType_None 0
#define MUIV_List_CursorType_Bar 1
#define MUIV_List_CursorType_Rect 2
#define MUIV_List_DestructHook_String -1

#define MUIV_List_Insert_Top             0
#define MUIV_List_Insert_Active         -1
#define MUIV_List_Insert_Sorted         -2
#define MUIV_List_Insert_Bottom         -3

#define MUIV_List_Remove_First           0
#define MUIV_List_Remove_Active         -1
#define MUIV_List_Remove_Last           -2
#define MUIV_List_Remove_Selected       -3

#define MUIV_List_Select_Off             0
#define MUIV_List_Select_On              1
#define MUIV_List_Select_Toggle          2
#define MUIV_List_Select_Ask             3

#define MUIV_List_GetEntry_Active       -1
#define MUIV_List_Select_Active         -1
#define MUIV_List_Select_All            -2

#define MUIV_List_Redraw_Active         -1
#define MUIV_List_Redraw_All            -2

#define MUIV_List_Move_Top               0
#define MUIV_List_Move_Active           -1
#define MUIV_List_Move_Bottom           -2
#define MUIV_List_Move_Next             -3 /* only valid for second parameter */
#define MUIV_List_Move_Previous         -4 /* only valid for second parameter */

#define MUIV_List_Exchange_Top           0
#define MUIV_List_Exchange_Active       -1
#define MUIV_List_Exchange_Bottom       -2
#define MUIV_List_Exchange_Next         -3 /* only valid for second parameter */
#define MUIV_List_Exchange_Previous     -4 /* only valid for second parameter */

#define MUIV_List_Jump_Top               0
#define MUIV_List_Jump_Active           -1
#define MUIV_List_Jump_Bottom           -2
#define MUIV_List_Jump_Up               -4
#define MUIV_List_Jump_Down             -3

#define MUIV_List_NextSelected_Start    -1
#define MUIV_List_NextSelected_End      -1


#define MUIA_List_Prop_Entries  0x8042a8f5 /* PRIV */
#define MUIA_List_Prop_Visible  0x804273e9 /* PRIV */
#define MUIA_List_Prop_First    0x80429df3 /* PRIV */

#define MUIA_List_VertProp_Entries  MUIA_List_Prop_Entries  /* PRIV */
#define MUIA_List_VertProp_Visible  MUIA_List_Prop_Visible  /* PRIV */
#define MUIA_List_VertProp_First    MUIA_List_Prop_First    /* PRIV */

#define MUIA_List_HorizProp_Entries  0x80429df4 /* PRIV */
#define MUIA_List_HorizProp_Visible  0x80429df5 /* PRIV */
#define MUIA_List_HorizProp_First    0x80429df6 /* PRIV */

/****************************************************************************/
/** Floattext                                                              **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Floattext[];
#else
#define MUIC_Floattext "Floattext.mui"
#endif

/* Attributes */

#define MUIA_Floattext_Justify              0x8042dc03 /* V4  isg BOOL              */
#define MUIA_Floattext_SkipChars            0x80425c7d /* V4  is. STRPTR            */
#define MUIA_Floattext_TabSize              0x80427d17 /* V4  is. LONG              */
#define MUIA_Floattext_Text                 0x8042d16a /* V4  isg STRPTR            */



/****************************************************************************/
/** Volumelist                                                             **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Volumelist[];
#else
#define MUIC_Volumelist "Volumelist.mui"
#endif


/****************************************************************************/
/** Scrmodelist                                                            **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Scrmodelist[];
#else
#define MUIC_Scrmodelist "Scrmodelist.mui"
#endif

/* Attributes */




/****************************************************************************/
/** Dirlist                                                                **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Dirlist[];
#else
#define MUIC_Dirlist "Dirlist.mui"
#endif

/* Methods */

#define MUIM_Dirlist_ReRead                 0x80422d71 /* V4  */
struct  MUIP_Dirlist_ReRead                 { ULONG MethodID; };

/* Attributes */

#define MUIA_Dirlist_AcceptPattern          0x8042760a /* V4  is. STRPTR            */
#define MUIA_Dirlist_Directory              0x8042ea41 /* V4  isg STRPTR            */
#define MUIA_Dirlist_DrawersOnly            0x8042b379 /* V4  is. BOOL              */
#define MUIA_Dirlist_FilesOnly              0x8042896a /* V4  is. BOOL              */
#define MUIA_Dirlist_FilterDrawers          0x80424ad2 /* V4  is. BOOL              */
#define MUIA_Dirlist_FilterHook             0x8042ae19 /* V4  is. struct Hook *     */
#define MUIA_Dirlist_MultiSelDirs           0x80428653 /* V6  is. BOOL              */
#define MUIA_Dirlist_NumBytes               0x80429e26 /* V4  ..g LONG              */
#define MUIA_Dirlist_NumDrawers             0x80429cb8 /* V4  ..g LONG              */
#define MUIA_Dirlist_NumFiles               0x8042a6f0 /* V4  ..g LONG              */
#define MUIA_Dirlist_Path                   0x80426176 /* V4  ..g STRPTR            */
#define MUIA_Dirlist_RejectIcons            0x80424808 /* V4  is. BOOL              */
#define MUIA_Dirlist_RejectPattern          0x804259c7 /* V4  is. STRPTR            */
#define MUIA_Dirlist_SortDirs               0x8042bbb9 /* V4  is. LONG              */
#define MUIA_Dirlist_SortHighLow            0x80421896 /* V4  is. BOOL              */
#define MUIA_Dirlist_SortType               0x804228bc /* V4  is. LONG              */
#define MUIA_Dirlist_Status                 0x804240de /* V4  ..g LONG              */

#define MUIV_Dirlist_SortDirs_First 0
#define MUIV_Dirlist_SortDirs_Last 1
#define MUIV_Dirlist_SortDirs_Mix 2
#define MUIV_Dirlist_SortType_Name 0
#define MUIV_Dirlist_SortType_Date 1
#define MUIV_Dirlist_SortType_Size 2
#define MUIV_Dirlist_Status_Invalid 0
#define MUIV_Dirlist_Status_Reading 1
#define MUIV_Dirlist_Status_Valid 2


extern const struct __MUIBuiltinClass _MUI_Listview_desc;
extern const struct __MUIBuiltinClass _MUI_List_desc;

/**********************/
/* List Position Test */
/**********************/

struct MUI_List_TestPos_Result
{
	LONG  entry;   /* number of entry, -1 if mouse not over valid entry */
	WORD  column;  /* numer of column, -1 if no valid column */
	UWORD flags;   /* see below */
	WORD  xoffset; /* x offset of mouse click relative to column start */
	WORD  yoffset; /* y offset of mouse click from center of line
	                  (negative values mean click was above center,
	                   positive values mean click was below center) */
};

#define MUI_LPR_ABOVE  (1<<0)
#define MUI_LPR_BELOW  (1<<1)
#define MUI_LPR_LEFT   (1<<2)
#define MUI_LPR_RIGHT  (1<<3)


#endif
