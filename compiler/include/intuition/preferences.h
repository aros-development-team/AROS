#ifndef INTUITION_PREFERENCES_H
#define INTUITION_PREFERENCES_H

/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$

    Old-style preferences structures and defines.
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef DEVICES_TIMER_H
#   include <devices/timer.h>
#endif

/* Printer configurations */
#define	FILENAME_SIZE	30
#define DEVNAME_SIZE	16

/* Size of Pointer data buffer */
#define	POINTERSIZE ((1+16+1)*2)


/* Default font sizes */
/* (Height of default font topaz in 60/80 column mode */
#define TOPAZ_EIGHTY 8
#define TOPAZ_SIXTY 9

/* NOTE: From V36 on fields of struct Preferences are ignored by SetPrefs().
 * Generally elements are added or replaced as new hard- and software arise.
 * Some fields can only be set by initial SetPrefs() using
 * DEVS:system-configuration. Some data must be truncated so applications
 * calling GetPrefs() modifying parts of the struct References and calling
 * SetPrefs() do not kill data.
 * See GetPrefs() and SetPrefs() for further information.
 */

struct Preferences
{
    BYTE FontHeight;
    UBYTE PrinterPort;	/* Printer port connection */
    UWORD BaudRate;	/* Baud rate for the serial port */

    /* Timing rates */
    struct timeval KeyRptSpeed; /* Repeat speed for keyboard */
    struct timeval KeyRptDelay; /* Delay before keys repeat */
    struct timeval DoubleClick; /* Interval allowed between clicks */

    /* Intuition Mouse-Pointer data */
    UWORD PointerMatrix[POINTERSIZE];	/* Definition of pointer sprite */
    BYTE  XOffset;			/* X-Offset for hot spot        */
    BYTE  YOffset;			/* Y-Offset for hot spot        */
    UWORD color17;			/* Colors for pointer sprite    */
    UWORD color18;			/*   "     "     "      "       */
    UWORD color19;			/*   "     "     "      "       */
    UWORD PointerTicks;			/* Sensitivity of the pointer   */

    /* Standard 4 Workbench-Screen colors */
    UWORD color0;
    UWORD color1;
    UWORD color2;
    UWORD color3;

    /* Positioning data for the Intuition View */
    BYTE ViewXOffset; /* Offset for top-left corner */
    BYTE ViewYOffset;
    WORD ViewInitX;   /* Initial offset values of View */
    WORD ViewInitY;

    BOOL EnableCLI; /* CLI availability switch */

    /* Printer configurations */
    UWORD PrinterType;			  /* printer type */
    UBYTE PrinterFilename[FILENAME_SIZE]; /* file for printer */

    /* Print format and quality configurations */
    UWORD PrintPitch;	    /* Print pitch		      */
    UWORD PrintQuality;	    /* Print quality		      */
    UWORD PrintSpacing;	    /* Number of lines per inch	      */
    UWORD PrintLeftMargin;  /* Left margin in characters      */
    UWORD PrintRightMargin; /* Right margin in characters     */
    UWORD PrintImage;	    /* Positive or negative	      */
    UWORD PrintAspect;	    /* Horizontal or vertical	      */
    UWORD PrintShade;	    /* B&W, half-tone, or color	      */
    WORD  PrintThreshold;   /* Darkness control for b/w dumps */

    /* Print-paper description */
    UWORD PaperSize;	/* Paper size			   */
    UWORD PaperLength;	/* Paper length in number of lines */
    UWORD PaperType;	/* Continuous or single sheet	   */

    /* Serial device settings:
     * six nibble-fields in three bytes
     */
    UBYTE SerRWBits;	/* Upper nibble = (8-number of read bits)    */
			/* Lower nibble = (8-number of write bits)   */
    UBYTE SerStopBuf;	/* Upper nibble = (number of stop bits - 1)  */
			/* Lower nibble = (table value for BufSize)  */
    UBYTE SerParShk;	/* Upper nibble = (value for Parity setting) */
			/* Lower nibble = (value for Handshake mode) */
    UBYTE LaceWB;	/* If workbench is to be interlaced	     */

    UBYTE Pad[12];
    UBYTE PrtDevName[DEVNAME_SIZE]; /* Device used by printer.device
				     * (leave out the ".device")
				     */
    UBYTE DefaultPrtUnit; /* Default unit opened by printer.device */
    UBYTE DefaultSerUnit; /* Default serial unit */

    BYTE RowSizeChange;    /* Affect NormalDisplayRows/Columns */
    BYTE ColumnSizeChange;

    UWORD PrintFlags;	  /* User preference flags */
    UWORD PrintMaxWidth;  /* Max width  of printed picture in 0.1 inch */
    UWORD PrintMaxHeight; /* Max height of printed picture in 0.1 inch */
    UBYTE PrintDensity;   /* Print density */
    UBYTE PrintXOffset;   /* Offset of printed picture in 0.1 inch */

    UWORD wb_Width;  /* Override default Workbench width  */
    UWORD wb_Height; /* Override default Workbench height */
    UBYTE wb_Depth;  /* Override default Workbench depth  */

    UBYTE ext_size; /* Internal value 'extension information'
		     * = extension size in blocks of 64 bytes
		     * DO NOT TOUCH !!!
		     */
};


/* Workbench Interlace (uses one bit) */
#define LACEWB		(1<<0)
#define LW_RESERVED	1	/* internal use only */

#define SCREEN_DRAG	(1<<14)
#define MOUSE_ACCEL	(1L<<15)

