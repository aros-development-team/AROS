
#if defined(__AROS__)
#include <aros/preprocessor/variadic/cast2iptr.hpp>
#endif

/* classbase.c */
DISPATCHERFLAGS struct IClass *ObtainGIFAnimEngine ( REGA6 struct ClassBase *cb );
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
struct IClass *initClass ( struct ClassBase *cb );
DISPATCHERFLAGS ULONG Dispatch ( REGA0 struct IClass *cl , REGA2 Object *o , REGA1 Msg msg );
void OpenLogfile ( struct ClassBase *cb , struct GIFAnimInstData *gaid );
void error_printf ( struct ClassBase *cb , struct GIFAnimInstData *gaid , STRPTR format , ...);
void verbose_printf ( struct ClassBase *cb , struct GIFAnimInstData *gaid , STRPTR format , ...);

/* prefs.c */
void ReadENVPrefs ( struct ClassBase *cb , struct GIFAnimInstData *gaid , struct GIFEncoder *genc );

/* misc.c */
void IBMPC2ISOLatin1 ( STRPTR ibmpc , STRPTR isolatin1 );
BOOL CMAP2Object ( struct ClassBase *cb , Object *o , UBYTE *rgb , ULONG rgbsize );
struct ColorMap *CMAP2ColorMap ( struct ClassBase *cb , ULONG anumcolors , UBYTE *rgb , ULONG rgbsize );
struct ColorMap *CopyColorMap ( struct ClassBase *cb , struct ColorMap *src );
void WriteRGBPixelArray8 ( struct ClassBase *cb , struct BitMap *bm , ULONG animwidth , ULONG animheight , struct ColorRegister *cm , UBYTE *chunky );
void CopyBitMap ( struct ClassBase *cb , struct BitMap *dest , struct BitMap *src , ULONG width , ULONG height );
#if !defined(__AROS__)
void mysprintf ( struct ClassBase * , STRPTR buffer , STRPTR fmt , ...);
APTR AllocPooledVec ( struct ClassBase *cb , APTR pool , ULONG memsize );
void FreePooledVec ( struct ClassBase *cb , APTR pool , APTR mem );
#else
#define mysprintf(cb, buffer, fmt, ...) sprintf(buffer, fmt, __VA_ARGS__)
#define error_printf(cb, gaid, fmt, ...) \
{ \
    IPTR errargs[] = { AROS_PP_VARIADIC_CAST2IPTR(__VA_ARGS__) }; \
    OpenLogfile( (cb), (gaid) ); \
    if ((gaid) -> gaid_VerboseOutput) \
    { \
        VFPrintf( ((gaid) -> gaid_VerboseOutput), (fmt), errargs); \
    } \
}
#define verbose_printf(cb, gaid, fmt, ... ) \
{ \
    IPTR pargs[] = { AROS_PP_VARIADIC_CAST2IPTR(__VA_ARGS__) }; \
    if( ((gaid) -> gaid_VerboseOutput) && (((gaid) -> gaid_VerboseOutput) != (BPTR)-1L) ) \
    { \
        VFPrintf( ((gaid) -> gaid_VerboseOutput), (fmt), pargs); \
    } \
}
#define AllocPooledVec(cb, pool, size) AllocVecPooled(pool, size)
#define FreePooledVec(cb, pool, mem) FreeVecPooled(pool, mem)
#endif

/* encoder.c */
ULONG SaveGIFAnim ( struct ClassBase *cb , struct IClass *cl , Object *o , struct dtWrite *dtw );
