/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _MUI_CLASSES_ICONLIST_H
#define _MUI_CLASSES_ICONLIST_H

#ifndef METHOD_USER
#define METHOD_USER TAG_USER
#endif

/************/
/* IconList */
/************/
#define MUIC_IconList "IconList.zune"

/* IconList methods */
#define MUIM_IconList_Clear               (METHOD_USER|0x1042ad89) /* Zune: V1 */
#define MUIM_IconList_Update              (METHOD_USER|0x1042ad8a) /* Zune: V1 */
#define MUIM_IconList_Add                 (METHOD_USER|0x1042ad8b) /* Zune: V1 returns BOOL */
struct MUIP_IconList_Clear                {ULONG MethodID;};
struct MUIP_IconList_Update               {ULONG MethodID;};
struct MUIP_IconList_Add                  {ULONG MethodID; struct DiskObject *dob; char *filename; void *udata; /* More to add */};

/* IconList attrs */
#define MUIA_IconList_DoubleClick         (TAG_USER|0x10427878) /* Zune: V1 ..g BOOL */

/******************/
/* IconDrawerList */
/******************/
#define MUIC_IconDrawerList "IconDrawerList.zune"

/* IconDrawerList attributes */
#define MUIA_IconDrawerList_Drawer        (TAG_USER|0x1042391c) /* Zune: V1  isg LONG     */

/*******************/
/* IconVolumneList */
/*******************/
#define MUIC_IconVolumeList "IconVolumneList.zune"

extern const struct __MUIBuiltinClass _MUI_IconList_desc; /* PRIV */
extern const struct __MUIBuiltinClass _MUI_IconDrawerList_desc; /* PRIV */
extern const struct __MUIBuiltinClass _MUI_IconVolumeList_desc; /* PRIV */


#endif
