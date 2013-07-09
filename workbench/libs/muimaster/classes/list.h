#ifndef _MUI_CLASSES_LIST_H
#define _MUI_CLASSES_LIST_H

/*
    Copyright © 2002-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

/****************************************************************************/
/*** Name *******************************************************************/
#define MUIC_List                     "List.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_List                     (MUIB_ZUNE | 0x00001400)

/*** Methods ****************************************************************/
#define MUIM_List_Clear           (MUIB_MUI | 0x0042ad89)     /* MUI: V4  */
#define MUIM_List_Compare         (MUIB_MUI | 0x00421b68)     /* MUI: V20 */
#define MUIM_List_Construct       (MUIB_MUI | 0x0042d662)     /* MUI: V20 */
#define MUIM_List_CreateImage     (MUIB_MUI | 0x00429804)     /* MUI: V11 */
#define MUIM_List_DeleteImage     (MUIB_MUI | 0x00420f58)     /* MUI: V11 */
#define MUIM_List_Destruct        (MUIB_MUI | 0x00427d51)     /* MUI: V20 */
#define MUIM_List_Display         (MUIB_MUI | 0x00425377)     /* MUI: V20 */
#define MUIM_List_Exchange        (MUIB_MUI | 0x0042468c)     /* MUI: V4  */
#define MUIM_List_GetEntry        (MUIB_MUI | 0x004280ec)     /* MUI: V4  */
#define MUIM_List_Insert          (MUIB_MUI | 0x00426c87)     /* MUI: V4  */
#define MUIM_List_InsertSingle    (MUIB_MUI | 0x004254d5)     /* MUI: V7  */
#define MUIM_List_Jump            (MUIB_MUI | 0x0042baab)     /* MUI: V4  */
#define MUIM_List_Move            (MUIB_MUI | 0x004253c2)     /* MUI: V9  */
#define MUIM_List_NextSelected    (MUIB_MUI | 0x00425f17)     /* MUI: V6  */
#define MUIM_List_Redraw          (MUIB_MUI | 0x00427993)     /* MUI: V4  */
#define MUIM_List_Remove          (MUIB_MUI | 0x0042647e)     /* MUI: V4  */
#define MUIM_List_Select          (MUIB_MUI | 0x004252d8)     /* MUI: V4  */
#define MUIM_List_Sort            (MUIB_MUI | 0x00422275)     /* MUI: V4  */
#define MUIM_List_TestPos         (MUIB_MUI | 0x00425f48)     /* MUI: V11 */

struct MUIP_List_Clear
{
    STACKED ULONG MethodID;
};

struct MUIP_List_Compare
{
    STACKED ULONG MethodID;
    STACKED APTR entry1;
    STACKED APTR entry2;
    STACKED LONG sort_type1;
    STACKED LONG sort_type2;
};

struct MUIP_List_Construct
{
    STACKED ULONG MethodID;
    STACKED APTR entry;
    STACKED APTR pool;
};

struct MUIP_List_CreateImage
{
    STACKED ULONG MethodID;
    STACKED Object *obj;
    STACKED ULONG flags;
};

struct MUIP_List_DeleteImage
{
    STACKED ULONG MethodID;
    STACKED APTR listimg;
};

struct MUIP_List_Destruct
{
    STACKED ULONG MethodID;
    STACKED APTR entry;
    STACKED APTR pool;
};

struct MUIP_List_Display
{
    STACKED ULONG MethodID;
    STACKED APTR entry;
    STACKED STRPTR *array;
    STACKED LONG entry_pos;
    STACKED STRPTR *preparses;
};

struct MUIP_List_Exchange
{
    STACKED ULONG MethodID;
    STACKED LONG pos1;
    STACKED LONG pos2;
};

struct MUIP_List_GetEntry
{
    STACKED ULONG MethodID;
    STACKED LONG pos;
    STACKED APTR *entry;
};

struct MUIP_List_Insert
{
    STACKED ULONG MethodID;
    STACKED APTR *entries;
    STACKED LONG count;
    STACKED LONG pos;
};

struct MUIP_List_InsertSingle
{
    STACKED ULONG MethodID;
    STACKED APTR entry;
    STACKED LONG pos;
};

struct MUIP_List_Jump
{
    STACKED ULONG MethodID;
    STACKED LONG pos;
};

struct MUIP_List_Move
{
    STACKED ULONG MethodID;
    STACKED LONG from;
    STACKED LONG to;
};

struct MUIP_List_NextSelected
{
    STACKED ULONG MethodID;
    STACKED LONG *pos;
};

