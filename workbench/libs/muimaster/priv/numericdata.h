#ifndef __NUMERICDATA_H__
#define __NUMERICDATA_H__

#include <eventhandler.h>

struct MUI_NumericData
{
    STRPTR format;
    LONG   defvalue;
    LONG   max;
    LONG   min;
    LONG   value;
    ULONG  flags;
    struct MUI_EventHandlerNode ehn;
    struct MUI_EventHandlerNode ccn;
};

enum flags {
    NUMERIC_REVERSE = (1<<0),
    NUMERIC_REVLEFTRIGHT = (1<<1),
    NUMERIC_REVUPDOWN = (1<<2),
    NUMERIC_CHECKALLSIZES = (1<<3),
};

#endif
