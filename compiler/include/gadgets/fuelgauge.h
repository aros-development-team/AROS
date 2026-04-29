/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible gadgets/fuelgauge.h
*/

#ifndef GADGETS_FUELGAUGE_H
#define GADGETS_FUELGAUGE_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef REACTION_REACTION_H
#include <reaction/reaction.h>
#endif

#define FUELGAUGE_CLASSNAME     "fuelgauge.gadget"
#define FUELGAUGE_VERSION       44

#define FUELGAUGE_Dummy         (REACTION_Dummy + 0x12000)

#define FUELGAUGE_Level         (FUELGAUGE_Dummy + 0x0001) /* Current level */
#define FUELGAUGE_Min           (FUELGAUGE_Dummy + 0x0002) /* Minimum value */
#define FUELGAUGE_Max           (FUELGAUGE_Dummy + 0x0003) /* Maximum value */
#define FUELGAUGE_Orientation   (FUELGAUGE_Dummy + 0x0004) /* Horiz or vert */
#define FUELGAUGE_Ticks         (FUELGAUGE_Dummy + 0x0005) /* Major tick count */
#define FUELGAUGE_ShortTicks    (FUELGAUGE_Dummy + 0x0006) /* Minor tick count */
#define FUELGAUGE_Percent       (FUELGAUGE_Dummy + 0x0007) /* Show percentage */
#define FUELGAUGE_Justification (FUELGAUGE_Dummy + 0x0008) /* Text alignment */
#define FUELGAUGE_VarArgs       (FUELGAUGE_Dummy + 0x0009) /* Format string args */
#define FUELGAUGE_DontFormat    (FUELGAUGE_Dummy + 0x000A) /* Skip formatting */

#define FUELGAUGE_ORIENT_HORIZ  0
#define FUELGAUGE_ORIENT_VERT   1

#ifndef FuelGaugeObject
#define FuelGaugeObject NewObject(NULL, FUELGAUGE_CLASSNAME
#endif
#ifndef FuelGaugeEnd
#define FuelGaugeEnd    TAG_END)
#endif
#define FuelObject      FuelGaugeObject

#endif /* GADGETS_FUELGAUGE_H */
