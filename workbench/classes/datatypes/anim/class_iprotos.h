
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

#if !defined(__AROS__)
void error_printf ( struct ClassBase *cb , struct AnimInstData *gaid , STRPTR format , ...);
void verbose_printf ( struct ClassBase *cb , struct AnimInstData *gaid , STRPTR format , ...);
void mysprintf ( struct ClassBase *cb , STRPTR buffer , STRPTR fmt , ...);
APTR AllocPooledVec ( struct ClassBase *cb , APTR pool , ULONG memsize );
void FreePooledVec ( struct ClassBase *cb , APTR pool , APTR mem );
#else
#define error_printf(cb, gaid, format, ...) bug(format, __VA_ARGS__)
#define verbose_printf(cb , gaid, format, ...)
#define mysprintf(cb,buffer,fmt,...) sprintf(buffer,fmt, __VA_ARGS__)
#define AllocPooledVec(cb, pool, size) AllocVecPooled(pool, size)
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

/* ????? */

LONG LoadILBMBody( struct ClassBase *cb, struct BitMap *bm, struct BitMapHeader *bmh, UBYTE *dlta, ULONG dltasize );
LONG unpackanimidelta(struct AnimHeader *anhd, struct ClassBase *cb, UBYTE *dlta, ULONG dltasize, struct BitMap *deltabm, struct BitMap *bm );
LONG unpackanimjdelta(struct AnimHeader *anhd, struct ClassBase *cb, UBYTE *dlta, ULONG dltasize, struct BitMap *deltabm, struct BitMap *bm );
LONG unpacklongdelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize );
LONG unpackshortdelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize );
LONG unpackbytedelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize );
#if defined(COMMENTED_OUT)
LONG unpackanim4longdelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize, ULONG flags );
LONG unpackanim4worddelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize, ULONG flags );
#endif
LONG unpackanim7longdelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize );
LONG unpackanim7worddelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize );
LONG unpackanim8longdelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize );
LONG unpackanim8worddelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize );
