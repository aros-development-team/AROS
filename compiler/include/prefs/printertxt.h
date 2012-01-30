/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef PREFS_PRINTERTXT_H
#define PREFS_PRINTERTXT_H


#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

#ifndef LIBRARIES_IFFPARSE_H
#   include <libraries/iffparse.h>
#endif


#define ID_PTXT MAKE_ID('P','T','X','T')
#define ID_PUNT MAKE_ID('P','U','N','T')
#define ID_PDEV MAKE_ID('P','D','E','V')
#define ID_PGFX MAKE_ID('P','G','F','X')


#define	DRIVERNAMESIZE 30
#define DEVICENAMESIZE 32
#define UNITNAMESIZE   32


struct PrinterTxtPrefs
{
    LONG  pt_Reserved[4];
    UBYTE pt_Driver[DRIVERNAMESIZE];
    UBYTE pt_Port;

    UWORD pt_PaperType;
    UWORD pt_PaperSize;
    UWORD pt_PaperLength;       /* # of lines per page */

    UWORD pt_Pitch;
    UWORD pt_Spacing;
    UWORD pt_LeftMargin;
    UWORD pt_RightMargin;
    UWORD pt_Quality;
};

#define PP_PARALLEL 0
#define PP_SERIAL   1

#define PT_FANFOLD  0
#define PT_SINGLE   1

#define PS_US_LETTER	0
#define PS_US_LEGAL	1
#define PS_N_TRACTOR	2
#define PS_W_TRACTOR	3
#define PS_CUSTOM	4
#define PS_EURO_A0	5
#define PS_EURO_A1	6
#define PS_EURO_A2	7
#define PS_EURO_A3	8
#define PS_EURO_A4	9
#define PS_EURO_A5	10
#define PS_EURO_A6	11
#define PS_EURO_A7	12
#define PS_EURO_A8	13

#define PP_PICA	 0      /* 10 characters per inch */
#define PP_ELITE 1      /* 12 characters per inch */
#define PP_FINE	 2      /* 17.1 characters per inch */

#define PS_SIX_LPI   0  /* 6 lines per inch */
#define PS_EIGHT_LPI 1  /* 8 lines per inch */

#define PQ_DRAFT  0     /* Density select 0 */
#define PQ_LETTER 1     /* Density select 1 */


struct PrinterUnitPrefs
{
    LONG  pu_Reserved[4];
    LONG  pu_UnitNum;
    ULONG pu_OpenDeviceFlags;
    UBYTE pu_DeviceName[DEVICENAMESIZE];
};

struct PrinterDeviceUnitPrefs
{
    LONG  pd_Reserved[4];               /* Reserved */
    LONG  pd_UnitNum;                   /* Unit number */
    UBYTE pd_UnitName[UNITNAMESIZE];    /* Name of the unit */
};

#endif /* PREFS_PRINTERTXT_H */
