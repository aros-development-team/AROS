
#if defined(__AROS__)
#include <aros/preprocessor/variadic/cast2iptr.hpp>
#endif


/* classbase.c */
DISPATCHERFLAGS struct IClass *ObtainMPEGVideoEngine ( REGA6 struct ClassBase * );
#if !defined(__AROS__)
DISPATCHERFLAGS struct Library *LibInit ( REGD0 struct ClassBase * , REGA0 BPTR seglist , REGA6 struct ExecBase *sysbase );
DISPATCHERFLAGS LONG LibOpen ( REGA6 struct ClassBase * );
DISPATCHERFLAGS LONG LibClose ( REGA6 struct ClassBase * );
DISPATCHERFLAGS LONG LibExpunge ( REGA6 struct ClassBase * );

/* stackswap.c */
DISPATCHERFLAGS ULONG StackSwapDispatch ( REGA0 struct IClass *cl , REGA2 Object *o , REGA1 Msg msg );
DISPATCHERFLAGS ULONG SwapMe ( REGA0 struct MyStackSwapStruct *mystk );
DISPATCHERFLAGS ULONG MyDispatch ( REGA0 struct MyStackSwapStruct *mystk );

/* dispatch.c */
struct IClass *initClass ( struct ClassBase * );
DISPATCHERFLAGS ULONG Dispatch ( REGA0 struct IClass *cl , REGA2 Object *o , REGA1 Msg msg );
#endif
struct FrameNode *AllocFrameNode ( struct ClassBase * , APTR pool );
void FreeFrameNode ( struct MPEGVideoInstData *mvid , struct FrameNode *fn );
struct FrameNode *FindFrameNode ( struct MinList *fnl , ULONG timestamp );
void verbose_printf ( struct MPEGVideoInstData *mvid , STRPTR format , ...);
void debug_printf ( struct MPEGVideoInstData *mvid , STRPTR format , ...);
void syntax_printf ( struct MPEGVideoInstData *mvid , STRPTR format , ...);
void error_printf ( struct MPEGVideoInstData *mvid , STRPTR format , ...);
#if !defined(__AROS__)
void mysprintf ( struct ClassBase * , STRPTR buffer , STRPTR fmt , ...);
#else
void OpenLogfile( struct ClassBase *, struct MPEGVideoInstData *);
#define mysprintf(cb,buffer,fmt,...) sprintf(buffer,fmt, __VA_ARGS__)
#define verbose_printf(mvid, fmt, ... ) \
{ \
    if( ((mvid) -> mvid_VerboseOutput) && (((mvid) -> mvid_VerboseOutput) != (BPTR)-1L) ) \
    { \
        IPTR pargs[] = { AROS_PP_VARIADIC_CAST2IPTR(__VA_ARGS__) }; \
        VFPrintf( ((mvid) -> mvid_VerboseOutput), (fmt), pargs); \
    } \
}
#define debug_printf(mvid, fmt, ... ) \
{ \
    if( ((mvid) -> mvid_DoDebug) && ((mvid) -> mvid_VerboseOutput) && (((mvid) -> mvid_VerboseOutput) != (BPTR)-1L) ) \
    { \
        IPTR pargs[] = { AROS_PP_VARIADIC_CAST2IPTR(__VA_ARGS__) }; \
        VFPrintf( ((mvid) -> mvid_VerboseOutput), (fmt), pargs); \
    } \
}
#define syntax_printf(mvid, fmt, ... ) \
{ \
    if( ((mvid) -> mvid_DoSyntax) && ((mvid) -> mvid_VerboseOutput) && (((mvid) -> mvid_VerboseOutput) != (BPTR)-1L) ) \
    { \
        IPTR pargs[] = { AROS_PP_VARIADIC_CAST2IPTR(__VA_ARGS__) }; \
        VFPrintf( ((mvid) -> mvid_VerboseOutput), (fmt), pargs); \
    } \
}
#define error_printf(mvid, fmt, ...) \
{ \
    IPTR errargs[] = { AROS_PP_VARIADIC_CAST2IPTR(__VA_ARGS__) }; \
    OpenLogfile( classbase, (mvid) ); \
    if ((mvid) -> mvid_VerboseOutput) \
    { \
        VFPrintf( ((mvid) -> mvid_VerboseOutput), (fmt), errargs); \
    } \
}
#endif

struct BitMap *AllocFrameBitMap ( struct MPEGVideoInstData *mvid );
void FreeFrameBitMap ( struct MPEGVideoInstData *mvid , struct BitMap *bm );
void UpdateProgressRequester ( struct MPEGVideoInstData *mvid );
//
LONG LoadFrames( struct ClassBase *, Object * );
struct FrameNode *FindNextIFrame( struct FrameNode * );

/* mpeg16bit.c */
void InitColorDither ( struct MPEGVideoInstData *mvid );
void Color16DitherImage ( struct MPEGVideoInstData *mvid , UBYTE *lum , UBYTE *cr , UBYTE *cb , UBYTE *out , UWORD cols , UWORD rows );
void Color32DitherImage ( struct MPEGVideoInstData *mvid , UBYTE *lum , UBYTE *cr , UBYTE *cb , UBYTE *out , UWORD cols , UWORD rows );
void Twox2Color16DitherImage ( struct MPEGVideoInstData *mvid , UBYTE *lum , UBYTE *cr , UBYTE *cb , UBYTE *out , UWORD cols , UWORD rows );
void Twox2Color32DitherImage ( struct MPEGVideoInstData *mvid , UBYTE *lum , UBYTE *cr , UBYTE *cb , UBYTE *out , UWORD cols , UWORD rows );

