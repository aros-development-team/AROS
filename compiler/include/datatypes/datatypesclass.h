#ifndef DATATYPES_DATATYPESCLASS_H
#define DATATYPES_DATATYPESCLASS_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: English
*/

#ifndef	UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif
#ifndef	DATATYPES_DATATYPES_H
#include <datatypes/datatypes.h>
#endif
#ifndef	INTUITION_INTUITION_H
#include <intuition/intuition.h>
#endif
#ifndef INTUITION_IMAGECLASS_H
#include <intuition/imageclass.h>
#endif

#ifndef	DEVICES_PRINTER_H
#include <devices/printer.h>
#endif
#ifndef	DEVICES_PRTBASE_H
#include <devices/prtbase.h>
#endif

#define  DATATYPESCLASS         "datatypesclass"

#define  DTA_Dummy              (TAG_USER + 0x1000)

/* Default TextAttr to use for text within the object (struct TextAttr *) */
#define  DTA_TextAttr           (DTA_Dummy+10)

/* Top vertical unit (LONG) */
#define  DTA_TopVert            (DTA_Dummy + 11)

/* Number of visible vertical units (LONG) */
#define  DTA_VisibleVert        (DTA_Dummy + 12)

/* Total number of vertical units */
#define  DTA_TotalVert          (DTA_Dummy + 13)

/* Number of pixels per vertical unit (LONG) */
#define  DTA_VertUnit           (DTA_Dummy + 14)

/* Top horizontal unit (LONG) */
#define  DTA_TopHoriz           (DTA_Dummy + 15)

/* Number of visible horizontal units (LONG) */
#define  DTA_VisibleHoriz       (DTA_Dummy + 16)

/* Total number of horiziontal units */
#define  DTA_TotalHoriz         (DTA_Dummy + 17)

/* Number of pixels per horizontal unit (LONG) */
#define  DTA_HorizUnit          (DTA_Dummy + 18)

/* Name of the current element within the object (UBYTE *) */
#define  DTA_NodeName           (DTA_Dummy + 19)

/* Object's title */
#define  DTA_Title              (DTA_Dummy + 20)

/* Pointer to a NULL terminated array of trigger methods (struct DTMethod *) */
#define  DTA_TriggerMethods     (DTA_Dummy + 21)

/* Object data (APTR) */
#define  DTA_Data               (DTA_Dummy + 22)

/* Default font to use (struct TextFont *) */
#define  DTA_TextFont           (DTA_Dummy + 23)

/* Pointer to an array (terminated with ~0) of methods that the object
   supports (ULONG *) */
#define  DTA_Methods            (DTA_Dummy + 24)

/* Printer error message -- numbers are defined in <devices/printer.h>
   (LONG) */
#define  DTA_PrinterStatus      (DTA_Dummy + 25)

/* PRIVATE! Pointer to the print process (struct Process *) */
#define  DTA_PrinterProc        (DTA_Dummy + 26)

/* PRIVATE! Pointer to the layout process (struct Process *) */
#define  DTA_LayoutProc         (DTA_Dummy + 27)

/* Turns the application's busy pointer on and off */
#define  DTA_Busy               (DTA_Dummy + 28)

/* Indicate that new information has been loaded into an object.
   (This is used for models that cache the DTA_TopVert-like tags.) */
#define  DTA_Sync               (DTA_Dummy + 29)

/* Base name of the class */
#define  DTA_BaseName           (DTA_Dummy + 30)

/* Group that the object must belong to */
#define  DTA_GroupID            (DTA_Dummy + 31)

/* Error level */
#define  DTA_ErrorLevel         (DTA_Dummy + 32)

/* datatypes.library error number */
#define  DTA_ErrorNumber        (DTA_Dummy + 33)

/* Argument for datatypes.library error */
#define  DTA_ErrorString        (DTA_Dummy + 34)

/* Name of a realtime.library conductor -- defaults to "Main" (UBYTE *) */
#define  DTA_Conductor          (DTA_Dummy + 35)

/* Specify whether a control panel should be embedded into the object or not
   (for example in the animation datatype) -- defaults to TRUE (BOOL) */
#define  DTA_ControlPanel       (DTA_Dummy + 36)

/* Should the object begin playing immediately? -- defaults to FALSE (BOOL) */
#define  DTA_Immediate          (DTA_Dummy + 37)

/* Indicate that the object should repeat playing -- defaults to FALSE (BOOL)*/
#define  DTA_Repeat             (DTA_Dummy + 38)

/* V44: Address of object if of type DTST_MEMORY */
#define  DTA_SourceAddress   	(DTA_Dummy + 39)

/* V44: Size of object if of type DTST_MEMORY */
#define  DTA_SourceSize	    	(DTA_Dummy + 40)

/* DTObject attributes */
#define  DTA_Name         (DTA_Dummy + 100)
#define  DTA_SourceType	  (DTA_Dummy + 101)
#define  DTA_Handle       (DTA_Dummy + 102)
#define  DTA_DataType     (DTA_Dummy + 103)
#define  DTA_Domain       (DTA_Dummy + 104)

