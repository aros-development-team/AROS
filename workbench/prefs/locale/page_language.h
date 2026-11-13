#ifndef _LLANG_H_
#define _LLANG_H_

/*
   Copyright © 2004-2010, The AROS Development Team. All rights reserved.
   $Id$
 */

#include <exec/types.h>
#include <libraries/mui.h>

/*** Identifier base ********************************************************/
#define MUIB_Language                   (TAG_USER | 0x30000000)

/*** Methods ****************************************************************/
#define MUIM_Language_GetBaseName       (MUIB_Language | 0)
#define MUIM_Language_GetNativeName     (MUIB_Language | 1)

/*** Attributes *************************************************************/
#define MUIA_Language_Preferred         (MUIB_Language | 0)
#define MUIA_Language_Characterset      (MUIB_Language | 1)

/*** Variables **************************************************************/
extern struct MUI_CustomClass *Language_CLASS;

/*** Macros *****************************************************************/
#define LanguageObject BOOPSIOBJMACRO_START(Language_CLASS->mcc_Class)

#endif /* _LLANG_H_ */
