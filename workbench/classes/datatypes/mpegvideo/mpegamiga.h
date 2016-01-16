
#ifndef MPEGAMIGA_H
#define MPEGAMIGA_H 1

/*
**
**  $VER: mpegamiga.h 1.11 (2.11.97)
**  mpegvideo.datatype 1.11
**
**  amiga includes
**
**  Written 1996/1997 by Roland 'Gizzy' Mainz
**
*/

struct ProgressRequester
{
    struct Window *pr_Window;
    ULONG          pr_Curr;
    ULONG          pr_Max;  /* ~0UL means: gauge disabled */
    ULONG          pr_StartSecond;
};

struct FrameNode
{
    struct MinNode      fn_Node;

/* Misc */
    WORD                fn_UseCount;
    BOOL                fn_IsKeyFrame; /* This frame must not be freed by ADTM_UNLOADFRAME */
    struct FrameNode   *fn_IFrame;     /* links back to the last I-Frame  */
    ULONG               fn_Reserved0;
    ULONG               fn_Reserved1;
    ULONG               fn_Reserved2;

/* Timing section */
    ULONG               fn_TimeStamp;
    ULONG               fn_Frame;
    ULONG               fn_Duration;

/* BitMap loading section */
    LONG                fn_BMOffset; /* File offset (0 is begin of file) of last I frame */
    ULONG               fn_BMSize;   /* Chunk size  */

/* Bitmap section */
    struct BitMap      *fn_BitMap;
    struct ColorMap    *fn_CMap;

/* Sample section */
    BYTE               *fn_Sample;
    ULONG               fn_SampleLength;
    ULONG               fn_Period;
};


/* mvid.datatype class instance data */
struct MPEGVideoInstData
{
    struct ClassBase        *mvid_ClassBase;      /* Links back to class library base */

    struct SignalSemaphore   mvid_SigSem;         /* Instance data lock                      */
    UWORD                    mvid_Pad0;

    APTR                     mvid_Pool;           /* Pool for misc frames  */
    BPTR                     mvid_VerboseOutput;  /* verbose log file */
    struct BitMap           *mvid_KeyBitMap;      /* key bitmap */
    struct MinList           mvid_FrameList;      /* frame nodes itself (list of struct FrameNodes) */
    struct FrameNode        *mvid_FirstFrameNode;
    ULONG                    mvid_TicksPerFrame;
    LONG                     mvid_retval;
    LONG                     mvid_retval2;
    ULONG                    mvid_ModeID;
    ULONG                    mvid_MaxFrame;
    ULONG                    mvid_SkipFrames;
    BOOL                     mvid_IgnoreErrors;
    BOOL                     mvid_LoadAll;
    BYTE                    *mvid_Sample;
    ULONG                    mvid_SampleLength;
    ULONG                    mvid_Period;
    ULONG                    mvid_Volume;
    ULONG                    mvid_MinTotalMem;

    /* Used by SearchColor (mpegamiga.c) */
    LONG                     mvid_ColorError;
    BOOL                     mvid_PalettePerFrame;
    BOOL                     mvid_Quality;  /* use floating-point DCT */

    /* input buffer */
    ULONG                    mvid_BufLength;

    /* used by mpegordered.c (static) */
    /* Structures used to implement hybrid ordered dither/floyd-steinberg dither algorithm. */
#define DITH_SIZE (16)
    ULONG                   *mvid_l_darrays[ DITH_SIZE ];
    ULONG                   *mvid_cr_darrays[ DITH_SIZE ];
    ULONG                   *mvid_cb_darrays[ DITH_SIZE ];
#define l_darrays  (mvid -> mvid_l_darrays)
#define cr_darrays (mvid -> mvid_cr_darrays)
#define cb_darrays (mvid -> mvid_cb_darrays)

    /* globals */
    /* used by bit i/o */
    ULONG                    mpegv_curBits;
    LONG                     mpegv_bitOffset;
    LONG                     mpegv_bufLength;
    ULONG                   *mpegv_bitBuffer;
/* External declarations of bitstream global variables. */
#define curBits   (mvid -> mpegv_curBits)
#define bitOffset (mvid -> mpegv_bitOffset)
#define bufLength (mvid -> mpegv_bufLength)
#define bitBuffer (mvid -> mpegv_bitBuffer)

    /* used to track the file position of the last read */
    LONG                     mvid_ReadMarkPos; /* file position of last Read() */
    UBYTE                   *mvid_ReadMark;    /* buffer pos of last read */

    LONG                     mvid_Last_PIC_SC_Pos; /* Last position of PICTURE_START_CODE or GOP_START_CODE */
    LONG                     mvid_Last_I_TYPE_Pos; /* begin of last I_TYPE frame */

    struct FrameNode        *mvid_LastIFrameNode;
    VidStream               *mvid_VidStream;
    BOOL                     mvid_IndexScan;
    BOOL                     mvid_DoDebug2; /* obsolete */
    BOOL                     mvid_DoDebug;
    BOOL                     mvid_DoSyntax;

    double                   mvid_gammaCorrect;  /* Defaults to 1.0 */
    double                   mvid_chromaCorrect; /* Defaults to 1.0 */

#define gammaCorrect  (mvid -> mvid_gammaCorrect)
#define chromaCorrect (mvid -> mvid_chromaCorrect)