struct MUIP_List_Redraw
{
    STACKED ULONG MethodID;
    STACKED LONG pos;
    STACKED APTR entry;
};

struct MUIP_List_Remove
{
    STACKED ULONG MethodID;
    STACKED LONG pos;
};

struct MUIP_List_Select
{
    STACKED ULONG MethodID;
    STACKED LONG pos;
    STACKED LONG seltype;
    STACKED LONG *info;
};

struct MUIP_List_Sort
{
    STACKED ULONG MethodID;
};

struct MUIP_List_TestPos
{
    STACKED ULONG MethodID;
    STACKED LONG x;
    STACKED LONG y;
    STACKED struct MUI_List_TestPos_Result *res;
};

#define MUIM_List_SelectChange  /* PRIV */ \
    (MUIB_List | 0x00000004)  /* Zune: V1 same like NLIST, PRIV for now! */

struct MUIP_List_SelectChange
{
    STACKED ULONG MethodID;
    STACKED LONG pos;
    STACKED LONG state;
    STACKED ULONG flags;
};

/*** Attributes *************************************************************/
#define MUIA_List_Active \
    (MUIB_MUI | 0x0042391c)     /* MUI: V4  isg LONG          */
#define MUIA_List_AdjustHeight \
    (MUIB_MUI | 0x0042850d)     /* MUI: V4  i.. BOOL          */
#define MUIA_List_AdjustWidth \
    (MUIB_MUI | 0x0042354a)     /* MUI: V4  i.. BOOL          */
#define MUIA_List_AutoVisible \
    (MUIB_MUI | 0x0042a445)     /* MUI: V11 isg BOOL          */
#define MUIA_List_CompareHook \
    (MUIB_MUI | 0x00425c14)     /* MUI: V4  is. struct Hook * */
#define MUIA_List_ConstructHook \
    (MUIB_MUI | 0x0042894f)     /* MUI: V4  is. struct Hook * */
#define MUIA_List_DestructHook \
    (MUIB_MUI | 0x004297ce)     /* MUI: V4  is. struct Hook * */
#define MUIA_List_DisplayHook \
    (MUIB_MUI | 0x0042b4d5)     /* MUI: V4  is. struct Hook * */
#define MUIA_List_DragSortable \
    (MUIB_MUI | 0x00426099)     /* MUI: V11 isg BOOL          */
#define MUIA_List_DropMark \
    (MUIB_MUI | 0x0042aba6)     /* MUI: V11 ..g LONG          */
#define MUIA_List_Entries \
    (MUIB_MUI | 0x00421654)     /* MUI: V4  ..g LONG          */
#define MUIA_List_First \
    (MUIB_MUI | 0x004238d4)     /* MUI: V4  ..g LONG          */
#define MUIA_List_Format \
    (MUIB_MUI | 0x00423c0a)     /* MUI: V4  isg STRPTR        */
#define MUIA_List_InsertPosition \
    (MUIB_MUI | 0x0042d0cd)     /* MUI: V9  ..g LONG          */
#define MUIA_List_MinLineHeight \
    (MUIB_MUI | 0x0042d1c3)     /* MUI: V4  i.. LONG          */
#define MUIA_List_MultiTestHook \
    (MUIB_MUI | 0x0042c2c6)     /* MUI: V4  is. struct Hook * */
#define MUIA_List_Pool \
    (MUIB_MUI | 0x00423431)     /* MUI: V13 i.. APTR          */
#define MUIA_List_PoolPuddleSize \
    (MUIB_MUI | 0x0042a4eb)     /* MUI: V13 i.. ULONG         */
#define MUIA_List_PoolThreshSize \
    (MUIB_MUI | 0x0042c48c)     /* MUI: V13 i.. ULONG         */
#define MUIA_List_Quiet \
    (MUIB_MUI | 0x0042d8c7)     /* MUI: V4  .s. BOOL          */
#define MUIA_List_ShowDropMarks \
    (MUIB_MUI | 0x0042c6f3)     /* MUI: V11 isg BOOL          */
#define MUIA_List_SourceArray \
    (MUIB_MUI | 0x0042c0a0)     /* MUI: V4  i.. APTR          */
#define MUIA_List_Title \
    (MUIB_MUI | 0x00423e66)     /* MUI: V6  isg char *        */
#define MUIA_List_Visible \
    (MUIB_MUI | 0x0042191f)     /* MUI: V4  ..g LONG          */
#define MUIA_List_Prop_Entries  /* PRIV */ \
    (MUIB_MUI | 0x0042a8f5)     /* .sg LONG  PRIV */
