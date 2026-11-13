#ifndef _LLANGLIST_H_
#define _LLANGLIST_H_

/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <libraries/mui.h>

/*** Variables **************************************************************/
extern struct MUI_CustomClass *Languagelist_CLASS;

/*** Macros *****************************************************************/
#define LanguagelistObject BOOPSIOBJMACRO_START(Language_CLASS->mcc_Class)

#endif /* _LLANGLIST_H_ */
