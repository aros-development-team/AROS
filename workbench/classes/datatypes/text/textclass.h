#ifdef __cplusplus

extern "C"
{
#endif

APTR Text_Create(void);
VOID Text_SetFrameBox( APTR mem, struct Screen *scr, struct RastPort *rp, LONG left, LONG top, LONG width, LONG height);
VOID Text_Load(APTR mem, STRPTR);
VOID Text_ChangeDimension( APTR mem, LONG left, LONG top, LONG width, LONG height);
VOID Text_Redraw( APTR mem );
VOID Text_Free(APTR mem);
ULONG Text_PageHeight( APTR mem );
ULONG Text_PageWidth( APTR mem );
ULONG Text_VisibleHeight( APTR mem );
ULONG Text_VisibleTop( APTR mem );
ULONG Text_VisibleHoriz( APTR mem );
VOID Text_SetVisibleTop( APTR mem, ULONG newy );
VOID Text_SetVisibleLeft( APTR mem, ULONG newx );
VOID Text_HandleMouse( APTR mem, LONG x, LONG y, LONG code, ULONG secs, ULONG mics);
VOID Text_Print( APTR mem );

#ifdef __cplusplus
}
#endif
