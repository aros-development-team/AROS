#ifndef CLASSDATA_H
#define CLASSDATA_H 1

/*
**
** $Id$
**  anim.datatype 1.11
**
**  anim class data
**
**  Written 1996/1997 by Roland 'Gizzy' Mainz
**  Original example source from David N. Junod
**
*/

/*****************************************************************************/

/* public BOOPSI class name */
#define ANIMDTCLASS "anim.datatype"

/*****************************************************************************/

/* Animation compression modes  */
#define acmpILBM            (0U)
#define acmpXORILBM         (1U)
#define acmpLongDelta       (2U)
#define acmpShortDelta      (3U)
#define acmpDelta           (4U)
#define acmpByteDelta       (5U)
#define acmpStereoByteDelta (6U)
#define acmpAnim7           (7U)
#define acmpAnim8           (8U)
#define acmpAnimI          (73U)
#define acmpAnimJ          (74U) /* ascii 'J', Bill Graham's compression format */

/* Animation header flags */
#define ahfLongData         (1U << 0U)
#define ahfXOR              (1U << 1U)
#define ahfOneInfoList      (1U << 2U)
#define ahfRLC              (1U << 3U)
#define ahfVertical         (1U << 4U)
#define ahfLongInfoOffsets  (1U << 5U)

/* Generic IFF Chunk ID's we may encounter */
#ifndef ID_ANNO
#define ID_ANNO         MAKE_ID('A','N','N','O')
#endif /* !ID_ANNO */

#ifndef ID_AUTH
#define ID_AUTH         MAKE_ID('A','U','T','H')
#endif /* !ID_AUTH */

#ifndef ID_Copyright
#define ID_Copyright    MAKE_ID('(','c',')',' ')
#endif /* !ID_Copyright */

#ifndef ID_FVER
#define ID_FVER         MAKE_ID('F','V','E','R')
#endif /* !ID_FVER */

#ifndef ID_NAME
#define ID_NAME         MAKE_ID('N','A','M','E')
#endif /* !ID_NAME */

/* IFF ANIM related IDs */
#ifndef ID_DPAN
#define ID_DPAN         MAKE_ID('D','P','A','N')
#endif /* !ID_DPAN */

struct DPAnimChunk
{
    UWORD dpan_Version;
    UWORD dpan_nframes;
    union {
        ULONG dpan_flags;
        struct {
            UBYTE dpan_FPS;
            UBYTE dpan_Pad[3];
        };
    };
};

/*****************************************************************************/

/* anim.datatype class instance data */
struct AnimInstData
{
    struct DataType     *aid_dt;

    /* Disk-loading section */
#ifdef DOASYNCIO
    struct AsyncFile       *aid_FH;
#else
    BPTR                    aid_FH;
#endif /* DOASYNCIO */
    LONG                    aid_CurrFilePos;

    /* Misc */
    struct SignalSemaphore  aid_SigSem;             /* Instance data lock                      */

    UBYTE                   aid_AnimMode;
    UBYTE                   aid_AnimFeat;

    APTR                    aid_Pool;               /* pool for misc things */
    APTR                    aid_FramePool;          /* pool for animation bitmaps */
    struct BitMapHeader    *aid_BMH;                /* Short cut to animation.datatype's bitmapheader */
    struct BitMap          *aid_KeyBitMap;          /* Key BitMap                              */
    struct MinList          aid_FrameList;          /* List of frames                          */
    struct MinList          aid_PostedFreeList;     /* List of frames which should be freed */
    struct FrameNode       *aid_CurrFN;             /* Last framenode obtained using ADTM_LOADFRAME */
    STRPTR                  aid_ProjectName;        /* Shortcut to DTA_Name                    */
    BPTR                    aid_VerboseOutput;      /* Verbose output                          */

    /* Sample stuff */
    BYTE                   *aid_Sample;             /* global sample */
    ULONG                   aid_SampleLength;       /* length of global sample */
    ULONG                   aid_Period;             /* period */
    ULONG                   aid_Volume;             /* volume */
    ULONG                   aid_SamplesPerFrame;    /* samples per frame; Set by prefs to override own calculations */

    /* Prefs */
    ULONG                   aid_ModeID;
    BOOL                    aid_NoCMAPs;            /* Don't create colormaps                  */
    BOOL                    aid_LoadAll;            /* Load all frames of the animation        */
    BOOL                    aid_AsyncIO;            /* sync or async IO ?                      */
    BOOL                    aid_NoDynamicTiming;    /* No dynamic timing ?                     */
    BOOL                    aid_NoDPaintBrushPatch; /* No forced xor mode for ANIM-5 with interleave == 1 */
    BOOL                    aid_Registered;         /* Shareware payed ? */
    ULONG                   aid_FPS;                /* fps of stream (maybe modified by prefs) */

};


/* node which holds information about a single animation frame */
struct FrameNode
{
    struct MinNode     fn_Node;           /* Node in aid_FrameList      */
    struct MinNode     fn_PostedFreeNode; /* Node in aid_PostedFreeList */

/* Get beginning of struct FrameNode from fn_PostedFreeNode */
#define POSTEDFREENODE2FN( pfn ) ((struct FrameNode *)(((struct MinNode *)(pfn)) - 1))

    struct FrameNode  *fn_PrevFrame;

/* Misc */
    UWORD              fn_PostedFree;
    WORD               fn_UseCount;       /* In-Use counter */

/* Timing section */
    ULONG              fn_TimeStamp;
    ULONG              fn_Frame;
    ULONG              fn_Duration;

/* Animation info */
    struct AnimHeader  fn_AH;

/* Bitmap/ColorMap section */
    struct BitMap     *fn_BitMap;
    struct ColorMap   *fn_CMap;

/* BitMap loading section */
    LONG               fn_BMOffset; /* File offset (0 is begin of file) */
    ULONG              fn_BMSize;   /* Chunk size  */
    ULONG              fn_LoadSize; /* temporary variable used by ADTM_LOADFRAME */

/* Sample section */
    BYTE              *fn_Sample;
    ULONG              fn_SampleLength;
    ULONG              fn_Period;
};

/*****************************************************************************/

/* encoder context data */
struct AnimContext
{
    APTR               ac_Pool;             /* memory pool for this context */
    UBYTE             *ac_WorkBuffer;
    ULONG              ac_WorkBufferSize;
    struct BitMap     *ac_BitMap[ 2 ];      /* two buffers...               */
    WORD               ac_WhichBitMap;      /* which buffer: 0 or 1         */
};


/*****************************************************************************/


#endif /* !CLASSDATA_H */
