#ifndef HARDWARE_CIA_H
#define HARDWARE_CIA_H

/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Amiga CIA chips
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

struct CIA
{
    UBYTE ciapra;
    UBYTE ciapad0[255];
    UBYTE ciaprb;
    UBYTE ciapad1[255];
    UBYTE ciaddra;
    UBYTE ciapad2[255];
    UBYTE ciaddrb;
    UBYTE ciapad3[255];
    UBYTE ciatalo;
    UBYTE ciapad4[255];
    UBYTE ciatahi;
    UBYTE ciapad5[255];
    UBYTE ciatblo;
    UBYTE ciapad6[255];
    UBYTE ciatbhi;
    UBYTE ciapad7[255];
    UBYTE ciatodlow;
    UBYTE ciapad8[255];
    UBYTE ciatodmid;
    UBYTE ciapad9[255];
    UBYTE ciatodhi;
    UBYTE ciapad10[255];
    UBYTE unusedreg;
    UBYTE ciapad11[255];
    UBYTE ciasdr;
    UBYTE ciapad12[255];
    UBYTE ciaicr;
    UBYTE ciapad13[255];
    UBYTE ciacra;
    UBYTE ciapad14[255];
    UBYTE ciacrb;
};

/* Used in rom/exec/execstrap_init.c */
/* Still has to be completed */
#define CIAF_GAMEPORT0                  (1L<<6)

/* iCR bit definitions */
#define CIAICRB_TA                      0
#define CIAICRB_TB                      1

/* iCR bit definitions for Timer A */
#define CIACRAB_START	                0
#define CIACRAB_PBON	                1
#define CIACRAB_OUTMODE                 2
#define CIACRAB_SPMODE	                6
#define CIACRAB_TODIN	                7

/* iCR Flags for Timer A */
#define CIACRAF_START	                (1 << CIACRAB_START)
#define CIACRAF_PBON	                (1 << CIACRAB_PBON)
#define CIACRAF_OUTMODE                 (1 << CIACRAB_OUTMODE)
#define CIACRAF_SPMODE	                (1 << CIACRAB_SPMODE)
#define CIACRAF_TODIN	                (1 << CIACRAB_TODIN)

/* iCR bit definitions for Timer B */
#define CIACRBB_START	                0
#define CIACRBB_PBON	                1
#define CIACRBB_OUTMODE                 2
#define CIACRBB_ALARM	                7

/* iCR Flags for Timer B */
#define CIACRBF_START	                (1 << CIACRBB_START)
#define CIACRBF_PBON	                (1 << CIACRBB_PBON)
#define CIACRBF_OUTMODE                 (1 << CIACRBB_OUTMODE)
#define CIACRBF_ALARM	                (1 << CIACRBB_ALARM)

#endif /* HARDWARE_CIA_H */
