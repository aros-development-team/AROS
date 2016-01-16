
/* classbase.c */
DISPATCHERFLAGS struct IClass *ObtainAnimEngine ( REGA6 struct ClassBase *cb );
#if !defined(__AROS__)
DISPATCHERFLAGS struct Library *LibInit ( REGD0 struct ClassBase *cb , REGA0 BPTR seglist , REGA6 struct ExecBase *sysbase );
DISPATCHERFLAGS LONG LibOpen ( REGA6 struct ClassBase *cb );
DISPATCHERFLAGS LONG LibClose ( REGA6 struct ClassBase *cb );
DISPATCHERFLAGS LONG LibExpunge ( REGA6 struct ClassBase *cb );
unpack_ilbm_t unpackilbmbody;
unpack_deltabm_t unpackanimidelta;
unpack_deltabm_t unpackanimjdelta;
unpack_delta_t unpacklongdelta;
unpack_delta_t unpackshortdelta;
unpack_delta_t unpackbytedelta;
#if defined(COMMENTED_OUT)
unpack_delta4_t unpackanim4longdelta;
unpack_delta4_t unpackanim4worddelta;
#endif
unpack_delta_t unpackanim7longdelta;
unpack_delta_t unpackanim7worddelta;
unpack_delta_t unpackanim8longdelta;
unpack_delta_t unpackanim8worddelta;

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

/* anim.c */
LONG generic_unpackilbmbody(struct ClassBase *cb, struct BitMap *bm, struct BitMapHeader *bmh, UBYTE *dlta, ULONG dltasize );
LONG generic_unpackanimidelta(struct AnimHeader *anhd, struct ClassBase *cb, UBYTE *dlta, ULONG dltasize, struct BitMap *deltabm, struct BitMap *bm );
LONG generic_unpackanimjdelta(struct AnimHeader *anhd, struct ClassBase *cb, UBYTE *dlta, ULONG dltasize, struct BitMap *deltabm, struct BitMap *bm );
LONG generic_unpacklongdelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize );
LONG generic_unpackshortdelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize );
LONG generic_unpackbytedelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize );
#if defined(COMMENTED_OUT)
LONG generic_unpackanim4longdelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize, ULONG flags );
LONG generic_unpackanim4worddelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize, ULONG flags );
#endif
LONG generic_unpackanim7longdelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize );
LONG generic_unpackanim7worddelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize );
LONG generic_unpackanim8longdelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize );
LONG generic_unpackanim8worddelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize );
