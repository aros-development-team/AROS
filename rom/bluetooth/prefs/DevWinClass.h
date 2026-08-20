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
    Object        *nametxt, *addrtxt, *typetxt, *statetxt, *bindtxt;
    Object        *svclist;
    Object        *rescanbtn;
    struct MinList svcentries;   /* struct SvcEntry backing store */
    APTR           device;       /* the device being shown */
};

#define TAGBASE_DevWin (TAG_USER | 0x1c00)

#define MUIM_DevWin_Populate (TAGBASE_DevWin | 0x01)
#define MUIM_DevWin_Rescan   (TAGBASE_DevWin | 0x02)

/* MUIM_DevWin_Show: set the device, refresh and open the window */
#define MUIM_DevWin_Show     (TAGBASE_DevWin | 0x03)
struct MUIP_DevWin_Show { STACKED ULONG MethodID; STACKED APTR device; };

AROS_UFP3(IPTR, DevWinDispatcher,
          AROS_UFPA(struct IClass *, cl, A0),
          AROS_UFPA(Object *, obj, A2),
          AROS_UFPA(Msg, msg, A1));

#endif /* DEVWINCLASS_H */
