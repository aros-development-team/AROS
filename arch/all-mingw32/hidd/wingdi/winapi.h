/* WinAPI definitions to be used with AROS-side code. Taken from various Mingw32 headers. */

#ifdef __x86_64__
#define __stdcall __attribute__((ms_abi))
#else
#define __stdcall __attribute__((stdcall))
#endif

#define BLACKNESS	0x00000042
#define NOTSRCERASE	0x001100A6
#define NOTSRCCOPY	0x00330008
#define SRCERASE	0x00440328
#define DSTINVERT	0x00550009
#define PATINVERT	0x005A0049
#define SRCINVERT	0x00660046
#define SRCAND		0x008800C6
#define MERGEPAINT	0x00BB0226
#define MERGECOPY	0x00C000CA
#define SRCCOPY		0x00CC0020
#define SRCPAINT	0x00EE0086
#define PATCOPY		0x00F00021
#define PATPAINT	0x00FB0A09
#define WHITENESS	0x00FF0062
#define CAPTUREBLT	0x40000000
#define NOMIRRORBITMAP	0x80000000

#define R2_BLACK	1
#define R2_COPYPEN	13
#define R2_MASKNOTPEN	3
#define R2_MASKPEN	9
#define R2_MASKPENNOT	5
#define R2_MERGENOTPEN	12
#define R2_MERGEPEN	15
#define R2_MERGEPENNOT	14
#define R2_NOP	11
#define R2_NOT	6
#define R2_NOTCOPYPEN	4
#define R2_NOTMASKPEN	8
#define R2_NOTMERGEPEN	2
#define R2_NOTXORPEN	10
#define R2_WHITE	16
#define R2_XORPEN	7

#define RDW_ERASE	    4
#define RDW_FRAME	    1024
#define RDW_INTERNALPAINT   2
#define RDW_INVALIDATE	    1
#define RDW_NOERASE	    32
#define RDW_NOFRAME	    2048
#define RDW_NOINTERNALPAINT 16
#define RDW_VALIDATE	    8
#define RDW_ERASENOW	    512
#define RDW_UPDATENOW	    256
#define RDW_ALLCHILDREN	    128
#define RDW_NOCHILDREN	    64

#define SWP_NOSIZE	   0x0001
#define SWP_NOMOVE	   0x0002
#define SWP_NOZORDER	   0x0004
#define SWP_NOREDRAW	   0x0008
#define SWP_NOACTIVATE	   0x0010
#define SWP_DRAWFRAME	   0x0020
#define SWP_FRAMECHANGED   0x0020
#define SWP_SHOWWINDOW	   0x0040
#define SWP_HIDEWINDOW	   0x0080
#define SWP_NOCOPYBITS	   0x0100
#define SWP_NOOWNERZORDER  0x0200
#define SWP_NOREPOSITION   0x0200
#define SWP_NOSENDCHANGING 0x0400
#define SWP_DEFERERASE	   0x2000
#define SWP_ASYNCWINDOWPOS 0x4000

