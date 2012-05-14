#ifndef _SEREDITOR_H_
#define _SEREDITOR_H_

/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <libraries/mui.h>

/*** Identifier base ********************************************************/
#define MUIB_BibEditor                  (TAG_USER | 0x10000000)

/*** Variables **************************************************************/
extern struct MUI_CustomClass *BibEditor_CLASS;

/*** Macros *****************************************************************/
#define BibEditorObject BOOPSIOBJMACRO_START(BibEditor_CLASS->mcc_Class)

#endif /* _SEREDITOR_H_ */
