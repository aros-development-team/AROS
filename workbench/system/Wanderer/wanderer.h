#ifndef _WANDERER_H_
#define _WANDERER_H_

/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <libraries/mui.h>

/*** Identifier Base ********************************************************/
#define MUIB_Wanderer                     (TAG_USER | 0x11000000)

/*** Public Attributes ******************************************************/
#define MUIA_Wanderer_Prefs               (MUIB_Wanderer | 0x00000000)
#define MUIA_Wanderer_ActiveWindow        (MUIB_Wanderer | 0x00000001)
#define MUIA_Wanderer_WorkbenchWindow     (MUIB_Wanderer | 0x00000002)

/*** Private Methods ********************************************************/
#define MUIM_Wanderer_HandleTimer         (MUIB_Wanderer | 0x00000000)
#define MUIM_Wanderer_HandleCommand       (MUIB_Wanderer | 0x00000001)
#define MUIM_Wanderer_HandleNotify        (MUIB_Wanderer | 0x00000002)
#define MUIM_Wanderer_CreateDrawerWindow  (MUIB_Wanderer | 0x00000003)
struct  MUIP_Wanderer_CreateDrawerWindow  {ULONG MethodID; CONST_STRPTR drawer;};

/*** Variables **************************************************************/
extern struct MUI_CustomClass *Wanderer_CLASS;

/*** Macros *****************************************************************/
#define WandererObject BOOPSIOBJMACRO_START(Wanderer_CLASS->mcc_Class)

#endif /* _WANDERER_H_ */
