#ifndef SLIDESWITCH_PRIVATE_H
#define SLIDESWITCH_PRIVATE_H

/*
    Copyright (C) 2026, Andy Rexx. All rights reserved.
*/

#include <libraries/mui.h>

struct SlideSwitch_DATA
{
    struct MUI_EventHandlerNode ss_Ehn;
    BOOL ss_Dragging;
};

#endif /* SLIDESWITCH_PRIVATE_H */