#if 0
/* These should not be used, and is therefore not available -- use the
   corresponding tags in <intuition/gadgetclass> instead */
#define  DTA_Left         (DTA_Dummy + 105)
#define  DTA_Top          (DTA_Dummy + 106)
#define  DTA_Width        (DTA_Dummy + 107)
#define  DTA_Height       (DTA_Dummy + 108)
#endif


#define  DTA_ObjName         (DTA_Dummy + 109)
#define  DTA_ObjAuthor       (DTA_Dummy + 110)
#define  DTA_ObjAnnotation   (DTA_Dummy + 111)
#define  DTA_ObjCopyright    (DTA_Dummy + 112)
#define  DTA_ObjVersion      (DTA_Dummy + 113)
#define  DTA_ObjectID        (DTA_Dummy + 114)
#define  DTA_UserData        (DTA_Dummy + 115)
#define  DTA_FrameInfo       (DTA_Dummy + 116)


#if 0
/* These should not be used, and is therefore not available -- use the
   corresponding tags in <intuition/gadgetclass> instead */
#define  DTA_RelRight        (DTA_Dummy + 117)
#define  DTA_RelBottom       (DTA_Dummy + 118)
#define  DTA_RelWidth        (DTA_Dummy + 119)
#define  DTA_RelHeight       (DTA_Dummy + 120)
#endif


#define  DTA_SelectDomain    (DTA_Dummy + 121)
#define  DTA_TotalPVert      (DTA_Dummy + 122)
#define  DTA_TotalPHoriz     (DTA_Dummy + 123)
#define  DTA_NominalVert     (DTA_Dummy + 124)
#define  DTA_NominalHoriz    (DTA_Dummy + 125)


/* Printing attributes */

/* Destination x width (LONG) */
#define  DTA_DestCols        (DTA_Dummy + 400)

/* Destination y width (LONG) */
#define  DTA_DestRows        (DTA_Dummy + 401)

/* Option flags (UWORD) */
#define  DTA_Special         (DTA_Dummy + 402)

/* RastPort used when printing (struct RastPort *) */
#define  DTA_RastPort        (DTA_Dummy + 403)

/* Pointer to base name for ARexx port */
#define  DTA_ARexxPortName   (DTA_Dummy + 404)


/**************************************************************************/

#define  DTST_RAM         1
#define  DTST_FILE        2
#define  DTST_CLIPBOARD   3
#define  DTST_HOTLINK     4
#define  DTST_MEMORY	  5 /* V44 */

/* This structure is attached to the Gadget.SpecialInfo field of the gadget.
   Use Get/Set calls to access it. */
struct DTSpecialInfo
{
    struct SignalSemaphore si_Lock;
    ULONG                  si_Flags;
    LONG                   si_TopVert;    /* Top row (in units) */
    LONG                   si_VisVert;    /* Number of visible rows (in 
					     units) */
    LONG                   si_TotVert;    /* Total number of rows (in units) */
    LONG                   si_OTopVert;   /* Previous top (in units) */
    LONG                   si_VertUnit;   /* Number of pixels in vertical
					     unit */
    LONG                   si_TopHoriz;   /* Top column (in units) */
    LONG                   si_VisHoriz;   /* Number of visible columns (in
					     units) */
    LONG                   si_TotHoriz;   /* Total number of columns (in
					     units) */
    LONG                   si_OTopHoriz;  /* Previous top (in units) */
    LONG                   si_HorizUnit;  /* Number of pixels in horizontal
					     unit */
};


/* Object is in layout processing */
#define  DTSIF_LAYOUT           (1L << 0)

/* Object needs to be layed out */
#define  DTSIF_NEWSIZE          (1L << 1)

#define  DTSIF_DRAGGING         (1L << 2)
#define  DTSIF_DRAGSELECT       (1L << 3)

#define  DTSIF_HIGHLIGHT        (1L << 4)

/* Object is being printed */
#define  DTSIF_PRINTING         (1L << 5)

/* Object is in layout process */
#define  DTSIF_LAYOUTPROC       (1L << 6)



struct DTMethod
{
    STRPTR	 dtm_Label;
    STRPTR	 dtm_Command;
    ULONG	 dtm_Method;
};



#define  DTM_Dummy            (0x600)

/* Get the environment an object requires */
#define  DTM_FRAMEBOX         (0x601)

#define  DTM_PROCLAYOUT       (0x602)
#define  DTM_ASYNCLAYOUT      (0x603)

/* When RemoveDTObject() is called */
#define  DTM_REMOVEDTOBJECT   (0x604)

#define  DTM_SELECT           (0x605)
#define  DTM_CLEARSELECTED    (0x606)

#define  DTM_COPY             (0x607)
#define  DTM_PRINT            (0x608)
#define  DTM_ABORTPRINT       (0x609)

#define  DTM_NEWMEMBER        (0x610)
#define  DTM_DISPOSEMEMBER    (0x611)

#define  DTM_GOTO             (0x630)
#define  DTM_TRIGGER          (0x631)

#define  DTM_OBTAINDRAWINFO   (0x640)
#define  DTM_DRAW             (0x641)
#define  DTM_RELEASEDRAWINFO  (0x642)

