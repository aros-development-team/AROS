/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/alib.h>			// CreateGadget()
#include <proto/dos.h>
#include <proto/diskfont.h>
#include <proto/intuition.h>		// Registertab
#include <proto/graphics.h>		// Registertab
#include <proto/gadtools.h>
#include <proto/arossupport.h>		// kprintf()
#include <exec/memory.h>
#include <intuition/imageclass.h>	// Registertab

#include <libraries/gadtools.h>
#include <libraries/locale.h>

#include <dos/dos.h>
#include <prefs/font.h>

#include <stdio.h>

#include "registertab.h"		// Needed for AppGUIData struct below

// Defines/Include needed for locale support
#define CATCOMP_ARRAY
#include "fontprefs_strings.h"
#define REQ_CAT_VERSION 2		// Catalog version required

// Constants for the "registertab" code
#define REGISTER_TABS 3			// Number of tabs - 1 (last tab for NULL termination)
#define PREVIEW_WIDTH 180		// Width of font preview
#define PREVIEW_HEIGHT 180		// Height of font preview
#define REGTAB_SETTINGS 0		// The "Settings" tab ID number
#define REGTAB_PREVIEW 1		// The "Preview" tab ID number

// Global variables
extern struct AppGUIData *appGUIData;
extern struct Catalog *catalogPtr;

struct AppGUIData
{
 UWORD			agd_MaxWidth;
 UWORD			agd_MaxHeight;
 UWORD			agd_BevelBoxOffset;
 APTR			agd_VisualInfo;
 struct Screen		*agd_Screen;
 struct Window		*agd_Window;
 struct DrawInfo	*agd_DrawInfo;
 struct Menu		*agd_Menu;
 struct Gadget		*agd_GadgetList;
 struct Gadget		*agd_FirstGadget;
 struct RegisterTab	*agd_RegisterTab;
 struct RegisterTabItem	agd_RegisterTabItem[REGISTER_TABS];
 UBYTE			agd_OldActive;
 void (*addGUIItems[REGISTER_TABS - 1])(struct AppGUIData *);
 void (*removeGUIItems[REGISTER_TABS - 1])(struct AppGUIData *);
};

// Defines needed by ReadArgs() processing
#define ARG_FROM 0
#define ARG_EDIT 1
#define ARG_USE 2
#define ARG_SAVE 3
#define ARG_PUBSCREEN 4
#define NUM_ARGS 5

#define APP_RUN		0
#define APP_STOP	1
#define APP_FAIL	2

// GUI constants
#define WINDOW_OFFSET		3

#define MX_INIT_ACTIVE		0 // Default MX choice

#define GADGET_INNER_SPACING	3 // Should be proportional to font size?
#define GADGET_OUTER_SPACING	3 // Should be proportional to font size?

#define MX_CHOOSEFONT		0
#define BUTTON_GETFONT		1
#define BUTTON_SAVE		2
#define BUTTON_USE		3
#define BUTTON_CANCEL		4
#define GADGET_ID_TEXT		5

// Function declarations (see source code for explanations on what each function does)

// main.c
STRPTR getCatalog(struct Catalog *, ULONG);
STRPTR formatErrorMsg(STRPTR);
void displayError(STRPTR);
BOOL initLibs(void);

// inputloop.c
UBYTE inputLoop(void);

// gui.c
void processRegisterTab(void);
BOOL createRegisterTabs(void);
void freeRegisterTabs(void);
struct AppGUIData * createGadgets(void);
struct AppGUIData * openAppWindow(void);
void updateGUI(UBYTE fontChoice);
BOOL getFont(struct FontPrefs *);
void addGadgets(void);
void addPreview(void);
void removeGadgets(void);
void removePreview(void);
struct Menu * setupMenus(void);

// args.c
struct RDArgs * getArguments(void);
UBYTE processArguments(void);

// handleiff.c
BOOL readIFF(UBYTE *, struct FontPrefs **);
BOOL writeIFF(UBYTE *, struct FontPrefs **);

// init.c
BOOL initPrefMem(void);
void initDefaultPrefs(struct FontPrefs **);
struct AppGUIData * initAppGUIDataMem(struct Catalog *);

// asl.c
STRPTR aslOpenPrefs(void);
STRPTR aslSavePrefs(void);