/* mpegamiga.c */
void mpeg_closedown ( struct MPEGVideoInstData *mvid );
void StoreFrame ( struct MPEGVideoInstData *mvid , UBYTE *data );
void AddFrame ( struct MPEGVideoInstData *mvid , UBYTE *data , struct ColorMap *cm );
void *mymalloc ( struct MPEGVideoInstData *mvid , size_t s );
void myfree ( struct MPEGVideoInstData *mvid , void *mem );
void *myrealloc ( struct MPEGVideoInstData *mvid , void *oldmem , size_t newsize );
ULONG SearchColor ( struct MPEGVideoInstData *mvid , struct ColorRegister *colortable , ULONG *numcolors , ULONG maxcount , struct ColorRegister *color );

/* mpegdecoders.c */
void init_tables ( void );
void decodeDCTDCSizeLum ( struct MPEGVideoInstData *mvid , unsigned int *value );
void decodeDCTDCSizeChrom ( struct MPEGVideoInstData *mvid , unsigned int *value );
void decodeDCTCoeff ( struct MPEGVideoInstData *mvid , const UWORD *dct_coeff_tbl , unsigned int *run , int *level );
void decodeDCTCoeffFirst ( struct MPEGVideoInstData *mvid , unsigned int *run , int *level );
void decodeDCTCoeffNext ( struct MPEGVideoInstData *mvid , unsigned int *run , int *level );

/* mpeggdith.c */
void InitColor ( struct MPEGVideoInstData *mvid );
void InitDisplay ( struct MPEGVideoInstData *mvid );
void InitGrayDisplay ( struct MPEGVideoInstData *mvid );
void InitHAMDisplay ( struct MPEGVideoInstData *mvid );
void ExecuteDisplay ( struct MPEGVideoInstData *mvid , VidStream *vid_stream );

/* mpeggray.c */
void GrayDitherImage ( struct MPEGVideoInstData *mvid , UBYTE *lum , UBYTE *cr , UBYTE *cb , UBYTE *out , UWORD h , UWORD w );

/* mpegjrevdct.c */
void init_pre_idct ( void );
void j_rev_dct_sparse ( struct MPEGVideoInstData *mvid , DCTBLOCK data , int pos );
void j_rev_dct ( DCTBLOCK data );

/* mpegfloatdct.c */
void init_float_idct ( void );
void float_idct ( short *block );

/* mpegmain.c */
int get_more_data ( struct MPEGVideoInstData *mvid , unsigned int *buf_start , int max_length , int *length_ptr , unsigned int **buf_ptr );
void loadvideo ( struct MPEGVideoInstData *mvid );
void DoDitherImage ( struct MPEGVideoInstData *mvid , UBYTE *l , UBYTE *Cr , UBYTE *Cb , UBYTE *disp , UWORD h , UWORD w );
void myexit ( struct MPEGVideoInstData *mvid , long retval , long retval2 );

/* mpegmotionvector.c */
void ComputeForwVector ( struct MPEGVideoInstData *mvid , int *recon_right_for_ptr , int *recon_down_for_ptr );
void ComputeBackVector ( struct MPEGVideoInstData *mvid , int *recon_right_back_ptr , int *recon_down_back_ptr );

/* mpegordered.c */
void InitOrderedDither ( struct MPEGVideoInstData *mvid );
void OrderedDitherImage ( struct MPEGVideoInstData *mvid , UBYTE *lum , UBYTE *cr , UBYTE *cb , UBYTE *out , UWORD h , UWORD w );

/* mpegparseblock.c */
void ParseReconBlock ( struct MPEGVideoInstData *mvid , int n );
void ParseAwayBlock ( struct MPEGVideoInstData *mvid , int n );

/* mpegutil.c */
void correct_underflow ( struct MPEGVideoInstData *mvid );
int next_bits ( struct MPEGVideoInstData *mvid , int num , unsigned int mask );
char *get_ext_data ( struct MPEGVideoInstData *mvid );
int next_start_code ( struct MPEGVideoInstData *mvid );
char *get_extra_bit_info ( struct MPEGVideoInstData *mvid );
long stream_pos ( struct MPEGVideoInstData *mvid );

/* mpegvideo.c */
VidStream *NewVidStream ( struct MPEGVideoInstData *mvid , int streambuflen );
void ResetVidStream ( struct MPEGVideoInstData *mvid , VidStream *vid );
PictImage *NewPictImage ( struct MPEGVideoInstData *mvid , unsigned int width , unsigned int height );
void DestroyPictImage ( struct MPEGVideoInstData *mvid , PictImage *apictimage );
VidStream *mpegVidRsrc ( struct MPEGVideoInstData *mvid , TimeStamp time_stamp , VidStream *vid_stream );
VidStream *mpegVidRsrcScan ( struct MPEGVideoInstData *mvid , TimeStamp time_stamp , VidStream *vid_stream );
ULONG GetFrameRate ( struct MPEGVideoInstData *mvid , VidStream *stream );
