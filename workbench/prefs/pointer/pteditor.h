#ifndef _PTEDITOR_H_
#define _PTEDITOR_H_

/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <libraries/mui.h>

/*** Identifier base ********************************************************/
#define MUIB_PTEditor                  (TAG_USER | 0x10000000)

/*** Variables **************************************************************/
extern struct MUI_CustomClass *PTEditor_CLASS;

/*** Macros *****************************************************************/
#define PTEditorObject BOOPSIOBJMACRO_START(PTEditor_CLASS->mcc_Class)


#endif /* _PTEDITOR_H_ */
