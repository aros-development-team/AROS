/*
** DevWinClass - a per-device information window (subclass of Window.mui),
** opened when a device on the Devices page is double clicked. It shows the
** device details and its services, the way Trident shows information about a
** connected USB device.
*/

#ifndef DEVWINCLASS_H
#define DEVWINCLASS_H

#include <exec/types.h>
#include <exec/lists.h>
#include <intuition/classusr.h>
#include <utility/hooks.h>

struct DevWinData
{
    Object        *nametxt, *addrtxt, *typetxt, *statetxt, *bindtxt, *keystxt;
    Object        *svclist;
    Object        *rescanbtn, *cfgbtn;   /* Configure: the selected service's binding settings window */
    /* per-device settings (the DEVC form: custom name, PoPo policy), as in
       Trident's device window */
    Object        *cwname, *setnamebtn, *resetnamebtn;
    Object        *chk_inhibitpopup, *chk_noclassbind, *chk_autoconnect, *chk_trusted;
    BOOL           loading;      /* filling the gadgets: ignore their notifications */
    struct MinList svcentries;   /* struct SvcEntry backing store */
    APTR           device;       /* the device being shown */
};

#define TAGBASE_DevWin (TAG_USER | 0x1c00)

#define MUIM_DevWin_Populate   (TAGBASE_DevWin | 0x01)
#define MUIM_DevWin_Rescan     (TAGBASE_DevWin | 0x02)
#define MUIM_DevWin_SettingChg (TAGBASE_DevWin | 0x04)   /* a policy checkmark changed */
#define MUIM_DevWin_SetName    (TAGBASE_DevWin | 0x05)   /* apply the custom name */
#define MUIM_DevWin_ResetName  (TAGBASE_DevWin | 0x06)   /* back to the name the device reports */
#define MUIM_DevWin_Configure  (TAGBASE_DevWin | 0x07)   /* settings window of the selected service's binding */
#define MUIM_DevWin_SvcActive  (TAGBASE_DevWin | 0x08)

/* MUIM_DevWin_Show: set the device, refresh and open the window */
#define MUIM_DevWin_Show     (TAGBASE_DevWin | 0x03)
struct MUIP_DevWin_Show { STACKED ULONG MethodID; STACKED APTR device; };

AROS_UFP3(IPTR, DevWinDispatcher,
          AROS_UFPA(struct IClass *, cl, A0),
          AROS_UFPA(Object *, obj, A2),
          AROS_UFPA(Msg, msg, A1));

#endif /* DEVWINCLASS_H */
