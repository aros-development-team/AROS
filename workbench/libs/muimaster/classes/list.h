#ifndef _MUI_CLASSES_LIST_H
#define _MUI_CLASSES_LIST_H

/*
    Copyright � 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/****************************************************************************/
/*** Name *******************************************************************/
#define MUIC_List                     "List.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_List                     (MUIB_ZUNE | 0x00001400)  

/*** Methods ****************************************************************/
#define MUIM_List_Clear               (MUIB_MUI|0x0042ad89) /* MUI: V4  */
#define MUIM_List_CreateImage         (MUIB_MUI|0x00429804) /* MUI: V11 */
#define MUIM_List_DeleteImage         (MUIB_MUI|0x00420f58) /* MUI: V11 */
#define MUIM_List_Exchange            (MUIB_MUI|0x0042468c) /* MUI: V4  */
#define MUIM_List_GetEntry            (MUIB_MUI|0x004280ec) /* MUI: V4  */
#define MUIM_List_Insert              (MUIB_MUI|0x00426c87) /* MUI: V4  */
#define MUIM_List_InsertSingle        (MUIB_MUI|0x004254d5) /* MUI: V7  */
#define MUIM_List_Jump                (MUIB_MUI|0x0042baab) /* MUI: V4  */
#define MUIM_List_Move                (MUIB_MUI|0x004253c2) /* MUI: V9  */
#define MUIM_List_NextSelected        (MUIB_MUI|0x00425f17) /* MUI: V6  */
#define MUIM_List_Redraw              (MUIB_MUI|0x00427993) /* MUI: V4  */
#define MUIM_List_Remove              (MUIB_MUI|0x0042647e) /* MUI: V4  */
#define MUIM_List_Select              (MUIB_MUI|0x004252d8) /* MUI: V4  */
#define MUIM_List_Sort                (MUIB_MUI|0x00422275) /* MUI: V4  */
#define MUIM_List_TestPos             (MUIB_MUI|0x00425f48) /* MUI: V11 */
struct MUIP_List_Clear                {ULONG MethodID;};
struct MUIP_List_CreateImage          {ULONG MethodID; Object *obj; ULONG flags;};
struct MUIP_List_DeleteImage          {ULONG MethodID; APTR listimg;};
struct MUIP_List_Exchange             {ULONG MethodID; LONG pos1; LONG pos2;};
struct MUIP_List_GetEntry             {ULONG MethodID; LONG pos; APTR *entry;};
struct MUIP_List_Insert               {ULONG MethodID; APTR *entries; LONG count; LONG pos;};
struct MUIP_List_InsertSingle         {ULONG MethodID; APTR entry; LONG pos;};
struct MUIP_List_Jump                 {ULONG MethodID; LONG pos;};
struct MUIP_List_Move                 {ULONG MethodID; LONG from; LONG to;};
struct MUIP_List_NextSelected         {ULONG MethodID; LONG *pos;};
struct MUIP_List_Redraw               {ULONG MethodID; LONG pos;};
struct MUIP_List_Remove               {ULONG MethodID; LONG pos;};
struct MUIP_List_Select               {ULONG MethodID; LONG pos; LONG seltype; LONG *state;};
struct MUIP_List_Sort                 {ULONG MethodID;};
struct MUIP_List_TestPos              {ULONG MethodID; LONG x; LONG y; struct MUI_List_TestPos_Result *res;};

#define MUIM_List_Construct           (MUIB_List | 0x00000000) /* Zune: V1 same like NList, PRIV for now! */
#define MUIM_List_Destruct            (MUIB_List | 0x00000001) /* Zune: V1 same like NList, PRIV for now! */
#define MUIM_List_Compare             (MUIB_List | 0x00000002) /* Zune: v1 same like NList, PRIV for now! */
#define MUIM_List_Display             (MUIB_List | 0x00000003) /* Zune: V1 same like NList, PRIV for now! */
#define MUIM_List_SelectChange        (MUIB_List | 0x00000004) /* Zune: V1 same like NLIST, PRIV for now! */
#define MUIM_List_InsertSingleAsTree  (MUIB_List | 0x00000005) /* Zune: V1 */
struct MUIP_List_Construct            {ULONG MethodID; APTR entry; APTR pool;};
struct MUIP_List_Destruct             {ULONG MethodID; APTR entry; APTR pool;};
struct MUIP_List_Compare              {ULONG MethodID; APTR entry1; APTR entry2; LONG sort_type1; LONG sort_type2;};
struct MUIP_List_Display              {ULONG MethodID; APTR entry; LONG entry_pos; STRPTR *strings; STRPTR *preparses;};
struct MUIP_List_SelectChange         {ULONG MethodID; LONG pos; LONG state; ULONG flags;};
struct MUIP_List_InsertSingleAsTree   {ULONG MethodID; APTR entry; LONG parent; LONG rel_entry_pos; ULONG flags;};

