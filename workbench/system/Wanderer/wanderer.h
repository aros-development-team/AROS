#ifndef _WANDERER_H_
#define _WANDERER_H_

/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <libraries/mui.h>

/*** Identifier Base ********************************************************/
#define MUIB_Wanderer                   (TAG_USER | 0x11000000)

/*** Private Methods ********************************************************/
#define MUIM_Wanderer_HandleTimer       (MUIB_Wanderer | 0x00000000)
#define MUIM_Wanderer_HandleNotify      (MUIB_Wanderer | 0x00000001)
#define MUIM_Wanderer_HandlePrefsNotify (MUIB_Wanderer | 0x00000002)

/*** Variables **************************************************************/
extern struct MUI_CustomClass *Wanderer_CLASS;

/*** Macros *****************************************************************/
#define WandererObject BOOPSIOBJMACRO_START(Wanderer_CLASS->mcc_Class)

#endif /* _WANDERER_H_ */