#define  DTM_WRITE            (0x650)


struct FrameInfo
{
    ULONG       fri_PropertyFlags;   /* DisplayInfo (graphics/displayinfo.h) */
    Point       fri_Resolution;      /* DisplayInfo */

    UBYTE       fri_RedBits;
    UBYTE       fri_GreenBits;
    UBYTE       fri_BlueBits;

    struct
    {
	ULONG Width;
	ULONG Height;
	ULONG Depth;
	
    } fri_Dimensions;
    
    struct Screen    *fri_Screen;
    struct ColorMap  *fri_ColorMap;

    ULONG             fri_Flags;    /* See below */
};

#define	FIF_SCALABLE	0x1
#define	FIF_SCROLLABLE	0x2
#define	FIF_REMAPPABLE	0x4


/* DTM_REMOVEDTOBJECT, DTM_CLEARSELECTED, DTM_COPY, DTM_ABORTPRINT */
struct dtGeneral
{
    STACKULONG         MethodID;
    struct GadgetInfo *dtg_GInfo;
};

/* DTM_SELECT */
struct dtSelect
{
    STACKULONG         MethodID;
    struct GadgetInfo *dts_GInfo;
    struct Rectangle   dts_Select;
};

/* DTM_FRAMEBOX */
struct dtFrameBox
{
    STACKULONG         MethodID;
    struct GadgetInfo *dtf_GInfo;
    struct FrameInfo  *dtf_ContentsInfo;        /* Input */
    struct FrameInfo  *dtf_FrameInfo;           /* Output */
    STACKULONG         dtf_SizeFrameInfo;
    STACKULONG         dtf_FrameFlags;
};

/* DTM_GOTO */
struct dtGoto
{
    STACKULONG         MethodID;
    struct GadgetInfo *dtg_GInfo;
    STRPTR             dtg_NodeName;          /* Node to goto */
    struct TagItem    *dtg_AttrList;          /* Additional attributes */
};

/* DTM_TRIGGER */
struct dtTrigger
{
    STACKULONG         MethodID;
    struct GadgetInfo *dtt_GInfo;
    STACKULONG         dtt_Function;
    APTR               dtt_Data;
};


#define  STMF_METHOD_MASK    0x0000ffff
#define  STMF_DATA_MASK      0x00ff0000
#define  STMF_RESERVED_MASK  0xff000000

#define  STMD_VOID           0x00010000
#define  STMD_ULONG          0x00020000
#define  STMD_STRPTR         0x00030000
#define  STMD_TAGLIST        0x00040000

#define  STM_DONE            0
#define  STM_PAUSE           1
#define  STM_PLAY            2
#define  STM_CONTENTS        3
#define  STM_INDEX	     4
#define  STM_RETRACE         5
#define  STM_BROWSE_PREV     6
#define  STM_BROWSE_NEXT     7

#define  STM_NEXT_FIELD      8
#define  STM_PREV_FIELD	     9
#define  STM_ACTIVATE_FIELD  10

#define  STM_COMMAND         11

#define  STM_REWIND          12
#define  STM_FASTFORWARD     13
#define  STM_STOP            14
#define  STM_RESUME          15
#define  STM_LOCATE          16
/* 17 is reserved for help */
#define  STM_SEARCH          18
#define  STM_SEARCH_NEXT     19
#define  STM_SEARCH_PREV     20


#define  STM_USER            100

/* Printer IO request */
union printerIO
{
    struct IOStdReq 	ios;
    struct IODRPReq 	iodrp;
    struct IOPrtCmdReq  iopc;
};


/* DTM_PRINT */
struct dtPrint
{
    STACKULONG         MethodID;
    struct GadgetInfo *dtp_GInfo;           /* Gadget information */
    union printerIO   *dtp_PIO;             /* Printer IO request */
    struct TagItem    *dtp_AttrList;        /* Additional attributes */
};

/* DTM_DRAW */
struct dtDraw
{
    STACKULONG       MethodID;
    struct RastPort *dtd_RPort;
    STACKLONG        dtd_Left;
    STACKLONG        dtd_Top;
    STACKLONG        dtd_Width;
    STACKLONG        dtd_Height;
    STACKLONG        dtd_TopHoriz;
    STACKLONG        dtd_TopVert;
    struct TagItem  *dtd_AttrList;         /* Additional attributes */
};

/* DTM_RELEASEDRAWINFO */
struct dtReleaseDrawInfo

{
    STACKULONG   MethodID;
    APTR         dtr_Handle;		   /* Handle as returned by DTM_OBTAINDRAWINFO */
};


/* DTM_WRITE */
struct dtWrite
{
    STACKULONG         MethodID;
    struct GadgetInfo *dtw_GInfo;          /* Gadget information */
    BPTR               dtw_FileHandle;     /* File handle to write to */
    STACKULONG         dtw_Mode;
    struct TagItem    *dtw_AttrList;       /* Additional attributes */
};


/* Save data as IFF data */
#define  DTWM_IFF        0

/* Save data as local data format */
#define  DTWM_RAW        1


#endif /* DATATYPES_DATATYPESCLASS_H */
