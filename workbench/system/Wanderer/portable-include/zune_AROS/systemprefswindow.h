#ifndef ZUNE_SYSTEMPREFSWINDOW_H
#define ZUNE_SYSTEMPREFSWINDOW_H

/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    This file is part of the SystemPrefsWindow class, which is distributed under
    the terms of version 2.1 of the GNU Lesser General Public License.
    
    $Id$
*/

#include <libraries/mui.h>

/*** Name *******************************************************************/
#define MUIC_SystemPrefsWindow           "SystemPrefsWindow.mcc"

/*** Identifier base ********************************************************/
#define MUIB_SystemPrefsWindow           (MUIB_AROS | 0x00000600)

/*** Macros *****************************************************************/
#define SystemPrefsWindowObject MUIOBJMACRO_START(MUIC_SystemPrefsWindow)

#endif /* ZUNE_SYSTEMPREFSWINDOW_H */
