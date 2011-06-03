#ifndef _KNOB_PRIVATE_H_
#define _KNOB_PRIVATE_H_

#include <libraries/mui.h>

struct Knob_DATA
{
    struct MUI_EventHandlerNode  ehn;
    DOUBLE prevangle;
};

#endif /* _KNOB_PRIVATE_H_ */
