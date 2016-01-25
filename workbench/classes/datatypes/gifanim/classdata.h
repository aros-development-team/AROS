
#ifndef CLASSDATA_H
#define CLASSDATA_H 1

/*
**
**  $VER: classdata.h 2.3 (24.5.98)
**  gifanim.datatype 2.3
**
**  gifanim class data
**
**  Written 1997/1998 by Roland 'Gizzy' Mainz
**  Original example source from David N. Junod
**
*/

/*****************************************************************************/

/* public BOOPSI class name */
#define GIFANIMDTCLASS "gifanim.datatype"

/*****************************************************************************/

/* Use WPA8, but do only on demand */
#define DELTAWPA8 1

/* Some CyberGFX configration stuff */
#define DIRECTRGB_DEPTH  (16UL)
#define DIRECTRGB_PIXFMT (PIXFMT_RGB16)

/*****************************************************************************/

/* Maximum number of colors in a GIF picture */
#define MAXCOLORMAPSIZE (256)

#define MAX_LWZ_BITS (12)

/* size of a gif colormap entry (r, g. b)*/
#define GIFCMAPENTRYSIZE (3UL)

/* GIF flags and test macro */
#define SORTED         (0x20) /* colors sorted, important first */
#define INTERLACE      (0x40) /* interlaced picture             */
#define LOCALCOLORMAP  (0x80) /* frames have their own colormap */
#define BitSet( byte, bit ) (((byte) & (bit)) == (bit))

/*****************************************************************************/

/* macros to get rid of Intel 80x86 byte order */
#define LOHI2UINT16( a, b ) ((UWORD)(((b) << 8)|(a)))

#define SWAPW(a)  ((WORD)(((UWORD)(a)>>8)+((((UWORD)(a)&0xff)<<8))))
#define SWAPUW(a) ((UWORD)(((UWORD)(a)>>8)+((((UWORD)(a)&0xff)<<8))))
#define SWAPL(a)  ((LONG)(((ULONG)(a)>>24)+(((ULONG)(a)&0xff0000)>>8)+(((ULONG)(a)&0xff00)<<8)+(((ULONG)(a)&0xff)<<24)))
#define SWAPUL(a) ((ULONG)(((ULONG)(a)>>24)+(((ULONG)(a)&0xff0000)>>8)+(((ULONG)(a)&0xff00)<<8)+(((ULONG)(a)&0xff)<<24)))

/*****************************************************************************/

/* GIF decoder context data */
struct GIFDecoder
{
    BOOL   which_fh;
    BOOL   pad0;
/* Read from file or pre-filled buffer (which merges multiple Read's to one Read */
#define WHICHFH_FILE   (0)
#define WHICHFH_BUFFER (1)
    BPTR   file;
    UBYTE *file_buffer; /* buffer start       */
    UBYTE *buffer;      /* current buffer pos */
    ULONG  buffersize;  /* buffer size        */

    struct
    {
      UWORD                Width;
      UWORD                Height;
      struct ColorRegister ColorMap[ MAXCOLORMAPSIZE ]; /* colormap         */
      UWORD                BitPixel;                    /* number of colors */
      UWORD                ColorResolution;
      UWORD                AspectRatio;
      UBYTE                Background;                  /* background pen   */
      UBYTE                Pad1;
    } GifScreen;

    struct
    {
      UWORD transparent; /* transparent pen; ~0U for nothing transparent */
      UWORD delayTime;   /* frame delay in 1/100 sec */
      BOOL  inputFlag;   /* user input before continue ? */
      UBYTE disposal;
#define GIF89A_DISPOSE_NOP                  (0U) /* No disposal specified. The decoder is not required to take any action. */
#define GIF89A_DISPOSE_NODISPOSE            (1U) /* Do not dispose. The graphic is to be left in place. */
#define GIF89A_DISPOSE_RESTOREBACKGROUND    (2U) /* Restore to background color. The area used by the graphic must be restored to the background color. */
#define GIF89A_DISPOSE_RESTOREPREVIOUS      (3U) /* Restore to previous. The decoder is required to restore the area overwritten by the graphic with what was there prior to rendering the graphic. */
#define GIF89A_DISPOSE_RESERVED4            (4U) /* reserved */
#define GIF89A_DISPOSE_RESERVED5            (5U) /* reserved */
#define GIF89A_DISPOSE_RESERVED6            (6U) /* reserved */
#define GIF89A_DISPOSE_RESERVED7            (7U) /* reserved */
      UBYTE Pad2;
    } Gif89; /* = { (UWORD)~0U, (UWORD)~0U, FALSE, GIF89A_DISPOSE_NOP };*/

