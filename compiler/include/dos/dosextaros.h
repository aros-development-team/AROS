#ifndef DOS_DOSEXTAROS_H
#define DOS_DOSEXTAROS_H

#ifndef AROS_DOS_PACKETS

/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: LibBase and some important structures
    Lang: English
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef EXEC_DEVICES_H
#   include <exec/devices.h>
#endif

/* Aros specific extension of struct DosList
 */
struct DosListAROSExt
{
    STRPTR dol_DevName;
    struct Device * dol_Device;
    struct Unit   * dol_Unit;
};
#define dl_DevName dol_DevName
#define dl_Device dol_Device
#define dl_Unit dol_Unit
#define dvi_DevName dol_DevName
#define dvi_Device dol_Device
#define dvi_Unit dol_Unit
#define dn_DevName dol_DevName
#define dn_Device dol_Device
#define dn_Unit dol_Unit

#define AROS_DOSDEVNAME(dn) (((struct DosList*)(dn))->dol_Ext.dol_AROS.dol_DevName)

#else

#define AROS_DOSDEVNAME(dn) AROS_BSTR_ADDR(((struct DosList*)(dn))->dol_Name)

#endif

#endif /* DOS_DOSEXTAROS_H */
