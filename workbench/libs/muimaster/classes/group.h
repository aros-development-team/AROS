/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#ifndef _MUI_CLASSES_GROUP_H
#define _MUI_CLASSES_GROUP_H

#ifndef METHOD_USER
#define METHOD_USER TAG_USER
#endif

#define MUIC_Group "Group.mui"

/* Group methods */
#define MUIM_Group_ExitChange      (METHOD_USER|0x0042d1cc) /* MUI: V11 */
#define MUIM_Group_InitChange      (METHOD_USER|0x00420887) /* MUI: V11 */
#define MUIM_Group_Sort            (METHOD_USER|0x80427417) /* MUI: V4  */
struct MUIP_Group_ExitChange       {ULONG MethodID;};
struct MUIP_Group_InitChange       {ULONG MethodID;};
struct MUIP_Group_Sort             {ULONG MethodID; Object *obj[1];};

/* Group attributes */
#define MUIA_Group_ActivePage      (TAG_USER|0x00424199) /* MUI: V5  isg LONG          */
#define MUIA_Group_Child           (TAG_USER|0x004226e6) /* MUI: V4  i.. Object *      */
#define MUIA_Group_ChildList       (TAG_USER|0x00424748) /* MUI: V4  ..g struct List * */
#define MUIA_Group_Columns         (TAG_USER|0x0042f416) /* MUI: V4  is. LONG          */
#define MUIA_Group_Forward         (TAG_USER|0x00421422) /* MUI: V11 .s. BOOL          */
#define MUIA_Group_Horiz           (TAG_USER|0x0042536b) /* MUI: V4  i.. BOOL          */
#define MUIA_Group_HorizSpacing    (TAG_USER|0x0042c651) /* MUI: V4  isg LONG          */
#define MUIA_Group_LayoutHook      (TAG_USER|0x0042c3b2) /* MUI: V11 i.. struct Hook * */
#define MUIA_Group_PageMode        (TAG_USER|0x00421a5f) /* MUI: V5  i.. BOOL          */
#define MUIA_Group_Rows            (TAG_USER|0x0042b68f) /* MUI: V4  is. LONG          */
#define MUIA_Group_SameHeight      (TAG_USER|0x0042037e) /* MUI: V4  i.. BOOL          */
#define MUIA_Group_SameSize        (TAG_USER|0x00420860) /* MUI: V4  i.. BOOL          */
#define MUIA_Group_SameWidth       (TAG_USER|0x0042b3ec) /* MUI: V4  i.. BOOL          */
#define MUIA_Group_Spacing         (TAG_USER|0x0042866d) /* MUI: V4  is. LONG          */
#define MUIA_Group_VertSpacing     (TAG_USER|0x0042e1bf) /* MUI: V4  isg LONG          */

enum {
    MUIV_Group_ActivePage_First = 0,
    MUIV_Group_ActivePage_Last = -1,
    MUIV_Group_ActivePage_Prev = -2,
    MUIV_Group_ActivePage_Next = -3,
    MUIV_Group_ActivePage_Advance = -4,
};

/* This is the message you get if your custom layout hook is called */
struct MUI_LayoutMsg
{
    ULONG              lm_Type;     /* the message type */
    struct MinList    *lm_Children; /* exec list of the children of this group */
    struct MUI_MinMax  lm_MinMax;   /* here you have to place the MUILM_MINMAX results */
    struct
    {
	LONG Width;
	LONG Height;
	ULONG priv5;
	ULONG priv6;
    } lm_Layout;   /* size (and result) for MUILM_LAYOUT */
};

/* lm_Type */
enum
{
    MUILM_MINMAX = 1,  /* Please calc your min & max siizes */
    MUILM_LAYOUT = 2,  /* Please layout your children */
};

#define MUILM_UNKNOWN  -1  /* should be returned if the hook function doesn't understand lm_Type */

/* The following tag is Zune only, it should not be used in MUI Programs */
#define MUIA_Group_Virtual  (TAG_USER|0x1042e1c0) /* Zune: V1 i.. BOOL  */

extern const struct __MUIBuiltinClass _MUI_Group_desc; /* PRIV */

#endif
