//
// Menu Prefs
//

#ifndef PM_PREFS_H
#define PM_PREFS_H

/// Pens
#define PM_TEXTPEN      0x0002
#define PM_SHINEPEN     0x0003
#define PM_SHADOWPEN        0x0004
#define PM_FILLPEN      0x0005
#define PM_FILLTEXTPEN      0x0006
#define PM_BACKGROUNDPEN    0x0007
#define PM_HIGHLIGHTTEXTPEN 0x0008  // MENU TITLE
#define PM_MENUTEXT     0x0009  // OUTLINE/SHADOW
#define PM_MENUFILL     0x000A
#define PM_MENUTRIM     0x000B
#define PM_HALFSHINE        0x000C
#define PM_HALFSHADOW       0x000D

#define MAX_PENS        0x000E
///

typedef struct VPTR { union { ULONG pos; STRPTR ptr }; } VPTR;

/// PM_RGB
struct PM_RGB {
    ULONG   Red;
    ULONG   Green;
    ULONG   Blue;
};
///

/// MenuPrefs
struct MenuPrefs {
    ULONG           ID;
    UWORD           Version;
    UWORD           Revision;

    BOOL            OBSOLETE1;

    ULONG           Subtime;    // Delay for submenus
    BOOL            SameHeight; // All items same height
    BOOL            OldStyle;   // Old Look
    UBYTE           RecSel;     // >1 = Recess Selected, 0 = no selborder, 1 = raised
    BOOL            WideSelBar; // Wide Select Bar
    BOOL            Compact;    // Smallest possible
    BOOL            WideMenu;   // Make the menu wider

    BOOL            UseDriPens; // Use DrawInfo pens
    struct PM_RGB       Colors[MAX_PENS];

    /* Version 2.00 */

    BOOL            OpenOnDown; // Open menu when the mouse button is pressed

    /* Version 3.00 */

    ULONG           SubMenuImg; //
    ULONG           CheckImg;   //
    ULONG           ExcludeImg; //

    /* Version 4.00 */

    BOOL            Shadows;    // Menu shadows

    /* Version 5.00 */

    BOOL            UseWindows; // Use windows instead of blitter

    /* Version 6.00 */

    ULONG           ColorPrecision;

    /* Version 7.00 */

    UWORD           MenuBorder; // Menu Border

    /* Version 8.00 */

    ULONG           AmigaImg;   // AmigaKey Image

    /* Version 10.00 */

    UBYTE           ImgName[48];    // Name of ImgTable

    LONG            ImgTable[150];  // Seek pos to images - struct size

    /* Version 11.00 */

    BOOL            Sticky;

    /* Version 12.00 */

    UWORD           TitlePatch; // Title text style
    BOOL            RealShadows;    // Enable/Disable hi-/truecolor shadows

    UBYTE           SelBarX;    // Spacing between selbar (pm->left/top)
    UBYTE           SelBarY;    // and text/image

    UBYTE           XSpacing;   // Spacing between borders and items
    UBYTE           YSpacing;   //

    UWORD           TextPatch;  // Item text style
    
    BOOL            WinBar;     // Pulldown at window bar
    BYTE            Popup;      // Pop up pulldowns
    
    BOOL            BlitImg;    // Blit images

    /* Version 13.00 */

    UBYTE           Intermediate;   // Spacing between items
    BYTE            TextDisplace;   // Text displacement
    BYTE            ShadeR;     //
    BYTE            ShadeG;     // Shadow intensity
    BYTE            ShadeB;     //

    UBYTE           Reserved[16];
};
///

/// PrefsImage
struct PrefsImage {
	WORD	LeftEdge;			// 0
	WORD	TopEdge;			// 2
	WORD	Width;				// 4
	WORD	Height;				// 6

	VPTR	Pattern;			// 8
	VPTR	Tag;				// C
	VPTR	UserData;			// 10

	UBYTE	Type;				// 14
	UBYTE	Flags;				// 15
	UWORD	Res;				// 16

	union {
		struct PMPI_Chunky {
			VPTR	SelData;
			VPTR	UnSelData;
		} Chunky;
		struct PMPI_TrueColour {
			VPTR	SelData;
			VPTR	UnSelData;
			ULONG	ChromaKey;	
		} TrueColor;
	};
};
///

/// Images
#define CHECKMARK_IMG   0
#define RIGHTARROW_IMG  1
#define MXIMAGE_IMG 2
#define CHECKIMAGE_IMG  3
#define AMIGAKEY_IMG    4
#define BULLET_A_IMG    5
#define BULLET_B_IMG    6
#define ARROW_A_IMG 7
#define ARROW_B_IMG 8
#define ARROW_C_IMG 9
#define MMCHECK_IMG 10
#define MMAMIGA_IMG 11
#define MMSUB_IMG   12
#define MMEXCLUDE_IMG   13

#define PREFSIMAGE_IMG  100 // Add to actual img number
///

/// Frames
#define BUTTON_FRAME        0
#define MAGIC_FRAME     1
#define THICK_BUTTON_FRAME  2
#define DOUBLE_FRAME        3
#define DROPBOX_FRAME       4
#define INTUI_FRAME     5
///

/// TextPatch
#define TP_CENTER       0x0001
#define TP_UNDERLINE        0x0002
#define TP_BOLD         0x0004
#define TP_SHINE        0x0008
#define TP_SHADOW       0x0010
#define TP_TEXT         0x0020
#define TP_HILITE       0x0040
#define TP_SHADOWED     0x0080
#define TP_LEFT         0x0100
#define TP_RIGHT        0x0200
#define TP_EMBOSS       0x0400
#define TP_KILLBAR      0x0800
#define TP_OUTLINE      0x1000
#define TP_ACTIVATE     0x8000
///

/// File name/ID
#define PMP_ID      (0x504d4e55)
#define PMP_PATH    "ENV:PopupMenu.cfg"
#define PMP_S_PATH  "ENVARC:PopupMenu.cfg"
///

#endif
