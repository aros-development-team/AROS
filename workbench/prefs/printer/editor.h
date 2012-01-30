#ifndef _PRINTEREDITOR_H_
#define _PRINTEREDITOR_H_

/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <libraries/mui.h>

/*** Identifier base ********************************************************/
#define MUIB_PrinterEditor                  (TAG_USER | 0x10000000)

#define MUIM_PrinterEditor_SelfCheck        (MUIB_PrinterEditor + 1)

/*** Variables **************************************************************/
extern struct MUI_CustomClass *PrinterEditor_CLASS;

/*** Macros *****************************************************************/
#define PrinterEditorObject BOOPSIOBJMACRO_START(PrinterEditor_CLASS->mcc_Class)

#endif /* _PRINTEREDITOR_H_ */
