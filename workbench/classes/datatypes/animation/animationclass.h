/*
    Copyright © 2015-2020, The AROS Development	Team. All rights reserved.
    $Id$
*/

#include <graphics/gfx.h>
#include <datatypes/pictureclass.h>
#include <datatypes/animationclass.h>
#include <datatypes/animationclassext.h>

#define	MIN(a,b) (((a) < (b)) ?	(a) : (b))
#define	MAX(a,b) (((a) > (b)) ?	(a) : (b))

/*
    api flags
 */
#define	ANIMDF_CONTROLPANEL     (1 << 0)
#define	ANIMDF_IMMEDIATE        (1 << 1)
#define	ANIMDF_REPEAT           (1 << 2)
#define	ANIMDF_REMAP            (1 << 3)
#define	ANIMDF_ADJUSTPALETTE    (1 << 4)
#define	ANIMDF_ADAPTFPS         (1 << 5)
#define	ANIMDF_FRAMESKIP        (1 << 6)
#define	ANIMDF_SMARTSKIP        (1 << 7)

/*
    special flags used by rendering/layout code
 */
#define ANIMDF_LAYOUT           (1 << 29)               
#define ANIMDF_REMAPPEDPENS     (1 << 30)               
#define ANIMDF_SHOWPANEL        (1 << 31)

#define ANIMPLAYER_TICKFREQ     ((struct RealTimeBase *)RealTimeBase)->rtb_Reserved1

struct ProcessPrivate;
struct Animation_Data;
struct AnimFrame;
BOOL ProcEnabled(struct ProcessPrivate *, volatile ULONG *, ULONG);
void cacheFrame(struct Animation_Data *, struct AnimFrame *);
void freeFrame(struct Animation_Data *, struct AnimFrame *);

struct AnimColor_Data
{
    struct SignalSemaphore      acd_PenLock;
    struct ColorMap             *acd_ColorMap;
    struct ColorRegister        *acd_ColorRegs;
    ULONG			*acd_CRegs;
    ULONG                       *acd_GRegs;

    UWORD                       acd_NumColors;
    UWORD                       acd_NumAlloc;

    UBYTE			*acd_ColorTable[2];
    UBYTE			*acd_Allocated;          /* pens we have actually allocated      */
    ULONG                       acd_PenPrecison;         /* precision to use allocating pens     */
};

struct AnimFrame_Data
{
    struct SignalSemaphore      afd_AnimFramesLock;
    struct List                 afd_AnimFrames;

    UWORD                       afd_Frames;              /* # of frames                          */
    UWORD                       afd_FrameCurrent;        /* # of current frame                   */
    UWORD                       afd_FramesStep;          /* how much to skip back/fwd            */
};

struct AnimTimer_Data
{
    UWORD                       atd_FramesPerSec;        /* Playback rate                        */
    UWORD                       atd_TicksPerFrame;       /* realtime.libraries tick frequency /
                                                           ad_FramesPerSec */
    UWORD                       atd_Tick;
};

/* our nodes used to play the anim! */
struct AnimFrame
{
    struct Node                 af_Node;
#define af_CacheBM af_Node.ln_Name
    ULONG                       af_Flags;
    struct adtNewFormatFrame    af_Frame;
};

#define AFFLAGB_READY      0
#define AFFLAGF_READY      (1 << 0)

/* for sanity, we embed the frame number in the ln_type/ln_pri fields */
static inline UWORD GetNODEID(struct AnimFrame *node) {
    UWORD *id_ptr = (UWORD *)&node->af_Node.ln_Type;
    return *id_ptr;
}

static inline void SetNODEID(struct AnimFrame *node, UWORD id) {
    UWORD *id_ptr = (UWORD *)&node->af_Node.ln_Type;
    *id_ptr = id;
}

struct Animation_Data
{
    ULONG                       ad_Flags;               /* object control flags                 */
    char                        *ad_BaseName;

    struct Window               *ad_Window;

    struct AnimFrame_Data       ad_FrameData;
    struct AnimTimer_Data       ad_TimerData;

    struct BitMap               *ad_FrameBM;        /* currently displayed frame            */
    struct BitMap               *ad_CacheBM;        /* .. */
    struct AnimFrame            *ad_KeyFrame;       /* animations key (first) frame         */