    /* used in mpeg16bit.c (static) */
    long                    *L_tab,
                            *Cr_r_tab,
                            *Cr_g_tab,
                            *Cb_g_tab,
                            *Cb_b_tab;

#define L_tab    (mvid -> L_tab)
#define Cr_r_tab (mvid -> Cr_r_tab)
#define Cr_g_tab (mvid -> Cr_g_tab)
#define Cb_g_tab (mvid -> Cb_g_tab)
#define Cb_b_tab (mvid -> Cb_b_tab)

    /* We define tables that convert a color value between -256 and 512
     * into the R, G and B parts of the pixel. The normal range is 0-255.
     */

    long                    *r_2_pix;
    long                    *g_2_pix;
    long                    *b_2_pix;
    long                    *r_2_pix_alloc;
    long                    *g_2_pix_alloc;
    long                    *b_2_pix_alloc;

#define r_2_pix         (mvid -> r_2_pix)
#define g_2_pix         (mvid -> g_2_pix)
#define b_2_pix         (mvid -> b_2_pix)
#define r_2_pix_alloc   (mvid -> r_2_pix_alloc)
#define g_2_pix_alloc   (mvid -> g_2_pix_alloc)
#define b_2_pix_alloc   (mvid -> b_2_pix_alloc)

    /* used by video.c/mpegVidRsrc (first set to TRUE !!) */
    BOOL                     mvid_mpegVidRsrc_first;
    BOOL                     mvid_UseVMM; /* use vmm.library memory for frame bitmaps */

    /* used by mpegvideo.c/ReconPMBlock */
    int                      ReconPMBlock_right_for,
                             ReconPMBlock_down_for,
                             ReconPMBlock_right_half_for,
                             ReconPMBlock_down_half_for;

    /* used in mpegvideo.c (static) */
    BOOL                     No_P_Flag;
    BOOL                     No_B_Flag;

    /* set in mpeggdith.c */
    /* Range values for lum, cr, cb. */
    ULONG                    LUM_RANGE;
    ULONG                    CR_RANGE;
    ULONG                    CB_RANGE;
#define LUM_RANGE (mvid -> LUM_RANGE)
#define CR_RANGE  (mvid -> CR_RANGE)
#define CB_RANGE  (mvid -> CB_RANGE)

    /* used by mpeggdith.c && mpeggray.c && mpegordered.c */
    LONG *mappixel; /* Array that remaps color numbers to actual pixel values */
#define mappixel      (mvid -> mappixel)

    /* Arrays holding quantized value ranged for lum, cr, and cb. */
    UBYTE                   *lum_values;
    UBYTE                   *cr_values;
    UBYTE                   *cb_values;
#define lum_values (mvid -> lum_values)
#define cr_values  (mvid -> cr_values)
#define cb_values  (mvid -> cb_values)

    /* globals */
    ULONG                     ditherType;        /* Declaration of global variable to hold dither info. */
#define ditherType    (mvid -> ditherType)
    ULONG                     pixfmt;            /* CyberGFX pixel format */
#define pixfmt        (mvid -> pixfmt)

    /* from mpeggdith.c (static) */
    BYTE                    *imagedata;

    /* used by mpegmain.c (static) */
    BPTR                     input;      /* Global file pointer to incoming data. */
    BOOL                     EOF_flag;   /* End of File flag. */
    ULONG                    xtpf;       /* streams TicksPerFrame */
    jmp_buf                  exit_buf;
#define EOF_flag      (mvid -> EOF_flag)
#define xtpf          (mvid -> xtpf)
#define exit_buf      (mvid -> exit_buf)

    VidStream               *curVidStream; /* Declaration of global pointer to current video stream. */
#define curVidStream  (mvid -> curVidStream)

    /* used in mpegvideo.c (static) */
    /* Max lum, chrom indices for illegal block checking. */
    int                      lmaxx;
    int                      lmaxy;
    int                      cmaxx;
    int                      cmaxy;
#define lmaxx         (mvid -> lmaxx)
#define lmaxy         (mvid -> lmaxy)
#define cmaxx         (mvid -> cmaxx)
#define cmaxy         (mvid -> cmaxy)

    /* used in mpegvideo.c && mpeggdith.c */
    ULONG                    totNumFrames;
#define totNumFrames  (mvid -> totNumFrames)

    /* video.c (static) */
    APTR                     mvid_MPPool;
    struct ColorRegister     used_colors[ 512 ];
    ULONG                    used_cnt;
    UWORD                    anim_width,
                             anim_height,
                             anim_depth,
                             video_width,
                             video_height;
    BOOL                     mvid_UseChunkyMap;
#define mpegrp                (mvid -> mpegrp)
#define temprp                (mvid -> temprp)
#define used_colors           (mvid -> used_colors)
#define used_cnt              (mvid -> used_cnt)
#define anim_width            (mvid -> anim_width)
#define anim_height           (mvid -> anim_height)
#define anim_depth            (mvid -> anim_depth)
#define video_width           (mvid -> video_width)
#define video_height          (mvid -> video_height)
    struct ProgressRequester mvid_PR;

    STRPTR                   mvid_ProjectName; /* From DTA_Name */
};

#endif /* !MPEGAMIGA_H */



