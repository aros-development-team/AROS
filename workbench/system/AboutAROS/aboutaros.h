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

/*** Macros *****************************************************************/
#define AboutAROSObject BOOPSIOBJMACRO_START(AboutAROS_CLASS->mcc_Class)


#endif /* _ABOUTAROS_H_ */