    /* GetCode static vars */
    struct
    {
      UBYTE    buf[ 280 ];
      int      curbit,
               lastbit,
               last_byte;
      BOOL     done;
    } GetCode;

    BOOL     ZeroDataBlock; /* defaults to FALSE */

    /* LWZReadByte static vars */
    struct
    {
      int      code_size,
               set_code_size;
      int      max_code,
               max_code_size;
      int      firstcode,
               oldcode;
      int      clear_code,
               end_code;
      short    table[ 2 ][ (1 << MAX_LWZ_BITS) ];
      short    stack[ (1 << (MAX_LWZ_BITS)) * 2 ],
              *sp;
      BOOL     fresh /*= FALSE*/;
    } LWZReadByte;
};

/*******************************************************************************/

/* gifanim.datatype class instance data */
struct GIFAnimInstData
{
    struct DataType     *gaid_dt;
    char                        *gaid_VerStr;

    /* Misc */
    struct SignalSemaphore  gaid_SigSem;          /* Instance data lock                      */
    UWORD                   gaid_Pad0;
    APTR                    gaid_Pool;
    UWORD                   gaid_Width,
                            gaid_PaddedWidth,
                            gaid_Height,
                            gaid_Depth;
    struct BitMap          *gaid_KeyBitMap;       /* Key BitMap                              */
    struct MinList          gaid_FrameList;       /* List of frames                          */
    STRPTR                  gaid_ProjectName;     /* Shortcut to DTA_Name                    */
    BPTR                    gaid_VerboseOutput;   /* Verbose output. -1 means: Avoid any output (NOVERBOSE option) */

    /* Prefs */
    IPTR                   gaid_ModeID;
    BOOL                    gaid_LoadAll;         /* Load all frames of the animation        */
    BOOL                    gaid_FPS;             /* fps of stream (maybe modified by prefs) */
    BOOL                    gaid_UseChunkyMap;
    BOOL                    gaid_StrictSyntax;

    /* Sample stuff */
    BYTE                   *gaid_Sample;
    ULONG                   gaid_SampleLength;
    ULONG                   gaid_Period;
    ULONG                   gaid_Volume;
    ULONG                   gaid_SamplesPerFrame;

    /* Disk-loading section */
    BPTR                    gaid_FH;
    LONG                    gaid_CurrFilePos;

    /* decoder specific data */
    struct GIFDecoder       gaid_GIFDec;
};


/* node which holds information about a single animation frame */
struct FrameNode
{
    struct MinNode        fn_Node;

/* Misc */
    WORD                  fn_UseCount;
    UWORD                 fn_Pad0;

/* Timing section */
    ULONG                 fn_TimeStamp;
    ULONG                 fn_Frame;
    ULONG                 fn_Duration;

/* Bitmap/ColorMap section */
    struct BitMap        *fn_BitMap;
    struct ColorMap      *fn_CMap;

    UBYTE                *fn_ChunkyMap;                   /* bitmap data in chunky version */
    struct ColorRegister  fn_ColorMap[ MAXCOLORMAPSIZE ]; /* colormap, used for 24 bit output */


/* BitMap loading section */
    LONG                  fn_BMOffset; /* File offset (0 is begin of file) */
    ULONG                 fn_BMSize;   /* Chunk size  */

/* GIF89a specific */
    UWORD                 fn_GIF89aDisposal; /* GIF 89a extension disposal mode */
    UWORD                 fn_GIF89aTransparent;

/* Sample section */
    BYTE                 *fn_Sample;
    ULONG                 fn_SampleLength;
    ULONG                 fn_Period;
};

/*******************************************************************************/

#endif /* !CLASSDATA_H */


