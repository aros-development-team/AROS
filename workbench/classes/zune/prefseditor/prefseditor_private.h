#ifndef _PREFSEDITOR_PRIVATE_H_
#define _PREFSEDITOR_PRIVATE_H_

/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    This file is part of the PrefsEditor class, which is distributed under
    the terms of version 2.1 of the GNU Lesser General Public License.
    
    $Id$
*/

#include <exec/types.h>
#include <dos/bptr.h>

/*** Instance data **********************************************************/
struct PrefsEditor_DATA
{
    BOOL   ped_Changed,
           ped_Testing;
    STRPTR ped_Name,
           ped_Path;
    BPTR   ped_BackupFH;
    TEXT   ped_BackupPath[15];
           /* Large enough for BACKUP_PREFIX, 8 digits and NULL byte */
};

#define BACKUP_PREFIX "T:PT"

/*** Private methods ********************************************************/
#define MUIM_PrefsEditor_ExportToDirectory   (TAG_USER | 0x10000000)
struct  MUIP_PrefsEditor_ExportToDirectory   {ULONG MethodID; CONST_STRPTR directory;};
#define MUIM_PrefsEditor_ImportFromDirectory (TAG_USER | 0x10000001)
struct  MUIP_PrefsEditor_ImportFromDirectory {ULONG MethodID; CONST_STRPTR directory;};

#endif /* _PREFSEDITOR_PRIVATE_H_ */