/*** Attributes *************************************************************/
#define MUIA_List_Active              (MUIB_MUI|0x0042391c) /* MUI: V4  isg LONG          */
#define MUIA_List_AdjustHeight        (MUIB_MUI|0x0042850d) /* MUI: V4  i.. BOOL          */
#define MUIA_List_AdjustWidth         (MUIB_MUI|0x0042354a) /* MUI: V4  i.. BOOL          */
#define MUIA_List_AutoVisible         (MUIB_MUI|0x0042a445) /* MUI: V11 isg BOOL          */
#define MUIA_List_CompareHook         (MUIB_MUI|0x00425c14) /* MUI: V4  is. struct Hook * */
#define MUIA_List_ConstructHook       (MUIB_MUI|0x0042894f) /* MUI: V4  is. struct Hook * */
#define MUIA_List_DestructHook        (MUIB_MUI|0x004297ce) /* MUI: V4  is. struct Hook * */
#define MUIA_List_DisplayHook         (MUIB_MUI|0x0042b4d5) /* MUI: V4  is. struct Hook * */
#define MUIA_List_DragSortable        (MUIB_MUI|0x00426099) /* MUI: V11 isg BOOL          */
#define MUIA_List_DropMark            (MUIB_MUI|0x0042aba6) /* MUI: V11 ..g LONG          */
#define MUIA_List_Entries             (MUIB_MUI|0x00421654) /* MUI: V4  ..g LONG          */
#define MUIA_List_First               (MUIB_MUI|0x004238d4) /* MUI: V4  ..g LONG          */
#define MUIA_List_Format              (MUIB_MUI|0x00423c0a) /* MUI: V4  isg STRPTR        */
#define MUIA_List_InsertPosition      (MUIB_MUI|0x0042d0cd) /* MUI: V9  ..g LONG          */
#define MUIA_List_MinLineHeight       (MUIB_MUI|0x0042d1c3) /* MUI: V4  i.. LONG          */
#define MUIA_List_MultiTestHook       (MUIB_MUI|0x0042c2c6) /* MUI: V4  is. struct Hook * */
#define MUIA_List_Pool                (MUIB_MUI|0x00423431) /* MUI: V13 i.. APTR          */
#define MUIA_List_PoolPuddleSize      (MUIB_MUI|0x0042a4eb) /* MUI: V13 i.. ULONG         */
#define MUIA_List_PoolThreshSize      (MUIB_MUI|0x0042c48c) /* MUI: V13 i.. ULONG         */
#define MUIA_List_Quiet               (MUIB_MUI|0x0042d8c7) /* MUI: V4  .s. BOOL          */
#define MUIA_List_ShowDropMarks       (MUIB_MUI|0x0042c6f3) /* MUI: V11 isg BOOL          */
#define MUIA_List_SourceArray         (MUIB_MUI|0x0042c0a0) /* MUI: V4  i.. APTR          */
#define MUIA_List_Title               (MUIB_MUI|0x00423e66) /* MUI: V6  isg char *        */
#define MUIA_List_Visible             (MUIB_MUI|0x0042191f) /* MUI: V4  ..g LONG          */
#define MUIA_List_Prop_Entries        (MUIB_MUI|0x0042a8f5) /* .sg LONG  PRIV */
#define MUIA_List_Prop_Visible        (MUIB_MUI|0x004273e9) /* .sg LONG  PRIV */
#define MUIA_List_Prop_First          (MUIB_MUI|0x00429df3) /* .sg LONG  PRIV */

#define MUIA_List_VertProp_Entries   MUIA_List_Prop_Entries  /* PRIV */
#define MUIA_List_VertProp_Visible   MUIA_List_Prop_Visible  /* PRIV */
#define MUIA_List_VertProp_First     MUIA_List_Prop_First    /* PRIV */
#define MUIA_List_HorizProp_Entries  (MUIB_List | 0x00000000) /* ... LONG  PRIV */
#define MUIA_List_HorizProp_Visible  (MUIB_List | 0x00000001) /* ... LONG  PRIV */
#define MUIA_List_HorizProp_First    (MUIB_List | 0x00000002) /* ... LONG  PRIV */

/* Structure of the List Position Text (MUIM_List_TestPos) */
struct MUI_List_TestPos_Result
{
    LONG  entry;   /* entry number, maybe -1 if testpos is not over valid entry */
    WORD  column;  /* the number of the column, maybe -1 (unvalid) */
    UWORD flags;   /* some flags, see below */
    WORD  xoffset; /* x offset (in pixels) of testpos relative to the start of the column */
    WORD  yoffset; /* y offset (in pixels) of testpos relative from center of line
	                    ( <0 => testpos was above, >0 => testpos was below center) */
};

