/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _MUI_CLASSES_POPASL_H
#define _MUI_CLASSES_POPASL_H

#define MUIC_Popasl "Popasl.mui"

/* Popasl attributes */
#define MUIA_Popasl_Active     (TAG_USER|0x00421b37) /* MUI: V7  ..g BOOL          */
#define MUIA_Popasl_StartHook  (TAG_USER|0x0042b703) /* MUI: V7  isg struct Hook * */
#define MUIA_Popasl_StopHook   (TAG_USER|0x0042d8d2) /* MUI: V7  isg struct Hook * */
#define MUIA_Popasl_Type       (TAG_USER|0x0042df3d) /* MUI: V7  i.g ULONG         */

extern const struct __MUIBuiltinClass _MUI_Popasl_desc; /* PRIV */

#endif
