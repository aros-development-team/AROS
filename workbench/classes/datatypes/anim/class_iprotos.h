/*
**
** $Id$
**  anim.datatype 1.12
**
*/

#if defined(__AROS__)
#include <aros/preprocessor/variadic/cast2iptr.hpp>
#endif

/* classbase.c */
DISPATCHERFLAGS struct IClass *ObtainAnimEngine ( REGA6 struct ClassBase *cb );
#if !defined(__AROS__)
DISPATCHERFLAGS struct Library *LibInit ( REGD0 struct ClassBase *cb , REGA0 BPTR seglist , REGA6 struct ExecBase *sysbase );
DISPATCHERFLAGS LONG LibOpen ( REGA6 struct ClassBase *cb );
DISPATCHERFLAGS LONG LibClose ( REGA6 struct ClassBase *cb );
DISPATCHERFLAGS LONG LibExpunge ( REGA6 struct ClassBase *cb );

/* stackswap.c */
DISPATCHERFLAGS ULONG StackSwapDispatch ( REGA0 struct IClass *cl , REGA2 Object *o , REGA1 Msg msg );
DISPATCHERFLAGS ULONG SwapMe ( REGA0 struct MyStackSwapStruct *mystk );
DISPATCHERFLAGS ULONG MyDispatch ( REGA0 struct MyStackSwapStruct *mystk );
#endif

/* dispatch.c */
#if !defined(__AROS__)
struct IClass *initClass ( struct ClassBase *cb );
DISPATCHERFLAGS ULONG Dispatch ( REGA0 struct IClass *cl , REGA2 Object *o , REGA1 Msg msg );
#endif
void ReadENVPrefs( struct ClassBase *cb, struct AnimInstData *aid );
void OpenLogfile ( struct ClassBase *cb , struct AnimInstData *gaid );
void error_printf ( struct ClassBase *cb , struct AnimInstData *gaid , STRPTR format , ...);
void verbose_printf ( struct ClassBase *cb , struct AnimInstData *gaid , STRPTR format , ...);
#if !defined(__AROS__)
void mysprintf ( struct ClassBase *cb , STRPTR buffer , STRPTR fmt , ...);
APTR AllocPooledVec ( struct ClassBase *cb , APTR pool , ULONG memsize );
void FreePooledVec ( struct ClassBase *cb , APTR pool , APTR mem );
#else
#define mysprintf(cb, buffer, fmt, ...) sprintf(buffer, fmt, __VA_ARGS__)
#define error_printf(cb, aid, fmt, ...) \
{ \
    IPTR errargs[] = { AROS_PP_VARIADIC_CAST2IPTR(__VA_ARGS__) }; \
    OpenLogfile( (cb), (aid) ); \
    if ((aid) -> aid_VerboseOutput) \
    { \
        VFPrintf( ((aid) -> aid_VerboseOutput), (fmt), errargs); \
    } \
}

#define verbose_printf(cb, aid, fmt, ... ) \
{ \
    IPTR pargs[] = { AROS_PP_VARIADIC_CAST2IPTR(__VA_ARGS__) }; \
    if( ((aid) -> aid_VerboseOutput) && (((aid) -> aid_VerboseOutput) != (BPTR)-1L) ) \
    { \
        VFPrintf( ((aid) -> aid_VerboseOutput), (fmt), pargs); \
    } \
}
#define AllocPooledVec(cb, pool, size) AllocVecPooled(pool, size)
#define FreePooledVec(cb, pool, mem) FreeVecPooled(pool, mem)
#endif
BOOL CMAP2Object ( struct ClassBase *cb , Object *o , UBYTE *rgb , ULONG rgbsize );
struct ColorMap *CMAP2ColorMap ( struct ClassBase *cb , struct AnimInstData *aid , UBYTE *rgb , ULONG rgbsize );
struct ColorMap *CopyColorMap ( struct ClassBase *cb , struct ColorMap *src );
void CopyBitMap( struct ClassBase *cb, struct BitMap *bm1, struct BitMap *bm2 );
struct BitMap *AllocBitMapPooled( struct ClassBase *cb, ULONG width, ULONG height, ULONG depth, APTR pool );
void ClearBitMap( struct BitMap *bm );
LONG DrawDLTA( struct ClassBase *cb, struct AnimInstData *aid, struct BitMap *prevbm, struct BitMap *bm, struct AnimHeader *ah, UBYTE *dlta, ULONG dltasize );
BOOL FreeAbleFrame( struct AnimInstData *, struct FrameNode * );
struct FrameNode *FindFrameNode( struct MinList *, ULONG );
ULONG SaveIFFAnim( struct ClassBase *cb, struct IClass *cl, Object *o, struct dtWrite *dtw );
void FreeFrameNodeResources( struct ClassBase *cb, struct MinList *fnl );
LONG LoadFrames( struct ClassBase *cb, Object *o );

/* animilbm.c */
LONG generic_unpackilbmbody(struct ClassBase *cb, struct BitMap *bm, struct BitMapHeader *bmh, UBYTE *dlta, ULONG dltasize );
/* anim1.c */
LONG generic_xorbm(struct AnimHeader *anhd, struct BitMap *bm, struct BitMap *deltabm );
/* anim2.c */
LONG generic_unpacklongdelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize );
/* anim3.c */
LONG generic_unpackshortdelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize );
/* anim4.c */
LONG generic_unpackanim4longdelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize, ULONG flags );
LONG generic_unpackanim4worddelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize, ULONG flags );
/* anim5.c */
LONG generic_unpackbytedelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize );
/* anim7.c */
LONG generic_unpackanim7longdelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize );
LONG generic_unpackanim7worddelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize );
/* anim8.c */
LONG generic_unpackanim8longdelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize );
LONG generic_unpackanim8worddelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize );
/* animj.c */
LONG generic_unpackanimjdelta(struct AnimHeader *anhd, struct ClassBase *cb, UBYTE *dlta, ULONG dltasize, struct BitMap *deltabm, struct BitMap *bm );
/* animi.c */
LONG generic_unpackanimidelta(struct AnimHeader *anhd, struct ClassBase *cb, UBYTE *dlta, ULONG dltasize, struct BitMap *deltabm, struct BitMap *bm );
