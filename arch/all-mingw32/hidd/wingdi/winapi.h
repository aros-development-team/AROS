/* WinAPI definitions to be used with AROS-side code. Taken from various Mingw32 headers. */

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

#define VK_LBUTTON		1
#define VK_RBUTTON		2
#define VK_CANCEL		3
#define VK_MBUTTON		4
#define VK_XBUTTON1		5
#define VK_XBUTTON2		6
#define VK_BACK			8
#define VK_TAB			9
#define VK_CLEAR		12
#define VK_RETURN		13
#define VK_SHIFT		16
#define VK_CONTROL		17
#define VK_MENU			18
#define VK_PAUSE		19
#define VK_CAPITAL		20
#define VK_KANA			0x15
#define VK_HANGEUL		0x15
#define VK_HANGUL		0x15
#define VK_JUNJA		0x17
#define VK_FINAL		0x18
#define VK_HANJA		0x19
#define VK_KANJI		0x19
#define VK_ESCAPE		0x1B
#define VK_CONVERT		0x1C
#define VK_NONCONVERT		0x1D
#define VK_ACCEPT		0x1E
#define VK_MODECHANGE		0x1F
#define VK_SPACE		32
#define VK_PRIOR		33
#define VK_NEXT			34
#define VK_END			35
#define VK_HOME			36
#define VK_LEFT			37
#define VK_UP			38
#define VK_RIGHT		39
#define VK_DOWN			40
#define VK_SELECT		41
#define VK_PRINT		42
#define VK_EXECUTE		43
#define VK_SNAPSHOT		44
#define VK_INSERT		45
#define VK_DELETE		46
#define VK_HELP			47
#define VK_LWIN			0x5B
#define VK_RWIN			0x5C
#define VK_APPS			0x5D
#define VK_SLEEP		0x5F
#define VK_NUMPAD0		0x60
#define VK_NUMPAD1		0x61
#define VK_NUMPAD2		0x62
#define VK_NUMPAD3		0x63
#define VK_NUMPAD4		0x64
#define VK_NUMPAD5		0x65
#define VK_NUMPAD6		0x66
#define VK_NUMPAD7		0x67
#define VK_NUMPAD8		0x68
#define VK_NUMPAD9		0x69
#define VK_MULTIPLY		0x6A
#define VK_ADD			0x6B
#define VK_SEPARATOR		0x6C
#define VK_SUBTRACT		0x6D
#define VK_DECIMAL		0x6E
#define VK_DIVIDE		0x6F
#define VK_F1			0x70
#define VK_F2			0x71
#define VK_F3			0x72
#define VK_F4			0x73
#define VK_F5			0x74
#define VK_F6			0x75
#define VK_F7			0x76
#define VK_F8			0x77
#define VK_F9			0x78
#define VK_F10			0x79
#define VK_F11			0x7A
#define VK_F12			0x7B
#define VK_F13			0x7C
#define VK_F14			0x7D
#define VK_F15			0x7E
#define VK_F16			0x7F
#define VK_F17			0x80
#define VK_F18			0x81
#define VK_F19			0x82
#define VK_F20			0x83
#define VK_F21			0x84
#define VK_F22			0x85
#define VK_F23			0x86
#define VK_F24			0x87
#define VK_NUMLOCK		0x90
#define VK_SCROLL		0x91
#define VK_LSHIFT		0xA0
#define VK_RSHIFT		0xA1
#define VK_LCONTROL		0xA2
#define VK_RCONTROL		0xA3
#define VK_LMENU		0xA4
#define VK_RMENU		0xA5
#define VK_BROWSER_BACK		0xA6
#define VK_BROWSER_FORWARD	0xA7
#define VK_BROWSER_REFRESH	0xA8
#define VK_BROWSER_STOP		0xA9
#define VK_BROWSER_SEARCH	0xAA
#define VK_BROWSER_FAVORITES	0xAB
#define VK_BROWSER_HOME		0xAC
#define VK_VOLUME_MUTE		0xAD
#define VK_VOLUME_DOWN		0xAE
#define VK_VOLUME_UP		0xAF
#define VK_MEDIA_NEXT_TRACK	0xB0
#define VK_MEDIA_PREV_TRACK	0xB1
#define VK_MEDIA_STOP		0xB2
#define VK_MEDIA_PLAY_PAUSE	0xB3
#define VK_LAUNCH_MAIL		0xB4
#define VK_LAUNCH_MEDIA_SELECT	0xB5
#define VK_LAUNCH_APP1		0xB6
#define VK_LAUNCH_APP2		0xB7
#define VK_OEM_1		0xBA
#define VK_OEM_PLUS		0xBB
#define VK_OEM_COMMA		0xBC
#define VK_OEM_MINUS		0xBD
#define VK_OEM_PERIOD		0xBE
#define VK_OEM_2		0xBF
#define VK_OEM_3		0xC0
#define VK_OEM_4		0xDB
#define VK_OEM_5		0xDC
#define VK_OEM_6		0xDD
#define VK_OEM_7		0xDE
#define VK_OEM_8		0xDF
#define VK_OEM_102		0xE2
#define VK_PROCESSKEY		0xE5
#define VK_PACKET		0xE7
#define VK_ATTN			0xF6
#define VK_CRSEL		0xF7
#define VK_EXSEL		0xF8
#define VK_EREOF		0xF9
#define VK_PLAY			0xFA
#define VK_ZOOM			0xFB
#define VK_NONAME		0xFC
#define VK_PA1			0xFD
#define VK_OEM_CLEAR		0xFE

#define BI_RGB 0

#define DIB_RGB_COLORS	0

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

typedef struct tagBITMAPINFO { 
  BITMAPINFOHEADER bmiHeader; 
  RGBQUAD          bmiColors[1];
} BITMAPINFO, *PBITMAPINFO;
