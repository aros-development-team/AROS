#ifndef DEVICES_PRTBASE_H
#define DEVICES_PRTBASE_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: printer driver structures and tags
    Lang: english
*/


#ifndef UTILITY_TAGITEM_H
#   include <utility/tagitem.h>
#endif

#ifndef  EXEC_DEVICES_H
#   include <exec/devices.h>
#endif

#ifndef  DEVICES_PARALLEL_H
#   include <devices/parallel.h>
#endif

#ifndef  DEVICES_SERIAL_H
#   include <devices/serial.h>
#endif

#ifndef  DEVICES_TIMER_H
#   include <devices/timer.h>
#endif

#ifndef  DOS_DOSEXTENS_H
#   include <dos/dosextens.h>
#endif

#ifndef  INTUITION_INTUITION_H
#   include <intuition/intuition.h>
#endif

struct DeviceData
{
    struct Library	dd_Device;
    APTR		dd_Segment;
    APTR		dd_ExecBase;
    APTR		dd_CmdVectors;
    APTR		dd_CmdBytes;
    UWORD		dd_NumCommands;
};

/* IO Flags */

#define IOB_QUEUED	    	4
#define IOB_CURRENT	    	5
#define IOB_SERVICING	    	6
#define IOB_DONE	    	7

#define IOF_QUEUED	    	(1L << IOB_QUEUED)
#define IOF_CURRENT	    	(1L << IOB_CURRENT)
#define IOF_SERVICING	    	(1L << IOB_SERVICING)
#define IOF_DONE	    	(1L << IOB_DONE)

/* pd_Flags */

#define PB_IOR0		    	0
#define PB_IOR1		    	1
#define PB_IOOPENED	    	2
#define PB_EXPUNGED	    	7

#define PBF_IOR0	    	(1L << PB_IOR0)
#define PBF_IOR1	    	(1L << PB_IOR1)
#define PBF_IOOPENDED	    	(1L << PB_IOOPENED)
#define PBF_EXPUNGED	    	(1L << PB_EXPUNGED)

/* du_Flags (actually placed in pd_Unit.mp_Node.ln_Pri) */

#define DUB_STOPPED	    	0

#define DUF_STOPPED	    	(1L << DUB_STOPPED)

#define P_OLDSTKSIZE	    	0x0800  /* Unused */
#define P_STKSIZE   	    	0x1000  /* Unused */
#define P_BUFSIZE	    	256
#define P_SAFESIZE	    	128

union printerIO;

struct PrinterData
{
    struct DeviceData 	      pd_Device;
    struct MsgPort  	      pd_Unit;
    BPTR    	    	      pd_PrinterSegment;
    UWORD   	    	      pd_PrinterType;
    struct PrinterSegment    *pd_SegmentData;
    UBYTE   	    	     *pd_PrintBuf;
    LONG    	    	    (*pd_PWrite)(APTR data, LONG len);
    LONG    	    	    (*pd_PBothReady)(VOID);
    union
    {
	struct IOExtPar pd_p0;
	struct IOExtSer pd_s0;
    } 	    	    	      pd_ior0;
    union
    {
	struct IOExtPar pd_p1;
	struct IOExtSer pd_s1;
    } 	    	    	      pd_ior1;
    struct timerequest        pd_TIOR;
    struct MsgPort  	      pd_IORPort;
    struct Task     	      pd_TC;
    UBYTE   	    	      pd_OldStk[P_OLDSTKSIZE];
    UBYTE   	    	      pd_Flags;
    UBYTE   	    	      pd_pad;
    struct Preferences        pd_Preferences;
    UBYTE   	    	      pd_PWaitEnabled;
    UBYTE   	    	      pd_Flags1;
    UBYTE   	    	      pd_Stk[P_STKSIZE];
    struct PrinterUnit       *pd_PUnit;
    LONG    	    	    (*pd_PRead)(char * buffer, LONG *length, struct timeval *tv);
    LONG    	    	    (*pd_CallErrHook)(struct Hook *hook, union printerIO *ior, struct PrtErrMsg *pem);
    ULONG   	    	      pd_UnitNumber;
    STRPTR  	    	      pd_DriverName;
    LONG    	    	    (*pd_PQuery)(LONG *numofchars);
};

#define pd_PIOR0    	    	pd_ior0.pd_p0
#define pd_SIOR0    	    	pd_ior0.pd_s0

#define pd_PIOR1    	    	pd_ior1.pd_p1
#define pd_SIOR1    	    	pd_ior1.pd_s1

/* Printer Class */

#define PPCB_GFX	    	0
#define PPCF_GFX	    	0x1
#define PPCB_COLOR	    	1
#define PPCF_COLOR	    	0x2