/* PrinterPort */
#define PARALLEL_PRINTER 0x00
#define SERIAL_PRINTER	 0x01

/* BaudRates */
#define BAUD_110	0x00
#define BAUD_300	0x01
#define BAUD_1200	0x02
#define BAUD_2400	0x03
#define BAUD_4800	0x04
#define BAUD_9600	0x05
#define BAUD_19200	0x06
#define BAUD_MIDI	0x07

/* PaperType */
#define FANFOLD		0x00
#define SINGLE		0x80

/* PrintPitch */
#define PICA		0x000
#define ELITE		0x400
#define FINE		0x800

/* PrintQuality */
#define DRAFT		0x000
#define LETTER		0x100

/* PrintSpacing */
#define SIX_LPI		0x000
#define EIGHT_LPI	0x200

/* Print Image */
#define IMAGE_POSITIVE	0x00
#define IMAGE_NEGATIVE	0x01

/* PrintAspect */
#define ASPECT_HORIZ	0x00
#define ASPECT_VERT	0x01

/* PrintShade */
#define SHADE_BW	0x00
#define SHADE_GREYSCALE	0x01
#define SHADE_COLOR	0x02

/* PaperSize (All paper sizes must have a zero in the lowest nibble) */
#define US_LETTER	0x00
#define US_LEGAL	0x10
#define N_TRACTOR	0x20
#define W_TRACTOR	0x30
#define CUSTOM		0x40

/* European sizes */
#define EURO_A0		0x50	/* A0: 841 x 1189 */
#define EURO_A1		0x60	/* A1: 594 x 841  */
#define EURO_A2		0x70	/* A2: 420 x 594  */
#define EURO_A3		0x80	/* A3: 297 x 420  */
#define EURO_A4		0x90	/* A4: 210 x 297  */
#define EURO_A5		0xA0	/* A5: 148 x 210  */
#define EURO_A6		0xB0	/* A6: 105 x 148  */
#define EURO_A7		0xC0	/* A7: 74 x 105   */
#define EURO_A8		0xD0	/* A8: 52 x 74    */

/* PrinterType */
#define CUSTOM_NAME	0x00
#define	ALPHA_P_101	0x01
#define BROTHER_15XL	0x02
#define CBM_MPS1000	0x03
#define DIAB_630	0x04
#define DIAB_ADV_D25	0x05
#define DIAB_C_150	0x06
#define EPSON		0x07
#define EPSON_JX_80	0x08
#define OKIMATE_20	0x09
#define QUME_LP_20	0x0A
#define HP_LASERJET	 0x0B
#define HP_LASERJET_PLUS 0x0C

/* Serial Input Buffer Sizes */
#define SBUF_512	0x00
#define SBUF_1024	0x01
#define SBUF_2048	0x02
#define SBUF_4096	0x03
#define SBUF_8000	0x04
#define SBUF_16000	0x05

/* Serial Bit Masks */
#define	SREAD_BITS	0xF0 /* SerRWBits */
#define	SWRITE_BITS	0x0F

#define	SSTOP_BITS	0xF0 /* SerStopBuf */
#define	SBUFSIZE_BITS	0x0F

#define	SPARITY_BITS	0xF0 /* SerParShk */
#define SHSHAKE_BITS	0x0F

/* Serial Parity
 * (upper nibble, after being shifted by macro SPARNUM() )
 */
#define SPARITY_NONE	0
#define SPARITY_EVEN	1
#define SPARITY_ODD	2
#define SPARITY_MARK	3
#define SPARITY_SPACE	4

/* Serial Handshake Mode
 * (lower nibble, after masking using macro SHANKNUM() )
 */
#define SHSHAKE_XON	0
#define SHSHAKE_RTS	1
#define SHSHAKE_NONE	2


/* New defines for PrintFlags */

#define CORRECT_RED	    0x0001 /* Color correct red shades */
#define CORRECT_GREEN	    0x0002 /* Color correct green shades */
#define CORRECT_BLUE	    0x0004 /* Color correct blue shades */

#define CENTER_IMAGE	    0x0008 /* Center image on paper */

#define IGNORE_DIMENSIONS   0x0000 /* Ignore max width/height settings */
#define BOUNDED_DIMENSIONS  0x0010 /* Use max width/height as boundaries */
#define ABSOLUTE_DIMENSIONS 0x0020 /* Use max width/height as absolutes */
#define PIXEL_DIMENSIONS    0x0040 /* Use max width/height as printer pixels */
#define MULTIPLY_DIMENSIONS 0x0080 /* Use max width/height as multipliers */

#define INTEGER_SCALING     0x0100 /* Force integer scaling */

#define ORDERED_DITHERING   0x0000 /* Ordered dithering */
#define HALFTONE_DITHERING  0x0200 /* Halftone dithering */
#define FLOYD_DITHERING     0x0400 /* Floyd-Steinberg dithering */

#define ANTI_ALIAS	    0x0800 /* Anti-alias image */
#define GREY_SCALE2	    0x1000 /* For use with HI-Res monitor */

/* masks used for checking bits */
#define CORRECT_RGB_MASK    (CORRECT_RED|CORRECT_GREEN|CORRECT_BLUE)
#define DIMENSIONS_MASK	    (BOUNDED_DIMENSIONS|ABSOLUTE_DIMENSIONS|PIXEL_DIMENSIONS|MULTIPLY_DIMENSIONS)
#define DITHERING_MASK	    (HALFTONE_DITHERING|FLOYD_DITHERING)

#endif /* INTUITION_PREFERENCES_H */
