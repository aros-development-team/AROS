#ifndef _IPEDITOR_H_
#define _IPEDITOR_H_

/*
    Copyright  2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <libraries/mui.h>

/*** Identifier base ********************************************************/
#define MUIB_IPEditor                  (TAG_USER | 0x10000000)

/*** Variables **************************************************************/
extern struct MUI_CustomClass *IPEditor_CLASS;

/*** Macros *****************************************************************/
#define IPEditorObject BOOPSIOBJMACRO_START(IPEditor_CLASS->mcc_Class)

#endif /* _IPEDITOR_H_ */
