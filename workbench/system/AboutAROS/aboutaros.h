#ifndef _ABOUTAROS_H_
#define _ABOUTAROS_H_

/*
    Copyright © 2003-2004, The AROS Development Team. All rights reserved.
    This file is part of the About program, which is distributed under
    the terms of version 2 of the GNU General Public License.
    
    $Id$
*/

#include <exec/types.h>
#include <libraries/mui.h>

/*** Variables **************************************************************/
extern struct MUI_CustomClass *AboutAROS_CLASS;

/*** Macros *****************************************************************/
#define AboutAROSObject BOOPSIOBJMACRO_START(AboutAROS_CLASS->mcc_Class)


#endif /* _ABOUTAROS_H_ */
