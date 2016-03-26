#ifndef FINDGROUP_CLASS_H
#define FINDGROUP_CLASS_H

/*
    Copyright © 2016, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <libraries/mui.h>

/*** Identifier base ********************************************************/
#define MUIB_FindGroup                 (TAG_USER | 0x10000000)

/*** Attributes *************************************************************/
//#define MUIA_UnarcGroup_Archive         (MUIB_UnarcGroup | 0)
//#define MUIA_UnarcGroup_Destination     (MUIB_UnarcGroup | 1)

/*** Variables **************************************************************/
extern struct MUI_CustomClass *FindGroup_CLASS;

/*** Macros *****************************************************************/
#define FindGroupObject BOOPSIOBJMACRO_START(FindGroup_CLASS->mcc_Class)


#endif /* FINDGROUP_CLASS_H */
