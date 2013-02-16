#ifndef COMPUTERPAGE_CL_H
#define COMPUTERPAGE_CL_H

/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <libraries/mui.h>

/*** Identifier base ********************************************************/
#define MUIB_ComputerPage               (TAG_USER | 0x10000000)

/*** Methods ****************************************************************/
#define MUIM_ComputerPage_Update        (MUIB_ComputerPage | 0x00000000)

/*** Variables **************************************************************/
extern struct MUI_CustomClass *ComputerPage_CLASS;

/*** Macros *****************************************************************/
#define ComputerPageObject BOOPSIOBJMACRO_START(ComputerPage_CLASS->mcc_Class)

#endif /* COMPUTERPAGE_CL_H */
