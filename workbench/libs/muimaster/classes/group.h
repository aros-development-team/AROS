#ifndef _MUI_CLASSES_GROUP_H
#define _MUI_CLASSES_GROUP_H

/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002-2012, The AROS Development Team.
    All rights reserved.

    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Group                 "Group.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Group                 (MUIB_ZUNE | 0x00001000)

/*** Methods ****************************************************************/
#define MUIM_Group_AddHead      (MUIB_MUI | 0x0042e200)        /* MUI: V8 */
#define MUIM_Group_AddTail      (MUIB_MUI | 0x0042d752)        /* MUI: V8 */
#define MUIM_Group_ExitChange   (MUIB_MUI | 0x0042d1cc)        /* MUI: V11 */
#define MUIM_Group_ExitChange2  (MUIB_MUI | 0x0042e541)        /* MUI: PRIV */
#define MUIM_Group_InitChange   (MUIB_MUI | 0x00420887)        /* MUI: V11 */
#define MUIM_Group_Sort         (MUIB_MUI | 0x00427417)        /* MUI: V4  */
#define MUIM_Group_Remove       (MUIB_MUI | 0x0042f8a9)        /* MUI: V8 */

struct MUIP_Group_AddHead
{
    STACKED ULONG MethodID;
    STACKED Object *obj;
};

struct MUIP_Group_AddTail
{
    STACKED ULONG MethodID;
    STACKED Object *obj;
};

struct MUIP_Group_ExitChange
{
    STACKED ULONG MethodID;
};

struct MUIP_Group_ExitChange2 /* PRIV */
{                             /* PRIV */
    STACKED ULONG MethodID;   /* PRIV */
    STACKED ULONG flags;      /* PRIV */
};                            /* PRIV */

struct MUIP_Group_InitChange
{
    STACKED ULONG MethodID;
};

struct MUIP_Group_Sort
{
    STACKED ULONG MethodID;
    STACKED Object *obj[1];
};

struct MUIP_Group_Remove
{
    STACKED ULONG MethodID;
    STACKED Object *obj;
};

#define MUIM_Group_DoMethodNoForward (MUIB_Group | 0x00000000)
struct MUIP_Group_DoMethodNoForward
{
    STACKED ULONG MethodID;
    STACKED ULONG DoMethodID;
};                              /* msg stuff follows */

/*** Attributes *************************************************************/
#define MUIA_Group_ActivePage \
    (MUIB_MUI | 0x00424199)        /* MUI: V5  isg LONG          */
#define MUIA_Group_Child \
    (MUIB_MUI | 0x004226e6)        /* MUI: V4  i.. Object *      */
#define MUIA_Group_ChildList \
    (MUIB_MUI | 0x00424748)        /* MUI: V4  ..g struct List * */
#define MUIA_Group_Columns \
    (MUIB_MUI | 0x0042f416)        /* MUI: V4  is. LONG          */
#define MUIA_Group_Forward \
    (MUIB_MUI | 0x00421422)        /* MUI: V11 .s. BOOL          */
#define MUIA_Group_Horiz \
    (MUIB_MUI | 0x0042536b)        /* MUI: V4  i.. BOOL          */
#define MUIA_Group_HorizSpacing \
    (MUIB_MUI | 0x0042c651)        /* MUI: V4  isg LONG          */
#define MUIA_Group_LayoutHook \
    (MUIB_MUI | 0x0042c3b2)        /* MUI: V11 i.. struct Hook * */
#define MUIA_Group_PageMode \
    (MUIB_MUI | 0x00421a5f)        /* MUI: V5  i.. BOOL          */
#define MUIA_Group_Rows \
    (MUIB_MUI | 0x0042b68f)        /* MUI: V4  is. LONG          */
#define MUIA_Group_SameHeight \
    (MUIB_MUI | 0x0042037e)        /* MUI: V4  i.. BOOL          */
#define MUIA_Group_SameSize \
    (MUIB_MUI | 0x00420860)        /* MUI: V4  i.. BOOL          */
#define MUIA_Group_SameWidth \
    (MUIB_MUI | 0x0042b3ec)        /* MUI: V4  i.. BOOL          */
#define MUIA_Group_Spacing \
    (MUIB_MUI | 0x0042866d)        /* MUI: V4  is. LONG          */
#define MUIA_Group_VertSpacing \
    (MUIB_MUI | 0x0042e1bf)        /* MUI: V4  isg LONG          */

#define MUIA_Group_Virtual \
    (MUIB_Group | 0x00000000)    /* Zune: V1 i.. BOOL  */

enum
{
    MUIV_Group_ActivePage_First = 0,
    MUIV_Group_ActivePage_Last = -1,
    MUIV_Group_ActivePage_Prev = -2,
    MUIV_Group_ActivePage_Next = -3,
    MUIV_Group_ActivePage_Advance = -4,
};

/* This is the message you get if your custom layout hook is called */
struct MUI_LayoutMsg
{
    STACKED ULONG lm_Type;      /* the message type */
    STACKED struct MinList *lm_Children;    /* exec list of the children
                                             * of this group */
    STACKED struct MUI_MinMax lm_MinMax;    /* here you have to place the
                                             * MUILM_MINMAX results */
    struct
    {
        STACKED LONG Width;
        STACKED LONG Height;
        STACKED ULONG priv5;
        STACKED ULONG priv6;
    } lm_Layout;                /* size (and result) for MUILM_LAYOUT */
};

/* lm_Type */
enum
{
    MUILM_MINMAX = 1,           /* Please calc your min & max sizes */
    MUILM_LAYOUT = 2,           /* Please layout your children */
};

#define MUILM_UNKNOWN  -1       /* should be returned if the hook function
                                 * doesn't understand lm_Type */


extern const struct __MUIBuiltinClass _MUI_Group_desc;  /* PRIV */

#endif /* _MUI_CLASSES_GROUP_H */
