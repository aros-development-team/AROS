#ifndef _PREFSWINDOW_PRIVATE_H_
#define _PREFSWINDOW_PRIVATE_H_

/*
    Copyright © 2003-2004, The AROS Development Team. All rights reserved.
    This file is part of the PrefsWindow class, which is distributed under
    the terms of version 2.1 of the GNU Lesser General Public License.
    
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
