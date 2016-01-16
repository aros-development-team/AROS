
extern BOOL ReadOK( struct ClassBase *cb, struct GIFDecoder *gifdec, void *buffer, ULONG len );
extern int ReadImage( struct ClassBase *cb, struct GIFAnimInstData *gaid, UBYTE *image,
               UWORD imagewidth, UWORD left, UWORD top, UWORD len, UWORD height,
               BOOL interlace, BOOL ignore, UWORD transparent );
extern void WriteDeltaPixelArray8Fast( struct BitMap *dest, UBYTE *source, UBYTE *prev );
extern BOOL ScanFrames( struct ClassBase *cb, Object *o );
extern struct FrameNode *FindFrameNode( struct MinList *fnl, ULONG timestamp );
extern struct FrameNode *GetPrevFrameNode( struct FrameNode *currfn, ULONG interleave );
extern void FreeFrameNodeResources( struct ClassBase *cb, struct GIFAnimInstData *gaid );
extern struct BitMap *AllocFrameBitMap( struct ClassBase *cb, struct GIFAnimInstData *gaid );
extern void FreeFrameBitMap( struct ClassBase *cb, struct GIFAnimInstData *gaid, struct BitMap *bm );
