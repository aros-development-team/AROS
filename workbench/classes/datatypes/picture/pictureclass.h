/*
    Copyright ©	1995-2003, The AROS Development	Team. All rights reserved.
    $Id$
*/

#define	MIN(a,b) (((a) < (b)) ?	(a) : (b))
#define	MAX(a,b) (((a) > (b)) ?	(a) : (b))

struct Picture_Data
{
    /*
     *	public entries,	accessible with	SET/GET
     */
    ULONG		  Precision;
    ULONG		  ModeID;
    struct BitMapHeader	  bmhd;
    struct BitMap	 *SrcBM; /* PDTA_BitMap	and PDTA_ClassBM */
    ULONG		  SrcColRegs[768]; /* PDTA_CRegs */
    struct BitMap	 *DestBM;
    UBYTE		 *MaskPlane;
    ULONG		  DestColRegs[768]; /* PDTA_GRegs */
    struct Screen	 *DestScreen;
    struct ColorRegister  ColMap[256];
    UBYTE		  ColTable[256];
    UWORD		  NumColors;
    UWORD		  NumAlloc;
    UBYTE		  SparseTable[256];
    UWORD		  NumSparse;
    Point		  Grab;
    UWORD		  MaxDitherPens;
    UWORD		  DitherQuality;
    UWORD		  ScaleQuality;
    BOOL		  FreeSource;
    BOOL		  Remap;
    BOOL		  UseFriendBM;
    BOOL		  DestMode;
    BOOL		  DelayRead;
    BOOL		  DelayedRead;
    /*
     *	private	entries
     */
    UBYTE		  *SrcBuffer;
    ULONG		  SrcWidth;
    ULONG		  SrcWidthBytes;
    ULONG		  SrcHeight;
    LONG		  SrcPixelFormat;
    UWORD		  SrcPixelBytes;
    UWORD		  SrcDepth;
  
    ULONG		  DestWidth;
    ULONG		  DestHeight;
    ULONG		  ColTableXRGB[256];
    UWORD		  DestDepth;
  
    BOOL		  TrueColorSrc;
    BOOL		  TrueColorDest;
    BOOL		  Layouted;
    BOOL		  UseAsImage;
    BOOL		  KeepSrcBM;
    BOOL		  NoDelay;

    BOOL		  Scale;
    ULONG		  XScale;
    ULONG		  YScale;

    LONG		  ClickX;
    LONG		  ClickY;
};
