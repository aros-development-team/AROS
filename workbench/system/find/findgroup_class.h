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

/*** Methods ****************************************************************/
#define MUIM_FindGroup_Start            (MUIB_FindGroup | 0)
#define MUIM_FindGroup_Stop             (MUIB_FindGroup | 1)
#define MUIM_FindGroup_AddEntry         (MUIB_FindGroup | 2)

/*** Messages ***************************************************************/

struct MUIP_FindGroup_AddEntry
{
    STACKED ULONG MethodID;
    STACKED struct Listentry *entry;
};

/*** Variables **************************************************************/
extern struct MUI_CustomClass *FindGroup_CLASS;

/*** Macros *****************************************************************/
#define FindGroupObject BOOPSIOBJMACRO_START(FindGroup_CLASS->mcc_Class)


#endif /* FINDGROUP_CLASS_H */
