/*
** ScanWinClass - the "Add Device" window (a subclass of Window.mui), opened
** from the Devices page. It shows devices that have been discovered but are
** not yet connected/known, with Refresh and Connect buttons. Closing the
** window simply cancels. This mirrors Trident's DevWinClass split.
*/

#ifndef SCANWINCLASS_H
#define SCANWINCLASS_H

#include <exec/types.h>
#include <exec/lists.h>
#include <intuition/classusr.h>
#include <utility/hooks.h>

struct ScanWinData
{
    Object        *list;
    Object        *refreshbtn;
    Object        *connectbtn;
    struct Hook    dischook;
    struct MinList entries;      /* struct DevEntry backing store */
    APTR           radio;        /* radio used for discovery */
    BOOL           busy;         /* connect/pair in progress (they block) */
};

#define TAGBASE_ScanWin (TAG_USER | 0x1b00)

#define MUIM_ScanWin_Refresh  (TAGBASE_ScanWin | 0x01)  /* (re)start discovery */
#define MUIM_ScanWin_Connect  (TAGBASE_ScanWin | 0x02)  /* connect selected */
#define MUIM_ScanWin_Populate (TAGBASE_ScanWin | 0x03)  /* refill from library */

AROS_UFP3(IPTR, ScanWinDispatcher,
          AROS_UFPA(struct IClass *, cl, A0),
          AROS_UFPA(Object *, obj, A2),
          AROS_UFPA(Msg, msg, A1));

#endif /* SCANWINCLASS_H */
