#ifndef ZUNE_PREFERENCESWINDOW_H
#define ZUNE_PREFERENCESWINDOW_H

#include <libraries/mui.h>

/*** Name *******************************************************************/
#define MUIC_PreferencesWindow                 "PreferencesWindow.mcc"

/*** Identifier base ********************************************************/
#define MUIB_PreferencesWindow                 (MUIB_AROS | 0x00000200)

/*** Public (Abstract) Methods **********************************************/
#define MUIM_PreferencesWindow_Test            (MUIB_PreferencesWindow | 0x00000000)
#define MUIM_PreferencesWindow_Revert          (MUIB_PreferencesWindow | 0x00000001)
#define MUIM_PreferencesWindow_Save            (MUIB_PreferencesWindow | 0x00000002)
#define MUIM_PreferencesWindow_Use             (MUIB_PreferencesWindow | 0x00000003)
#define MUIM_PreferencesWindow_Cancel          (MUIB_PreferencesWindow | 0x00000004)

/*** Protected Attributes ***************************************************/
#define MUIA_PreferencesWindow_Test_Disabled   (MUIB_PreferencesWindow | 0x00000000)
#define MUIA_PreferencesWindow_Revert_Disabled (MUIB_PreferencesWindow | 0x00000001)
#define MUIA_PreferencesWindow_Save_Disabled   (MUIB_PreferencesWindow | 0x00000002)
#define MUIA_PreferencesWindow_Use_Disabled    (MUIB_PreferencesWindow | 0x00000003)
#define MUIA_PreferencesWindow_Cancel_Disabled (MUIB_PreferencesWindow | 0x00000004)

/*** Macros *****************************************************************/
#define PreferencesWindowObject MUIOBJMACRO_START(MUIC_PreferencesWindow)

#endif /* ZUNE_PREFERENCESWINDOW_H */
