#ifndef STEPPER_PRIVATE_H
#define STEPPER_PRIVATE_H

/*
    Copyright (C) 2026, Andy Rexx. All rights reserved.
*/

#include <libraries/mui.h>

struct Stepper_DATA
{
    struct MUI_EventHandlerNode st_Ehn;
    ULONG st_Flags;
    LONG st_Min;
    LONG st_Max;
    LONG st_Default;
    LONG st_Value;
    STRPTR st_Format;
    UBYTE st_PressedSide;
    char st_Stringify[50];
};

#endif /* STEPPER_PRIVATE_H */
