#ifndef _MUI_CLASSES_POPASL_H
#define _MUI_CLASSES_POPASL_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Popasl             "Popasl.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Popasl            (MUIB_ZUNE | 0x00002100)


/*** Attributes *************************************************************/
#define MUIA_Popasl_Active     (MUIB_MUI|0x00421b37) /* MUI: V7  ..g BOOL          */
#define MUIA_Popasl_StartHook  (MUIB_MUI|0x0042b703) /* MUI: V7  isg struct Hook * */
#define MUIA_Popasl_StopHook   (MUIB_MUI|0x0042d8d2) /* MUI: V7  isg struct Hook * */
#define MUIA_Popasl_Type       (MUIB_MUI|0x0042df3d) /* MUI: V7  i.g ULONG         */


extern const struct __MUIBuiltinClass _MUI_Popasl_desc; /* PRIV */

#endif /* _MUI_CLASSES_POPASL_H */