#define DRIVERVERSION 0
#define TECHNOLOGY 2
#define DT_PLOTTER 0
#define DT_RASDISPLAY 1
#define DT_RASPRINTER 2
#define DT_RASCAMERA 3
#define DT_CHARSTREAM 4
#define DT_METAFILE 5
#define DT_DISPFILE 6
#define HORZSIZE 4
#define VERTSIZE 6
#define HORZRES 8
#define VERTRES 10
#define LOGPIXELSX 88
#define LOGPIXELSY 90
#define BITSPIXEL 12
#define PLANES 14
#define NUMBRUSHES 16
#define NUMPENS 18
#define NUMFONTS 22
#define NUMCOLORS 24
#define NUMMARKERS 20
#define ASPECTX 40
#define ASPECTY 42
#define ASPECTXY 44
#define PDEVICESIZE 26
#define CLIPCAPS 36
#define SIZEPALETTE 104
#define NUMRESERVED 106
#define COLORRES 108
#define PHYSICALWIDTH 110
#define PHYSICALHEIGHT 111
#define PHYSICALOFFSETX 112
#define PHYSICALOFFSETY 113
#define SCALINGFACTORX 114
#define SCALINGFACTORY 115
#define VREFRESH 116
#define DESKTOPHORZRES 118
#define DESKTOPVERTRES 117
#define BLTALIGNMENT 119
#define SHADEBLENDCAPS 120
#define SB_NONE 0x00
#define SB_CONST_ALPHA 0x01
#define SB_PIXEL_ALPHA 0x02
#define SB_PREMULT_ALPHA 0x04
#define SB_GRAD_RECT 0x10
#define SB_GRAD_TRI 0x20
#define COLORMGMTCAPS 121
#define CM_NONE 0x00
#define CM_DEVICE_ICM 0x01
#define CM_GAMMA_RAMP 0x02
#define CM_CMYK_COLOR 0x04
#define RASTERCAPS 38
#define RC_BANDING 2
#define RC_BITBLT 1
#define RC_BITMAP64 8
#define RC_DI_BITMAP 128
#define RC_DIBTODEV 512
#define RC_FLOODFILL 4096
#define RC_GDI20_OUTPUT 16
#define RC_PALETTE 256
#define RC_SCALING 4
#define RC_STRETCHBLT 2048
#define RC_STRETCHDIB 8192
#define RC_DEVBITS 0x8000
#define RC_OP_DX_OUTPUT 0x4000
#define CURVECAPS 28
#define CC_NONE 0
#define CC_CIRCLES 1
#define CC_PIE 2
#define CC_CHORD 4
#define CC_ELLIPSES 8
#define CC_WIDE 16
#define CC_STYLED 32
#define CC_WIDESTYLED 64
#define CC_INTERIORS 128
#define CC_ROUNDRECT 256
#define LINECAPS 30
#define LC_NONE 0
#define LC_POLYLINE 2
#define LC_MARKER 4
#define LC_POLYMARKER 8
#define LC_WIDE 16
#define LC_STYLED 32
#define LC_WIDESTYLED 64
#define LC_INTERIORS 128
#define POLYGONALCAPS 32
#define RC_BANDING 2
#define RC_BIGFONT 1024
#define RC_BITBLT 1
#define RC_BITMAP64 8
#define RC_DEVBITS 0x8000
#define RC_DI_BITMAP 128
#define RC_GDI20_OUTPUT 16
#define RC_GDI20_STATE 32
#define RC_NONE 0
#define RC_OP_DX_OUTPUT 0x4000
#define RC_PALETTE 256
#define RC_SAVEBITMAP 64
#define RC_SCALING 4
#define PC_NONE 0
#define PC_POLYGON 1
#define PC_POLYPOLYGON 256
#define PC_PATHS 512
#define PC_RECTANGLE 2
#define PC_WINDPOLYGON 4
#define PC_SCANLINE 8
#define PC_TRAPEZOID 4
#define PC_WIDE 16
#define PC_STYLED 32
#define PC_WIDESTYLED 64
#define PC_INTERIORS 128
#define PC_PATHS 512
#define TEXTCAPS 34
#define TC_OP_CHARACTER 1
#define TC_OP_STROKE 2
#define TC_CP_STROKE 4
#define TC_CR_90 8
#define TC_CR_ANY 16
#define TC_SF_X_YINDEP 32
#define TC_SA_DOUBLE 64
#define TC_SA_INTEGER 128
#define TC_SA_CONTIN 256
#define TC_EA_DOUBLE 512
#define TC_IA_ABLE 1024
#define TC_UA_ABLE 2048
#define TC_SO_ABLE 4096
#define TC_RA_ABLE 8192
#define TC_VA_ABLE 16384
#define TC_RESERVED 32768
#define TC_SCROLLBLT 65536

#define WM_CLOSE      16
#define WM_QUIT	      18
#define WM_KEYDOWN    256
#define WM_KEYUP      257
#define WM_SYSKEYDOWN 260
#define WM_SYSKEYUP   261
#define WM_MOUSEWHEEL 522

#define MK_LBUTTON	1
#define MK_RBUTTON	2
#define MK_SHIFT	4
#define MK_CONTROL	8
#define MK_MBUTTON	16
#define MK_XBUTTON1	32
#define MK_XBUTTON2	64

#define VK_LWIN 0x5B
#define VK_RWIN 0x5C

#define WA_INACTIVE 0

#define KF_EXTENDED 0x0100
#define LLKHF_EXTENDED 0x01

#define BI_RGB 0

#define DIB_RGB_COLORS	0
#define DIB_PAL_COLORS	1

typedef struct tagRECT { 
  LONG left; 
  LONG top; 
  LONG right; 
  LONG bottom; 
} RECT;

typedef struct tagBITMAPINFOHEADER{
  ULONG  biSize; 
  LONG   biWidth; 
  LONG   biHeight; 
  WORD   biPlanes; 
  WORD   biBitCount; 
  ULONG  biCompression; 
  ULONG  biSizeImage; 
  LONG   biXPelsPerMeter; 
  LONG   biYPelsPerMeter; 
  ULONG  biClrUsed; 
  ULONG  biClrImportant; 
} BITMAPINFOHEADER, *PBITMAPINFOHEADER; 

typedef struct tagRGBQUAD {
  BYTE    rgbBlue; 
  BYTE    rgbGreen; 
  BYTE    rgbRed; 
  BYTE    rgbReserved; 
} RGBQUAD;

typedef struct tagBITMAP {
  ULONG   bmType;
  ULONG   bmWidth; 
  ULONG   bmHeight; 
  ULONG   bmWidthBytes; 
  UWORD   bmPlanes; 
  UWORD   bmBitsPixel; 
  APTR bmBits; 
} BITMAP, *PBITMAP; 

typedef struct tagBITMAPINFO { 
  BITMAPINFOHEADER bmiHeader; 
  RGBQUAD          bmiColors[1];
} BITMAPINFO, *PBITMAPINFO;

typedef struct _ICONINFO {
    ULONG fIcon;
    ULONG xHotspot;
    ULONG yHotspot;
    APTR hbmMask;
    APTR hbmColor;
} ICONINFO;
