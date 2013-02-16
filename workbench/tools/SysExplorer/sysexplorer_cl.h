#ifndef SYSEXPLORER_CL_H
#define SYSEXPLORER_CL_H

/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <libraries/mui.h>

/*** Identifier base ********************************************************/
#define MUIB_SysExplorer                        (TAG_USER | 0x10000000)

/*** Variables **************************************************************/
extern struct MUI_CustomClass *SysExplorer_CLASS;

/*** Macros *****************************************************************/
#define SysExplorerObject BOOPSIOBJMACRO_START(SysExplorer_CLASS->mcc_Class)

#endif /* SYSEXPLORER_CL_H */
