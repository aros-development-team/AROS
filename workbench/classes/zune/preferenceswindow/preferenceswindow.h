#ifndef ZUNE_PREFERENCESWINDOW_H
#define ZUNE_PREFERENCESWINDOW_H

#include <libraries/mui.h>

/*** Name *******************************************************************/
#define MUIC_PreferencesWindow  "PreferencesWindow.mcc"

/*** Methods ****************************************************************/
#define MUIM_PreferencesWindow_Test     (METHOD_USER|0x00426801)
#define MUIM_PreferencesWindow_Revert   (METHOD_USER|0x00426802)
#define MUIM_PreferencesWindow_Save     (METHOD_USER|0x00426803)
#define MUIM_PreferencesWindow_Use      (METHOD_USER|0x00426804)
#define MUIM_PreferencesWindow_Cancel   (METHOD_USER|0x00426805)

/*** Macros *****************************************************************/
#define PreferencesWindowObject MUIOBJMACRO_START(MUIC_PreferencesWindow)

#endif /* ZUNE_PREFERENCESWINDOW_H */
