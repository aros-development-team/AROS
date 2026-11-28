#ifndef STEPSLIDER_PRIVATE_H
#define STEPSLIDER_PRIVATE_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <libraries/mui.h>

struct Stepslider_DATA
{
    struct MUI_EventHandlerNode ss_Ehn;
    BOOL ss_HandlerAdded;
    BOOL ss_Dragging;
};

#endif /* STEPSLIDER_PRIVATE_H */
