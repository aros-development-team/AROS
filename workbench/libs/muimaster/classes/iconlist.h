/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _MUI_CLASSES_ICONLIST_H
#define _MUI_CLASSES_ICONLIST_H

/************/
/* IconList */
/************/
#define MUIC_IconList "IconList.zune"

/* IconList methods */
#define MUIM_IconList_Clear             (MUIB_MUI|0x1042ad89) /* Zune: V1 */
#define MUIM_IconList_Update            (MUIB_MUI|0x1042ad8a) /* Zune: V1 */
#define MUIM_IconList_Add               (MUIB_MUI|0x1042ad8b) /* Zune: V1 returns BOOL */
#define MUIM_IconList_NextSelected      (MUIB_MUI|0x1042ad8c) /* Zune: V1 */
#define MUIM_IconList_UnselectAll		  (MUIB_MUI|0x1042ad8d) /* Zune: V1 */
struct MUIP_IconList_Clear              {ULONG MethodID;};
struct MUIP_IconList_Update             {ULONG MethodID;};
struct MUIP_IconList_Add                {ULONG MethodID; char *filename; char *label; LONG type; void *udata; /* More file attrs to add */};
struct MUIP_IconList_NextSelected       {ULONG MethodID; struct IconList_Entry **entry;}; /* *entry maybe MUIV_IconList_NextSelected_Start, *entry is MUIV_IconList_NextSelected_End if no more entries are selected */

#define MUIV_IconList_NextSelected_Start 0
#define MUIV_IconList_NextSelected_End   0

/* IconList attrs */
#define MUIA_IconList_DoubleClick         (MUIB_MUI|0x10427878) /* Zune: V1 ..G BOOL */
#define MUIA_IconList_Left                (MUIB_MUI|0x10427879) /* Zune: V1 ..G LONG */
#define MUIA_IconList_Top                 (MUIB_MUI|0x1042787a) /* Zune: V1 ..G LONG */
#define MUIA_IconList_Width               (MUIB_MUI|0x1042787b) /* Zune: V1 ..G LONG */
#define MUIA_IconList_Height              (MUIB_MUI|0x1042787c) /* Zune: V1 ..G LONG */
#define MUIA_IconList_IconsDropped				(MUIB_MUI|0x1042787d) /* Zune: V1 ..G struct IconList_Entry * */
#define MUIA_IconList_Clicked						(MUIB_MUI|0x1042787e) /* Zune: V1 ..G struct IconList_Click * */

/* used by MUIM_IconList_NextSelected */
struct IconList_Entry
{
    char *filename;  /* The absolute filename of the file which the icons represents (means without the */
    char *label;     /* The label which is displayed (often FilePart(filename)) */
    LONG type;
    void *udata;     /* userdate given at MUIM_IconList_Add */
};

struct IconList_Click
{
    int shift; /* TRUE for shift click */
    struct IconList_Entry *entry; /* might be NULL */
};

/******************/
/* IconDrawerList */
/******************/
#define MUIC_IconDrawerList "IconDrawerList.zune"

/* IconDrawerList attributes */
#define MUIA_IconDrawerList_Drawer        (MUIB_MUI|0x1042391c) /* Zune: V1  isg LONG     */

/*******************/
/* IconVolumneList */
/*******************/
#define MUIC_IconVolumeList "IconVolumneList.zune"

extern const struct __MUIBuiltinClass _MUI_IconList_desc; /* PRIV */
extern const struct __MUIBuiltinClass _MUI_IconDrawerList_desc; /* PRIV */
extern const struct __MUIBuiltinClass _MUI_IconVolumeList_desc; /* PRIV */


#endif
