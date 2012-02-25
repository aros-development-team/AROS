#ifndef KEYBOARDGROUP_CLASS_H
#define KEYBOARDGROUP_CLASS_H

/*
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <libraries/mui.h>

/*** Identifier base ********************************************************/
#define MUIB_KeyboardGroup              (TAG_USER | 0x10000000)

/*** Attributes *************************************************************/

/*** Variables **************************************************************/
extern struct MUI_CustomClass *KeyboardGroup_CLASS;

/*** Macros *****************************************************************/
#define KeyboardGroupObject BOOPSIOBJMACRO_START(KeyboardGroup_CLASS->mcc_Class)


#endif /* KEYBOARDGROUP_H */
