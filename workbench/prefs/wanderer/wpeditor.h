#ifndef _WPEDITOR_H_
#define _WPEDITOR_H_

/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    This file is part of the Wanderer Preferences program, which is distributed
    under the terms of version 2 of the GNU General Public License.
    
    $Id$
*/

#include <exec/types.h>
#include <libraries/mui.h>
#include <aros/debug.h>
#define DEBUG 1

/*** Identifier base ********************************************************/
#define MUIB_WPEditor                  (TAG_USER | 0x10000000)

/*** Variables **************************************************************/
extern struct MUI_CustomClass *WPEditor_CLASS;

/*** Macros *****************************************************************/
#define WPEditorObject BOOPSIOBJMACRO_START(WPEditor_CLASS->mcc_Class)

#endif /* _WPEDITOR_H_ */
