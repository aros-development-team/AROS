/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _MUI_CLASSES_POPOBJECT_H
#define _MUI_CLASSES_POPOBJECT_H

#define MUIC_Popobject "Popobject.mui"

/* Popobject attributes */
#define MUIA_Popobject_Follow       (TAG_USER|0x00424cb5) /* MUI: V7  isg BOOL          */
#define MUIA_Popobject_Light        (TAG_USER|0x0042a5a3) /* MUI: V7  isg BOOL          */
#define MUIA_Popobject_Object       (TAG_USER|0x004293e3) /* MUI: V7  i.g Object *      */
#define MUIA_Popobject_ObjStrHook   (TAG_USER|0x0042db44) /* MUI: V7  isg struct Hook * */
#define MUIA_Popobject_StrObjHook   (TAG_USER|0x0042fbe1) /* MUI: V7  isg struct Hook * */
#define MUIA_Popobject_Volatile     (TAG_USER|0x004252ec) /* MUI: V7  isg BOOL          */
#define MUIA_Popobject_WindowHook   (TAG_USER|0x0042f194) /* MUI: V9  isg struct Hook * */

/**************************************************************************
 Poplist
**************************************************************************/
#define MUIC_Poplist "Poplist.mui"

/* Poplist attributes */
#define MUIA_Poplist_Array          (TAG_USER|0x0042084c) /* MUI: V8  i.. char ** */

/**************************************************************************
 Popscreen
**************************************************************************/
#define MUIC_Popscreen "Popscreen.mui"

extern const struct __MUIBuiltinClass _MUI_Popobject_desc; /* PRIV */

#endif
