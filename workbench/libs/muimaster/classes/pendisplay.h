/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _MUI_CLASSES_PENDISPLAY_H
#define _MUI_CLASSES_PENDISPLAY_H

#define MUIC_Pendisplay "Pendisplay.mui"

#ifndef METHOD_USER
#define METHOD_USER TAG_USER
#endif

/* Pendisplay methods */
#define MUIM_Pendisplay_SetColormap   (METHOD_USER|0x004243a7) /* MUI: V13 */
#define MUIM_Pendisplay_SetMUIPen     (METHOD_USER|0x00426ecd) /* MUI: V13 */
#define MUIM_Pendisplay_SetRGB        (METHOD_USER|0x0042032c) /* MUI: V13 */
struct MUIP_Pendisplay_SetColormap    {ULONG MethodID; LONG colormap;};
struct MUIP_Pendisplay_SetMUIPen      {ULONG MethodID; LONG muipen;};
struct MUIP_Pendisplay_SetRGB         {ULONG MethodID; ULONG r; ULONG g; ULONG b;};

/* Pendisplay attributes */
#define MUIA_Pendisplay_Pen        (TAG_USER|0x0042a748) /* MUI: V13  ..g Object *       */
#define MUIA_Pendisplay_Reference  (TAG_USER|0x0042dc24) /* MUI: V13  isg Object *       */
#define MUIA_Pendisplay_RGBcolor   (TAG_USER|0x0042a1a9) /* MUI: V11  isg struct MUI_RGBcolor * */
#define MUIA_Pendisplay_Spec       (TAG_USER|0x0042a204) /* MUI: V11  isg struct MUI_PenSpec  * */

extern const struct __MUIBuiltinClass _MUI_Pendisplay_desc; /* PRIV */

#endif
