#ifndef _PREFSWINDOW_PRIVATE_H_
#define _PREFSWINDOW_PRIVATE_H_

/*
    Copyright © 2003-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Instance data **********************************************************/
struct PrefsWindow_DATA
{
    struct Catalog *pwd_Catalog;
    Object         *pwd_TestButton,
                   *pwd_RevertButton,
                   *pwd_SaveButton,
                   *pwd_UseButton,
                   *pwd_CancelButton;
};

#endif /* _PREFSWINDOW_PRIVATE_H_ */
