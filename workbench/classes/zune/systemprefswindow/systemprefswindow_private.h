#ifndef _SYSTEMPREFSWINDOW_PRIVATE_H_
#define _SYSTEMPREFSWINDOW_PRIVATE_H_

/*
    Copyright � 2004, The AROS Development Team. All rights reserved.
    This file is part of the SystemPrefsWindow class, which is distributed under
    the terms of version 2.1 of the GNU Lesser General Public License.
    
    $Id$
*/

#include <intuition/classusr.h>
#include <libraries/locale.h>
#include <libraries/asl.h>

/*** Instance data **********************************************************/
struct SystemPrefsWindow_DATA
{
    struct Catalog       *spwd_Catalog;
    Object               *spwd_Editor;
    struct FileRequester *spwd_FileRequester;
};

/*** Private methods ********************************************************/
#define MUIM_SystemPrefsWindow_UpdateButtons  (TAG_USER | 0x10000000)

#endif /* _SYSTEMPREFSWINDOW_PRIVATE_H_ */
