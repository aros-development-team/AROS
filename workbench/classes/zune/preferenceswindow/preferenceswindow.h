#ifndef PREFERENCESWINDOW_H
#define PREFERENCESWINDOW_H

#define MUIMASTER_YES_INLINE_STDARG
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

#endif /* PREFERENCESWINDOW_H */
