/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc: m68k arch specific private data for lowlevel library
    Lang: english
*/

struct llCIATimer
{
    struct Library              *llciat_Base;
    struct Interrupt            llciat_Int;
    WORD                        llciat_iCRBit;
};

struct llArchData
{
    /*
     * Variables used by amiga-m68k
     */
    ULONG                       llad_PortType[2];
    struct Library              *llad_PotgoBase;
    struct llCIATimer           llad_CIA;
    WORD                        llad_EClockMult;
};
