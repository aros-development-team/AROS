#ifndef _FWEDITOR_H_
#define _FWEDITOR_H_

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Firewall Preferences -- PrefsEditor subclass.
*/

#include <exec/types.h>
#include <libraries/mui.h>

/*** Identifier base ********************************************************/
#define MUIB_FWEditor                   (TAG_USER | 0x46570000)

/*** Variables **************************************************************/
extern struct MUI_CustomClass *FWEditor_CLASS;

/*** Macros *****************************************************************/
#define FWEditorObject BOOPSIOBJMACRO_START(FWEditor_CLASS->mcc_Class)

/*** Custom method IDs *****************************************************/
#define MUIM_FWEditor_AddFilter         (MUIB_FWEditor | 0x0001)
#define MUIM_FWEditor_RemFilter         (MUIB_FWEditor | 0x0002)
#define MUIM_FWEditor_UpFilter          (MUIB_FWEditor | 0x0003)
#define MUIM_FWEditor_DownFilter        (MUIB_FWEditor | 0x0004)
#define MUIM_FWEditor_EditFilter        (MUIB_FWEditor | 0x0005)
#define MUIM_FWEditor_UseFilterEdit     (MUIB_FWEditor | 0x0006)
#define MUIM_FWEditor_CancelFilterEdit  (MUIB_FWEditor | 0x0007)
#define MUIM_FWEditor_AddNAT            (MUIB_FWEditor | 0x0010)
#define MUIM_FWEditor_RemNAT            (MUIB_FWEditor | 0x0011)
#define MUIM_FWEditor_EditNAT           (MUIB_FWEditor | 0x0012)
#define MUIM_FWEditor_UseNATEdit        (MUIB_FWEditor | 0x0013)
#define MUIM_FWEditor_CancelNATEdit     (MUIB_FWEditor | 0x0014)

#endif /* _FWEDITOR_H_ */
