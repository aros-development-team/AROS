#ifndef _PREFERENCESWINDOW_PRIVATE_H_
#define _PREFERENCESWINDOW_PRIVATE_H_

/*** Instance data **********************************************************/
struct PreferencesWindow_DATA
{
    struct Catalog *pwd_Catalog;
    Object         *pwd_TestButton,
                   *pwd_RevertButton,
                   *pwd_SaveButton,
                   *pwd_UseButton,
                   *pwd_CancelButton;
};

#endif /* _PREFERENCESWINDOW_PRIVATE_H_ */
