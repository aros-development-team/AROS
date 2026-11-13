
/*  Bitmap header chunk (BMHD) structure  */
struct BitMapHeader {
	UWORD	w, h;		/*  Width, height in pixels */
	WORD	x, y;		/*  x, y position for this bitmap  */
	UBYTE	nplanes;	/*  # of planes  */
	UBYTE	Masking;
	UBYTE	Compression;	/*  Compression flag */
	UBYTE	pad1;
	UWORD	TransparentColor;
	UBYTE	XAspect, YAspect;   /*	Aspect ratios */
	WORD	PageWidth, PageHeight;
};

struct SHAM
{
   UWORD *colorBuf;
};

/* IFF chunk types */
#define ID_ILBM    MAKE_ID('I', 'L', 'B', 'M')
#define ID_BMHD    MAKE_ID('B', 'M', 'H', 'D')
#define ID_CMAP    MAKE_ID('C', 'M', 'A', 'P')
#define ID_BODY    MAKE_ID('B', 'O', 'D', 'Y')
#define ID_CAMG    MAKE_ID('C', 'A', 'M', 'G')
#define ID_CRNG    MAKE_ID('C', 'R', 'N', 'G')
#define ID_DRNG    MAKE_ID('D', 'R', 'N', 'G')
#define ID_SHAM    MAKE_ID('S', 'H', 'A', 'M')
#define ID_CTBL    MAKE_ID('C', 'T', 'B', 'L')

