#ifndef CLIB_MUI_PROTOS_H
#define CLIB_MUI_PROTOS_H

/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
*/

#include <aros/libcall.h>
AROS_LP8(LONG, MiamiPanelInit,
         AROS_LPA(IPTR, synccb, A0),
         AROS_LPA(IPTR, asynccb, A1),
         AROS_LPA(LONG, flags, D0),
         AROS_LPA(STRPTR, font, A2),
         AROS_LPA(STRPTR, screen, A3),
         AROS_LPA(LONG, xo, D1),
         AROS_LPA(LONG, yo, D2),
         AROS_LPA(IPTR, sigbit, A4),
         LIBBASETYPEPTR, MiamiPanelBase, 5, MiamiPanel
);
AROS_LP0(void, MiamiPanelCleanup,
         LIBBASETYPEPTR, MiamiPanelBase, 6, MiamiPanel
);
AROS_LP5(void, MiamiPanelAddInterface,
         AROS_LPA(LONG, unit, D0),
         AROS_LPA(STRPTR, name, A0),
         AROS_LPA(LONG, state, D1),
         AROS_LPA(LONG, ontime, D2),
         AROS_LPA(STRPTR, speed, A1),
         LIBBASETYPEPTR, MiamiPanelBase, 7, MiamiPanel
);
AROS_LP1(void, MiamiPanelDelInterface,
         AROS_LPA(LONG, unit, D0),
         LIBBASETYPEPTR, MiamiPanelBase, 8, MiamiPanel
);
AROS_LP3(void, MiamiPanelSetInterfaceState,
         AROS_LPA(LONG, unit, D0),
         AROS_LPA(LONG, state, D1),
         AROS_LPA(LONG, ontime, D2),
         LIBBASETYPEPTR, MiamiPanelBase, 9, MiamiPanel
);
AROS_LP2(void, MiamiPanelSetInterfaceSpeed,
         AROS_LPA(LONG, unit, D0),
         AROS_LPA(UBYTE *, speed, A0),
         LIBBASETYPEPTR, MiamiPanelBase, 10, MiamiPanel
);
AROS_LP5(void, MiamiPanelInterfaceReport,
         AROS_LPA(LONG, unit, D0),
         AROS_LPA(LONG, rate, D1),
         AROS_LPA(LONG, now, D2),
         AROS_LPA(LONG, totalhi, D3),
         AROS_LPA(ULONG, totallo, D4),
         LIBBASETYPEPTR, MiamiPanelBase, 11, MiamiPanel
);
AROS_LP0(void, MiamiPanelToFront,
         LIBBASETYPEPTR, MiamiPanelBase, 12, MiamiPanel
);
AROS_LP1(void, MiamiPanelInhibitRefresh,
         AROS_LPA(LONG, val, D0),
         LIBBASETYPEPTR, MiamiPanelBase, 13, MiamiPanel
);
AROS_LP2(void, MiamiPanelGetCoord,
         AROS_LPA(LONG *, xp, A0),
         AROS_LPA(LONG *, yp, A1),
         LIBBASETYPEPTR, MiamiPanelBase, 14, MiamiPanel
);
AROS_LP1(void, MiamiPanelEvent,
         AROS_LPA(ULONG, sigs, D0),
         LIBBASETYPEPTR, MiamiPanelBase, 15, MiamiPanel
);
AROS_LP2(void, MiamiPanelRefreshName,
         AROS_LPA(LONG, unit, D0),
         AROS_LPA(UBYTE *, name, A0),
         LIBBASETYPEPTR, MiamiPanelBase, 16, MiamiPanel
);
AROS_LP0(LONG, MiamiPanelGetVersion,
         LIBBASETYPEPTR, MiamiPanelBase, 17, MiamiPanel
);
AROS_LP0(ULONG, MiamiPanelKill,
         LIBBASETYPEPTR, MiamiPanelBase, 26, MiamiPanel
);

#endif /* CLIB_MUI_PROTOS_H */