#define MUI_LPR_ABOVE (1<<0)
#define MUI_LPR_BELOW (1<<1)
#define MUI_LPR_LEFT  (1<<2)
#define MUI_LPR_RIGHT (1<<3)

enum
{
    MUIV_List_Active_Off = -1,
    MUIV_List_Active_Top = -2,
    MUIV_List_Active_Bottom = -3,
    MUIV_List_Active_Up = -4,
    MUIV_List_Active_Down = -5,
    MUIV_List_Active_PageUp = -6,
    MUIV_List_Active_PageDown = -7,
};

#define MUIV_List_ConstructHook_String -1
#define MUIV_List_CopyHook_String -1
#define MUIV_List_CursorType_None 0
#define MUIV_List_CursorType_Bar 1
#define MUIV_List_CursorType_Rect 2
#define MUIV_List_DestructHook_String -1

enum
{
    MUIV_List_Insert_Top    =  0,
    MUIV_List_Insert_Active = -1,
    MUIV_List_Insert_Sorted = -2,
    MUIV_List_Insert_Bottom = -3
};

enum
{
	  MUIV_List_Remove_First    =  0,
    MUIV_List_Remove_Active   = -1,
    MUIV_List_Remove_Last     = -2,
    MUIV_List_Remove_Selected = -3,
};

enum
{
    MUIV_List_Select_Active = -1,
    MUIV_List_Select_All    = -2,

    MUIV_List_Select_Off    = 0,
    MUIV_List_Select_On     = 1,
    MUIV_List_Select_Toggle = 2,
    MUIV_List_Select_Ask    = 3,
};

enum
{
    MUIV_List_GetEntry_Active = -1,
};

enum
{
    MUIV_List_Redraw_Active = -1,
    MUIV_List_Redraw_All    = -2,
};

enum
{
    MUIV_List_Move_Top      =  0,
    MUIV_List_Move_Active   = -1,
    MUIV_List_Move_Bottom   = -2,
    MUIV_List_Move_Next     = -3, /* for 2nd parameter only */
    MUIV_List_Move_Previous = -4, /* for 2nd parameter only */
};

enum
{
    MUIV_List_Exchange_Top      =  0,
    MUIV_List_Exchange_Active   = -1,
    MUIV_List_Exchange_Bottom   = -2,
    MUIV_List_Exchange_Next     = -3, /* for 2nd parameter only */
    MUIV_List_Exchange_Previous = -4, /* for 2nd parameter only */
};

enum
{
    MUIV_List_Jump_Top    =  0,
    MUIV_List_Jump_Active = -1,
    MUIV_List_Jump_Bottom = -2,
    MUIV_List_Jump_Down   = -3,
    MUIV_List_Jump_Up     = -4,
};

#define MUIV_List_NextSelected_Start  (-1)
#define MUIV_List_NextSelected_End    (-1)


#define MUIV_NList_SelectChange_Flag_Multi (1 << 0)

#define MUIV_List_InsertSingleAsTree_Root     (-1)

#define MUIV_List_InsertSingleAsTree_Top      (0)
#define MUIV_List_InsertSingleAsTree_Active   (-1)
#define MUIV_List_InsertSingleAsTree_Sorted   (-2)
#define MUIV_List_InsertSingleAsTree_Bottom   (-3)

#define MUIV_List_InsertSingleAsTree_List    (1<<0)
#define MUIV_List_InsertSingleAsTree_Closed  (1<<1)

extern const struct __MUIBuiltinClass _MUI_List_desc; /* PRIV */



/****************************************************************************/
/*** Name *******************************************************************/
#define MUIC_Floattext           "Floattext.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Floattext           (MUIB_ZUNE | 0x00001500)

/*** Attributes *************************************************************/
#define MUIA_Floattext_Justify   (MUIB_MUI|0x0042dc03) /* MUI: V4  isg BOOL   */
#define MUIA_Floattext_SkipChars (MUIB_MUI|0x00425c7d) /* MUI: V4  is. STRPTR */
#define MUIA_Floattext_TabSize   (MUIB_MUI|0x00427d17) /* MUI: V4  is. LONG   */
#define MUIA_Floattext_Text      (MUIB_MUI|0x0042d16a) /* MUI: V4  isg STRPTR */




/****************************************************************************/
/*** Name *******************************************************************/
#define MUIC_Scrmodelist "Scrmodelist.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Scrmodelist (MUIB_ZUNE | 0x00001700)





#endif /* _MUI_CLASSES_LIST_H */