    UWORD                       ad_VertTop;             /* Y offset of visible rectangle        */
    UWORD                       ad_VertTotal;           
    UWORD                       ad_VertVis;
    UWORD                       ad_HorizTop;            /* X offset of visible rectangle        */
    UWORD                       ad_HorizTotal;
    UWORD                       ad_HorizVis;

    UWORD                       ad_RenderLeft;
    UWORD                       ad_RenderTop;
    UWORD                       ad_RenderWidth;
    UWORD                       ad_RenderHeight;

    IPTR                        ad_ModeID;
    struct BitMapHeader         ad_BitMapHeader;        /* objects embedded bitmap header       */

    struct AnimColor_Data       ad_ColorData;

    IPTR                        ad_ProcStack;
    struct ProcessPrivate       *ad_ProcessData;
    struct Process              *ad_BufferProc;         /* buffering process */
    struct Process              *ad_PlayerProc;         /* playback process */
    struct Player               *ad_Player;
    struct Hook                 ad_PlayerHook;

    struct Gadget               *ad_Tapedeck;
    ULONG                       ad_BufferTime;         /* (prefs) how many seconds to buffer  */
    ULONG                       ad_BufferStep;         /* (prefs) no of frames to try to load in one go */
    UBYTE                       ad_PlayerSourceLastState;
};

struct ProcessPrivate
{
    Object                      *pp_Object;
    struct Animation_Data       *pp_Data;
    char                        *pp_PlayBackName;
    char                        *pp_BufferingName;
    volatile ULONG              pp_PlayerFlags;
    volatile ULONG              pp_BufferFlags;
    ULONG                       pp_BufferFrames;       /* no of frames to buffer in total       */
    ULONG                       pp_BufferLevel;        /* no of frames buffered                 */
    IPTR                        pp_BufferSpecific;     /* specific frame to load                */
    struct AnimFrame            *pp_BufferFirst;       /* starting point to load from           */
    struct AnimFrame            *pp_PlaybackFrame;

    ULONG                       pp_BufferSigMask;
    UBYTE                       pp_BufferEnable;
    UBYTE                       pp_BufferDisable;
    UBYTE                       pp_BufferFill;
    UBYTE                       pp_BufferPurge;

    ULONG                       pp_PlaybackSigMask;
    UBYTE                       pp_PlaybackEnable;
    UBYTE                       pp_PlaybackDisable;
    UBYTE                       pp_PlaybackTick;          /* signal frames needs to change      */
    UBYTE                       pp_PlaybackSync;          /* signal position changed            */
};

#define PRIVPROCF_ENABLED       (1 << 0)
#define PRIVPROCF_RUNNING       (1 << 1)
#define PRIVPROCF_ACTIVE        (1 << 2)
#define PRIVPROCF_BUSY          (1 << 3)

#define TAG_PRIVATE             	(ADTA_Dummy + 100)
#define PRIVATE_INITPLAYER              (TAG_PRIVATE - 1)
#define PRIVATE_ALLOCCOLORTABLES        (TAG_PRIVATE - 2)
#define PRIVATE_MAPFRAMEPENS            (TAG_PRIVATE - 3)     
#define PRIVATE_FREECOLORTABLES         (TAG_PRIVATE - 4) 
#define PRIVATE_FREEPENS                (TAG_PRIVATE - 5)             
#define PRIVATE_ALLOCBUFFER             (TAG_PRIVATE - 6)
#define PRIVATE_RENDERFRAME             (TAG_PRIVATE - 7)
#define PRIVATE_REMAPFRAME              (TAG_PRIVATE - 8)

struct privAllocColorTables
{
    STACKED ULONG MethodID;
    STACKED ULONG NumColors;
};

struct privMapFramePens
{
    STACKED ULONG MethodID;
    STACKED struct AnimFrame *Frame;
};

struct privAllocBuffer
{
    STACKED ULONG MethodID;
    STACKED struct BitMap *Friend;
    STACKED UBYTE Depth;
};

struct privRenderFrame
{
    STACKED ULONG MethodID;
    STACKED struct AnimFrame *Frame;
    STACKED struct BitMap *Target;
};

#if DEBUG > 0
#define DFRAMES(...) bug(__VA_ARGS__);
#else
#define DFRAMES(...)
#endif
