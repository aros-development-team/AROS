#ifndef _ABOUTAROS_H_
#define _ABOUTAROS_H_

/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <libraries/mui.h>

/*** Variables **************************************************************/
extern struct MUI_CustomClass *AboutAROS_CLASS;

/*** Methods ****************************************************************/
#define MUIM_Application_Execute (TAG_USER | 0x20000001)

/*** Macros *****************************************************************/
#define AboutAROSObject BOOPSIOBJMACRO_START(AboutAROS_CLASS->mcc_Class)

/*** Prototypes *************************************************************/
BOOL AboutAROS_Initialize();
void AboutAROS_Deinitialize();

#endif /* _ABOUTAROS_H_ */
