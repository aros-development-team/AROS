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

#define FUELGAUGE_Min           (FUELGAUGE_Dummy + 1)  /* (LONG) Minimum value */
#define FUELGAUGE_Max           (FUELGAUGE_Dummy + 2)  /* (LONG) Maximum value */
#define FUELGAUGE_Level         (FUELGAUGE_Dummy + 3)  /* (LONG) Current level */
#define FUELGAUGE_Orientation   (FUELGAUGE_Dummy + 4)  /* (WORD) Layout orientation */
#define FUELGAUGE_Percent       (FUELGAUGE_Dummy + 5)  /* (BOOL) Show percentage text */
#define FUELGAUGE_Ticks         (FUELGAUGE_Dummy + 6)  /* (WORD) Major tick count */
#define FUELGAUGE_ShortTicks    (FUELGAUGE_Dummy + 7)  /* (WORD) Minor tick count */
#define FUELGAUGE_TickSize      (FUELGAUGE_Dummy + 8)  /* (WORD) Major tick height */
#define FUELGAUGE_TickPen       (FUELGAUGE_Dummy + 9)  /* (WORD) Tick mark pen */
#define FUELGAUGE_PercentPen    (FUELGAUGE_Dummy + 10) /* (WORD) Percentage text pen */
#define FUELGAUGE_FillPen       (FUELGAUGE_Dummy + 11) /* (WORD) Fuel bar pen */
#define FUELGAUGE_EmptyPen      (FUELGAUGE_Dummy + 12) /* (WORD) Background/empty pen */
#define FUELGAUGE_VarArgs       (FUELGAUGE_Dummy + 13) /* GA_Text varargs array */
#define FUELGAUGE_Justification (FUELGAUGE_Dummy + 14) /* GA_Text justification */

/* FUELGAUGE_Orientation modes */
#define FGORIENT_HORIZ  0
#define FGORIENT_VERT   1

/* FUELGAUGE_Justification modes */
#define FGJ_LEFT    0
#define FGJ_CENTER  1
#define FGJ_CENTRE  FGJ_CENTER

/* Obsolete compatibility defines */
#define FUELGAUGE_HORIZONTAL    FGORIENT_HORIZ
#define FUELGAUGE_VERTICAL      FGORIENT_VERT

#ifndef FuelGaugeObject
#define FuelGaugeObject NewObject(NULL, FUELGAUGE_CLASSNAME
#endif
#ifndef FuelGaugeEnd
#define FuelGaugeEnd    TAG_END)
#endif
#define FuelObject      FuelGaugeObject

#endif /* GADGETS_FUELGAUGE_H */
