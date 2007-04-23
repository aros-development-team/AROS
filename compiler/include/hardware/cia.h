#ifndef HARDWARE_CIA_H
#define HARDWARE_CIA_H

/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
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
#define CIAF_GAMEPORT0	(1L<<6)


#endif /* HARDWARE_CIA_H */
