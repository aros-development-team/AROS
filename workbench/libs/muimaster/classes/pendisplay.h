#ifndef _MUI_CLASSES_PENDISPLAY_H
#define _MUI_CLASSES_PENDISPLAY_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Pendisplay              "Pendisplay.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Pendisplay              (MUIB_ZUNE | 0x00002000)  

/*** Methods ****************************************************************/
#define MUIM_Pendisplay_SetColormap  (MUIB_MUI|0x004243a7) /* MUI: V13 */
#define MUIM_Pendisplay_SetMUIPen    (MUIB_MUI|0x00426ecd) /* MUI: V13 */
#define MUIM_Pendisplay_SetRGB       (MUIB_MUI|0x0042032c) /* MUI: V13 */
struct MUIP_Pendisplay_SetColormap   {STACKED ULONG MethodID; STACKED LONG colormap;};
struct MUIP_Pendisplay_SetMUIPen     {STACKED ULONG MethodID; STACKED LONG muipen;};
struct MUIP_Pendisplay_SetRGB        {STACKED ULONG MethodID; STACKED ULONG r; STACKED ULONG g; STACKED ULONG b;};

/*** Attributes *************************************************************/
#define MUIA_Pendisplay_Pen        (MUIB_MUI|0x0042a748) /* MUI: V13  ..g Object *       */
#define MUIA_Pendisplay_Reference  (MUIB_MUI|0x0042dc24) /* MUI: V13  isg Object *       */
#define MUIA_Pendisplay_RGBcolor   (MUIB_MUI|0x0042a1a9) /* MUI: V11  isg struct MUI_RGBcolor * */
#define MUIA_Pendisplay_Spec       (MUIB_MUI|0x0042a204) /* MUI: V11  isg struct MUI_PenSpec  * */


extern const struct __MUIBuiltinClass _MUI_Pendisplay_desc; /* PRIV */

#endif /* _MUI_CLASSES_PENDISPLAY_H */
