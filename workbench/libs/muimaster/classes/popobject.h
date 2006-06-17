#ifndef _MUI_CLASSES_POPOBJECT_H
#define _MUI_CLASSES_POPOBJECT_H

/*
    Copyright © 2002-2006, The AROS Development Team. All rights reserved.
    $Id$
*/

/****************************************************************************/
/*** Name *******************************************************************/
#define MUIC_Popobject              "Popobject.mui"

/*** Identifier base ********************************************************/
#define MUIB_Popobject              (MUIB_ZUNE | 0x00002400)

/*** Attributes *************************************************************/
#define MUIA_Popobject_Follow       (MUIB_MUI|0x00424cb5) /* MUI: V7  isg BOOL          */
#define MUIA_Popobject_Light        (MUIB_MUI|0x0042a5a3) /* MUI: V7  isg BOOL          */
#define MUIA_Popobject_Object       (MUIB_MUI|0x004293e3) /* MUI: V7  i.g Object *      */
#define MUIA_Popobject_ObjStrHook   (MUIB_MUI|0x0042db44) /* MUI: V7  isg struct Hook * */
#define MUIA_Popobject_StrObjHook   (MUIB_MUI|0x0042fbe1) /* MUI: V7  isg struct Hook * */
#define MUIA_Popobject_Volatile     (MUIB_MUI|0x004252ec) /* MUI: V7  isg BOOL          */
#define MUIA_Popobject_WindowHook   (MUIB_MUI|0x0042f194) /* MUI: V9  isg struct Hook * */


extern const struct __MUIBuiltinClass _MUI_Popobject_desc; /* PRIV */

#endif /*_MUI_CLASSES_POPOBJECT_H */
