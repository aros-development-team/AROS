#ifndef FINDGROUP_CLASS_H
#define FINDGROUP_CLASS_H

/*
    Copyright © 2016, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <libraries/mui.h>

/*** Identifier base ********************************************************/
#define MUIB_FindGroup                  (TAG_USER | 0x10000000)

/*** Attributes *************************************************************/
#define MUIA_FindGroup_Path             (MUIB_FindGroup | 0)
#define MUIA_FindGroup_Pattern          (MUIB_FindGroup | 1)
#define MUIA_FindGroup_Contents         (MUIB_FindGroup | 2)

/*** Variables **************************************************************/
extern struct MUI_CustomClass *FindGroup_CLASS;

/*** Macros *****************************************************************/
#define FindGroupObject BOOPSIOBJMACRO_START(FindGroup_CLASS->mcc_Class)


#endif /* FINDGROUP_CLASS_H */
