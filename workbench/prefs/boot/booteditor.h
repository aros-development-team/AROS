#ifndef _BOOTEDITOR_H_
#define _BOOTEDITOR_H_

/*
    Copyright © 2009-2015, The AROS Development Team. All rights reserved.
    $Id$
 */

#include <exec/types.h>
#include <libraries/mui.h>

/*** Identifier base ********************************************************/
#define MUIB_BootEditor              (MUIB_PrefsEditor + 0x100)

/*** Variables **************************************************************/
extern struct MUI_CustomClass *BootEditor_CLASS;

/*** Macros *****************************************************************/
#define BootEditorObject BOOPSIOBJMACRO_START(BootEditor_CLASS->mcc_Class)

#define MUIM_BootEditor_ShowModule   (MUIB_BootEditor + 1)
#define MUIM_BootEditor_UpdateModule (MUIB_BootEditor + 2)
#define MUIM_BootEditor_AddModule    (MUIB_BootEditor + 3)
#define MUIM_BootEditor_RemoveModule (MUIB_BootEditor + 4)

#endif /* _BOOTEDITOR_H_ */
