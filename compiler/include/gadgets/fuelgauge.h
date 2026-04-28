/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible gadgets/fuelgauge.h
*/

#ifndef GADGETS_FUELGAUGE_H
#define GADGETS_FUELGAUGE_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#define FUELGAUGE_CLASSNAME     "gadgets/fuelgauge.gadget"
#define FUELGAUGE_VERSION       44

#define FUELGAUGE_Dummy         (TAG_USER + 0x70000)

#define FUELGAUGE_Level         (FUELGAUGE_Dummy + 0x0001)
#define FUELGAUGE_Min           (FUELGAUGE_Dummy + 0x0002)
#define FUELGAUGE_Max           (FUELGAUGE_Dummy + 0x0003)
#define FUELGAUGE_Orientation   (FUELGAUGE_Dummy + 0x0004)
#define FUELGAUGE_Ticks         (FUELGAUGE_Dummy + 0x0005)
#define FUELGAUGE_ShortTicks    (FUELGAUGE_Dummy + 0x0006)
#define FUELGAUGE_Percent       (FUELGAUGE_Dummy + 0x0007)
#define FUELGAUGE_Justification (FUELGAUGE_Dummy + 0x0008)
#define FUELGAUGE_VarArgs       (FUELGAUGE_Dummy + 0x0009)
#define FUELGAUGE_DontFormat    (FUELGAUGE_Dummy + 0x000A)

#define FUELGAUGE_ORIENT_HORIZ  0
#define FUELGAUGE_ORIENT_VERT   1

#define FuelGaugeObject NewObject(NULL, FUELGAUGE_CLASSNAME
#define FuelGaugeEnd    TAG_END)
#define FuelObject      FuelGaugeObject

#endif /* GADGETS_FUELGAUGE_H */