#define MUIA_List_Prop_Visible  /* PRIV */ \
    (MUIB_MUI | 0x004273e9)     /* .sg LONG  PRIV */
#define MUIA_List_Prop_First    /* PRIV */ \
    (MUIB_MUI | 0x00429df3)     /* .sg LONG  PRIV */

#define MUIA_List_VertProp_Entries  /* PRIV */ \
    MUIA_List_Prop_Entries     /* PRIV */
#define MUIA_List_VertProp_Visible  /* PRIV */ \
    MUIA_List_Prop_Visible     /* PRIV */
#define MUIA_List_VertProp_First  /* PRIV */ \
    MUIA_List_Prop_First       /* PRIV */
#define MUIA_List_HorizProp_Entries  /* PRIV */ \
    (MUIB_List | 0x00000000)   /* ... LONG  PRIV */
#define MUIA_List_HorizProp_Visible  /* PRIV */ \
    (MUIB_List | 0x00000001)   /* ... LONG  PRIV */
#define MUIA_List_HorizProp_First  /* PRIV */ \
    (MUIB_List | 0x00000002)   /* ... LONG  PRIV */

/* Structure of the List Position Test (MUIM_List_TestPos) */
struct MUI_List_TestPos_Result
{
    LONG entry;      /* entry number, maybe -1 if testpos is not over valid
                      * entry */
    WORD column;     /* the number of the column, maybe -1 (invalid) */
    UWORD flags;     /* some flags, see below */
    WORD xoffset;    /* x offset (in pixels) of testpos relative to the start
                      * of the column */
    WORD yoffset;    /* y offset (in pixels) of testpos relative from center
                      * of line( <0 => testpos was above, >0 => testpos was
                      * below center) */
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

#define MUIV_List_ConstructHook_String (IPTR)-1
#define MUIV_List_CopyHook_String      (IPTR)-1
#define MUIV_List_CursorType_None 0
#define MUIV_List_CursorType_Bar  1
#define MUIV_List_CursorType_Rect 2
#define MUIV_List_DestructHook_String  (IPTR)-1

enum
{
    MUIV_List_Insert_Top = 0,
    MUIV_List_Insert_Active = -1,
    MUIV_List_Insert_Sorted = -2,
    MUIV_List_Insert_Bottom = -3
};

enum
{
    MUIV_List_Remove_First = 0,
    MUIV_List_Remove_Active = -1,
    MUIV_List_Remove_Last = -2,
    MUIV_List_Remove_Selected = -3,
};

enum
{
    MUIV_List_Select_Active = -1,
    MUIV_List_Select_All = -2,

    MUIV_List_Select_Off = 0,
    MUIV_List_Select_On = 1,
    MUIV_List_Select_Toggle = 2,
    MUIV_List_Select_Ask = 3,
};

enum
{
    MUIV_List_GetEntry_Active = -1,
};

enum
{
    MUIV_List_Redraw_Active = -1,
    MUIV_List_Redraw_All = -2,
    MUIV_List_Redraw_Entry = -3,
};

enum
{
    MUIV_List_Move_Top = 0,
    MUIV_List_Move_Active = -1,
    MUIV_List_Move_Bottom = -2,
    MUIV_List_Move_Next = -3,        /* for 2nd parameter only */
    MUIV_List_Move_Previous = -4,    /* for 2nd parameter only */
};

enum
{
    MUIV_List_Exchange_Top = 0,
    MUIV_List_Exchange_Active = -1,
    MUIV_List_Exchange_Bottom = -2,
    MUIV_List_Exchange_Next = -3,       /* for 2nd parameter only */
    MUIV_List_Exchange_Previous = -4,   /* for 2nd parameter only */
};

enum
{
    MUIV_List_Jump_Top = 0,
    MUIV_List_Jump_Active = -1,
    MUIV_List_Jump_Bottom = -2,
    MUIV_List_Jump_Down = -3,
    MUIV_List_Jump_Up = -4,
};

#define MUIV_List_NextSelected_Start  (-1)
#define MUIV_List_NextSelected_End    (-1)


#define MUIV_NList_SelectChange_Flag_Multi (1 << 0)


extern const struct __MUIBuiltinClass _MUI_List_desc;   /* PRIV */




/****************************************************************************/
/*** Name *******************************************************************/
#define MUIC_Scrmodelist "Scrmodelist.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Scrmodelist (MUIB_ZUNE | 0x00001700)

#endif /* _MUI_CLASSES_LIST_H */