#define PPC_BWALPHA	    	0x00
#define PPC_BWGFX	    	0x01
#define PPC_COLORALPHA	    	0x02
#define PPC_COLORGFX	    	0x03

#define PPCB_EXTENDED	    	2
#define PPCF_EXTENDED	    	0x4

#define PPCB_NOSTRIP	    	3
#define PPCF_NOSTRIP	    	0x8

/* Color Class */

#define	PCC_BW		    	0x01
#define	PCC_YMC		    	0x02
#define	PCC_YMC_BW	    	0x03
#define	PCC_YMCB	    	0x04
#define	PCC_4COLOR	    	0x04
#define	PCC_ADDITIVE	    	0x08
#define	PCC_WB		    	0x09
#define	PCC_BGR		    	0x0A
#define	PCC_BGR_WB	    	0x0B
#define	PCC_BGRW	    	0x0C
#define PCC_MULTI_PASS	    	0x10

struct PrinterExtendedData
{
    char    	     *ped_PrinterName;
    LONG    	    (*ped_Init)(struct PrinterData *pd); /* return 0 for success */
    VOID    	    (*ped_Expunge)(VOID);
    LONG    	    (*ped_Open)(union printerIO *ior);   /* return 0 for success */
    VOID    	    (*ped_Close)(union printerIO *ior);
    UBYTE   	      ped_PrinterClass;
    UBYTE   	      ped_ColorClass;
    UBYTE   	      ped_MaxColumns;
    UBYTE   	      ped_NumCharSets;
    UWORD   	      ped_NumRows;
    ULONG   	      ped_MaxXDots;
    ULONG   	      ped_MaxYDots;
    UWORD   	      ped_XDotsInch;
    UWORD   	      ped_YDotsInch;
    STRPTR  	     *ped_Commands;
    LONG    	    (*ped_DoSpecial)(UWORD *command,
    	    	    		     UBYTE output_buffer[],
				     BYTE *current_line_position,
				     BYTE *current_line_spacing,
				     BYTE *crlf_flag,
				     UBYTE params[]);
    LONG    	    (*ped_Render)(SIPTR ct, LONG x, LONG y, LONG status);
    LONG    	      ped_TimeoutSecs;
    /* Version 33 and above drivers */
    STRPTR  	     *ped_8BitChars;
    LONG    	      ped_PrintMode;
    /* Version 34 and above drivers */
    LONG    	    (*ped_ConvFunc)(UBYTE *buf, UBYTE c, LONG crlf_flag);
    /* Version 44 and above drivers, with PPCF_EXTENDED */
    struct TagItem   *ped_TagList;
    LONG    	    (*ped_DoPreferences)(union printerIO *ior, LONG command);
    VOID    	    (*ped_CallErrHook)(union printerIO *ior, struct Hook *hook);
};

/* Tags to define more printer driver features */

#define PRTA_Dummy  	    	(TAG_USER + 0x50000)
#define PRTA_8BitGuns		(PRTA_Dummy + 1)
#define PRTA_ConvertSource	(PRTA_Dummy + 2)
#define PRTA_FloydDithering	(PRTA_Dummy + 3)
#define PRTA_AntiAlias		(PRTA_Dummy + 4)
#define PRTA_ColorCorrection	(PRTA_Dummy + 5)
#define PRTA_NoIO		(PRTA_Dummy + 6)
#define PRTA_NewColor		(PRTA_Dummy + 7)
#define PRTA_ColorSize		(PRTA_Dummy + 8)
#define PRTA_NoScaling		(PRTA_Dummy + 9)

/* User interface */
#define PRTA_DitherNames	(PRTA_Dummy + 20)
#define PRTA_ShadingNames	(PRTA_Dummy + 21)
#define PRTA_ColorCorrect	(PRTA_Dummy + 22)
#define PRTA_DensityInfo	(PRTA_Dummy + 23)

/* Hardware page borders */
#define PRTA_LeftBorder		(PRTA_Dummy + 30)
#define PRTA_TopBorder		(PRTA_Dummy + 31)

#define PRTA_MixBWColor		(PRTA_Dummy + 32)

/* Driver Preferences */
#define PRTA_Preferences	(PRTA_Dummy + 40)

/****************************************************************************/

struct PrinterSegment
{
    BPTR			ps_NextSegment;
    ULONG			ps_runAlert;
    UWORD			ps_Version;
    UWORD			ps_Revision;
    struct PrinterExtendedData	ps_PED;	
};

/****************************************************************************/

struct PrtDriverPreferences
{
    UWORD    pdp_Version;
    UBYTE    pdp_PrinterID[32];
    char     pdp_PrefName[FILENAME_SIZE-16];
    ULONG    pdp_Length;    	/* size of this structure */

    /* .. more driver private fields following .. */
};

#endif /* DEVICES_PRTBASE_H */
