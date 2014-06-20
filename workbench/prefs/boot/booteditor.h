#ifndef _BOOTEDITOR_H_
#define _BOOTEDITOR_H_

/*
    Copyright © 2009-2013, The AROS Development Team. All rights reserved.
    $Id$
 */

#include <exec/types.h>
#include <libraries/mui.h>

/*** Variables **************************************************************/
extern struct MUI_CustomClass *BootEditor_CLASS;

/*** Macros *****************************************************************/
#define BootEditorObject BOOPSIOBJMACRO_START(BootEditor_CLASS->mcc_Class)

#endif /* _BOOTEDITOR_H_ */
