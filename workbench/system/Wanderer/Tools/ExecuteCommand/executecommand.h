#ifndef _EXECUTECOMMAND_H_
#define _EXECUTECOMMAND_H_

/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <libraries/mui.h>

/*** Attributes *************************************************************/
/*+
    [I..] BPTR
    Lock on parent directory.
+*/
#define MUIA_ExecuteCommand_Parent  (TAG_USER | 0x20000000)  

/*+
    [I..] CONST_STRPTR
    Initial command line.
+*/
#define MUIA_ExecuteCommand_Initial (TAG_USER | 0x20000001)

/*** Variables **************************************************************/
extern struct MUI_CustomClass *ExecuteCommand_CLASS;

/*** Macros *****************************************************************/
#define ExecuteCommandObject BOOPSIOBJMACRO_START(ExecuteCommand_CLASS->mcc_Class)

/*** Prototypes *************************************************************/
BOOL ExecuteCommand_Initialize();
void ExecuteCommand_Deinitialize();

#endif /* _EXECUTECOMMAND_H_ */
