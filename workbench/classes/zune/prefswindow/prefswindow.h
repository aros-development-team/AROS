#ifndef ZUNE_PREFSWINDOW_H
#define ZUNE_PREFSWINDOW_H

/*
    Copyright © 2003-2004, The AROS Development Team. All rights reserved.
    This file is part of the PrefsWindow class, which is distributed under
    the terms of version 2.1 of the GNU Lesser General Public License.
    
    $Id$
*/

#include <libraries/mui.h>

/*** Name *******************************************************************/
#define MUIC_PrefsWindow                 "PrefsWindow.mcc"

/*** Identifier base ********************************************************/
#define MUIB_PrefsWindow                 (MUIB_AROS | 0x00000200)

/*** Public (Abstract) Methods **********************************************/
#define MUIM_PrefsWindow_Test            (MUIB_PrefsWindow | 0x00000000)
#define MUIM_PrefsWindow_Revert          (MUIB_PrefsWindow | 0x00000001)
#define MUIM_PrefsWindow_Save            (MUIB_PrefsWindow | 0x00000002)
#define MUIM_PrefsWindow_Use             (MUIB_PrefsWindow | 0x00000003)
#define MUIM_PrefsWindow_Cancel          (MUIB_PrefsWindow | 0x00000004)

/*** Protected Attributes ***************************************************/
#define MUIA_PrefsWindow_Test_Disabled   (MUIB_PrefsWindow | 0x00000000)
#define MUIA_PrefsWindow_Revert_Disabled (MUIB_PrefsWindow | 0x00000001)
#define MUIA_PrefsWindow_Save_Disabled   (MUIB_PrefsWindow | 0x00000002)
#define MUIA_PrefsWindow_Use_Disabled    (MUIB_PrefsWindow | 0x00000003)
#define MUIA_PrefsWindow_Cancel_Disabled (MUIB_PrefsWindow | 0x00000004)

/*** Macros *****************************************************************/
#define PrefsWindowObject MUIOBJMACRO_START(MUIC_PrefsWindow)

#endif /* ZUNE_PREFSWINDOW_H */
