#ifndef GUI_WINDOW_H
#define GUI_WINDOW_H

/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <libraries/mui.h>

/*** Variables **************************************************************/
extern struct MUI_CustomClass *FPWindow_CLASS;

/*** Macros *****************************************************************/
#define FPWindowObject BOOPSIOBJMACRO_START(FPWindow_CLASS->mcc_Class)

/*** Prototypes *************************************************************/
BOOL FPWindow_Initialize();
void FPWindow_Deinitialize();

#endif /* GUI_WINDOW_H */
