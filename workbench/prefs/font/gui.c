/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "global.h"

#include "string.h"		// strlen()

struct Gadget *textGadget;	// Pointer needed to update text gadget

// TODO: These variables shouldn't need to be global - do something about it (petah)
UWORD extraXSpace = 0;	// TODO: Do we need to set this to NULL or will the compiler to it for us? (petah)
UWORD offsetX[3];
UWORD offsetY = 0;	// TODO: Do we need to set this to NULL or will the compiler to it for us? (petah)

static struct IBox zoomCoords;

BYTE fontChoices[] = { MSG_MX_WORKBENCH, MSG_MX_SYSTEM, MSG_MX_SCREEN, NULL };
BYTE buttonStrings[] = { MSG_GAD_SAVE, MSG_GAD_USE, MSG_GAD_CANCEL, NULL };

struct NewMenu windowMenus[] =
{
 { NM_TITLE,	(STRPTR)MSG_MEN_PROJECT },
 { NM_ITEM,	(STRPTR)MSG_MEN_PROJECT_OPEN },
 { NM_ITEM,	(STRPTR)MSG_MEN_PROJECT_SAVEAS },
 { NM_ITEM,	NM_BARLABEL },
 { NM_ITEM,	(STRPTR)MSG_MEN_PROJECT_QUIT },
 { NM_TITLE,	(STRPTR)MSG_MEN_EDIT },
 { NM_ITEM,	(STRPTR)MSG_MEN_EDIT_DEFAULT },
 { NM_ITEM,	(STRPTR)MSG_MEN_EDIT_LASTSAVED },
 { NM_ITEM,	(STRPTR)MSG_MEN_EDIT_RESTORE },
 { NM_END }	// TODO: Should we use a trailing comma here? (petah)
};

UWORD calcTextWidth(UBYTE *text, struct Screen *screenPtr, struct RastPort *rastPort)
{
    struct RastPort tempRastPort;
    struct DrawInfo *drawInfo;
    UWORD width = 0;

    if(rastPort)
    {
	width = TextLength(rastPort, text, strlen(text));
    }
    else
    {
	InitRastPort(&tempRastPort); // TODO: Error check please?

	drawInfo = GetScreenDrawInfo(screenPtr); // TODO: Can't fail according to AmigaOS 3.1 Autodocs?

	SetFont(&tempRastPort, drawInfo->dri_Font);

	width = TextLength(&tempRastPort, text, strlen(text));

	FreeScreenDrawInfo(screenPtr, drawInfo);
    }

    return(width);
}

// Add gadgets to the settings tab
void addGadgets()
{
    AddGList(appGUIData->agd_Window, appGUIData->agd_FirstGadget, 0, 3, NULL);
}

// Construct the preview image
void addPreview()
{
    extern struct FontPrefs *fontPrefs[3];
    struct TextFont *bitmapFont, *oldFont;
    struct ColorMap *colorMap = appGUIData->agd_Window->WScreen->ViewPort.ColorMap;
    UBYTE a;
    UWORD bevelOffsetX = 0, bevelOffsetY = 0, offsetX = 0, offsetY = 0, tempValue = 0, previewOffsetX[3]; // Real ugly, change this (petah)
    LONG pen = 0;

    if(appGUIData->agd_RegisterTab->framewidth - GADGET_OUTER_SPACING * 2 > PREVIEW_WIDTH)
	bevelOffsetX = (appGUIData->agd_RegisterTab->framewidth - PREVIEW_WIDTH - GADGET_OUTER_SPACING * 2) / 2;

    if(appGUIData->agd_RegisterTab->frameheight - appGUIData->agd_RegisterTab->height - GADGET_OUTER_SPACING * 2 > PREVIEW_HEIGHT)
        bevelOffsetY = (appGUIData->agd_RegisterTab->framewidth - appGUIData->agd_RegisterTab->height - PREVIEW_HEIGHT -
	    GADGET_OUTER_SPACING * 2) / 2; 

    DrawBevelBox(appGUIData->agd_Window->RPort,
	appGUIData->agd_Window->WScreen->WBorLeft + WINDOW_OFFSET + GADGET_OUTER_SPACING + bevelOffsetX,
	appGUIData->agd_Window->WScreen->WBorTop + WINDOW_OFFSET + GADGET_OUTER_SPACING + bevelOffsetY +
		appGUIData->agd_Window->WScreen->Font->ta_YSize + 1 + appGUIData->agd_RegisterTab->height,
 	PREVIEW_WIDTH,
	PREVIEW_HEIGHT,
	GT_VisualInfo, appGUIData->agd_VisualInfo, GTBB_Recessed, TRUE,
	TAG_END);

    // Extra feature of "Font" Preferences: Allocate a pen for the preview background (just for the looks)
    // If ObtainBestPen() fails, it'll return -1. If this is the case, set pen to "0".
    // Todo: Will this work on m68k systems? (petah)
    // 0x91, 0x91, 0x9e = Blueish 0xea, 0xb8, 0x61 = Yellowish
    if(-1 == (pen = ObtainBestPen(colorMap, 0xeaaaaaaa, 0xb8888888, 0x61111111, OBP_Precision, PRECISION_GUI, OBP_FailIfBad, FALSE, TAG_END)))
	pen = 0;

    kprintf("Pen is %ld\n", pen);
    SetAPen(appGUIData->agd_Window->RPort, pen);

    RectFill(appGUIData->agd_Window->RPort,
	appGUIData->agd_RegisterTab->left + 1 + GADGET_OUTER_SPACING + bevelOffsetX,
	appGUIData->agd_RegisterTab->top + appGUIData->agd_RegisterTab->height + GADGET_OUTER_SPACING + 1 + bevelOffsetY,
	appGUIData->agd_Window->WScreen->WBorLeft + WINDOW_OFFSET + GADGET_OUTER_SPACING + PREVIEW_WIDTH + bevelOffsetX - 2,
	appGUIData->agd_Window->WScreen->Font->ta_YSize + 1 + WINDOW_OFFSET + GADGET_OUTER_SPACING + appGUIData->agd_RegisterTab->height +
		+ GADGET_OUTER_SPACING + bevelOffsetY + PREVIEW_HEIGHT);

    // Release pen (if we got one)
    if(pen != 0)		
	ReleasePen(colorMap, pen);

    SetAPen(appGUIData->agd_Window->RPort, 1);

    offsetX = offsetY = 0;

    for(a = 0; a <= 2; a++)
    {
	if((bitmapFont = OpenDiskFont(&(fontPrefs[a]->fp_TextAttr))))
	{
	    SetFont(appGUIData->agd_Window->RPort, bitmapFont);

	    previewOffsetX[a] = calcTextWidth(getCatalog(catalogPtr, a + MSG_MX_WORKBENCH), NULL, appGUIData->agd_Window->RPort);

	    if(previewOffsetX[a] > offsetX)
		offsetX = previewOffsetX[a];

            if(tempValue > offsetY)
		offsetY = tempValue;
	}
    }

    // Figure out the offsetX and offsetY values for the font preview
    offsetY = (PREVIEW_HEIGHT - tempValue) / 4 + bevelOffsetY;

    for(a = 0; a <= 2; a++)
	previewOffsetX[a] = ((PREVIEW_WIDTH - previewOffsetX[a]) / 2) + bevelOffsetX;

    for(a = 0; a <= 2; a++)
    {
	if((bitmapFont = OpenFont(&(fontPrefs[a]->fp_TextAttr))))
	{
	    Move(appGUIData->agd_Window->RPort,
		appGUIData->agd_Window->WScreen->WBorLeft + WINDOW_OFFSET + GADGET_OUTER_SPACING + previewOffsetX[a],
		appGUIData->agd_Window->WScreen->WBorTop + WINDOW_OFFSET + GADGET_OUTER_SPACING +
		appGUIData->agd_Window->WScreen->Font->ta_YSize + 1 + appGUIData->agd_RegisterTab->height + (offsetY * (a + 1)));

	    oldFont = appGUIData->agd_Window->RPort->Font;

	    SetFont(appGUIData->agd_Window->RPort, bitmapFont);

	    Text(appGUIData->agd_Window->RPort, getCatalog(catalogPtr, a + MSG_MX_WORKBENCH),
		strlen(getCatalog(catalogPtr, a + MSG_MX_WORKBENCH)));

	    CloseFont(bitmapFont);
	}
    }

    // Restore old font to RastPort
    SetFont(appGUIData->agd_Window->RPort, oldFont);
}

// Remove the gadgets from the settings tab
void removeGadgets()
{
    // Remove first three gadgets (MX, buttongadget and textgadget)
    RemoveGList(appGUIData->agd_Window, appGUIData->agd_FirstGadget, 3);
}

// Remove the preview tab (this function does nothing right now)
void removePreview()
{
    kprintf("removePreview() - appGUIData->agd_RegisterTab->height = %d\n",
    	appGUIData->agd_RegisterTab->height);
}

// Allocate and initialize resources needed for registertab
BOOL createRegisterTabs()
{
    if(!(appGUIData->agd_RegisterTab = AllocMem(sizeof(struct RegisterTab), (MEMF_ANY | MEMF_CLEAR))))
    {
	printf("Unable to allocate memory for registerTab!\n");
	return(FALSE);
    }

    // TODO: Initializing several entries of an array can probably be done in a better way? (petah)
    appGUIData->agd_RegisterTabItem[0].text = getCatalog(catalogPtr, MSG_TAB_SETTINGS); //titleSettings;
    appGUIData->agd_RegisterTabItem[0].image = NULL;

    appGUIData->agd_RegisterTabItem[1].text = getCatalog(catalogPtr, MSG_TAB_PREVIEW); //titlePreview;
    appGUIData->agd_RegisterTabItem[1].image = NULL;

    appGUIData->agd_RegisterTabItem[2].text = NULL; // NULL terminator

    InitRegisterTab(appGUIData->agd_RegisterTab, &(appGUIData->agd_RegisterTabItem[0]));

    LayoutRegisterTab(appGUIData->agd_RegisterTab, appGUIData->agd_Screen, appGUIData->agd_DrawInfo, TRUE); // "TRUE" sets the same tab width all over

    return(TRUE);
}

void freeRegisterTabs()
{
    kprintf("freeRegisterTab() called\n");
    if(appGUIData->agd_RegisterTab)
	FreeMem(appGUIData->agd_RegisterTab, sizeof(struct RegisterTab));
    else
	printf("WARNING: No memory for RegisterTab!\n"); // TODO: Fix this!
}

// Deal with the registertab (remove/add contents to old/new page chosen by user)
void processRegisterTab()
{
    // Check whether or not the user has actually requested to change registertabs
    if(appGUIData->agd_OldActive != appGUIData->agd_RegisterTab->active)
    {
	// Remove/free any resources allocated by the previously used registertab frame
	kprintf("appGUIData->agd_RegisterTab->active = %d\n", appGUIData->agd_RegisterTab->active);
	kprintf("Calling removeGUIItems(%d)... ", appGUIData->agd_OldActive);
	appGUIData->removeGUIItems[appGUIData->agd_OldActive](appGUIData);
	kprintf("OK!\n\n");

	// Setup the proper drawing mode to erase the contents of the registertab frame
	// Todo: Investigate the mode currently being used (petah)
	SetDrMd(appGUIData->agd_Window->RPort, JAM1),
	SetAPen(appGUIData->agd_Window->RPort, 0);

	// Remove whatever has been rendered into the registertab frame. The one and two
	// pixel offsets used below are compensations for the frame border width
	RectFill(appGUIData->agd_Window->RPort,
	    appGUIData->agd_RegisterTab->left + 1,
	    appGUIData->agd_RegisterTab->top + appGUIData->agd_RegisterTab->height,
	    appGUIData->agd_RegisterTab->left + appGUIData->agd_RegisterTab->framewidth - 2,
	    appGUIData->agd_RegisterTab->top + appGUIData->agd_RegisterTab->height +
		appGUIData->agd_RegisterTab->frameheight - 2);

	// Set "agd_OldActive" to previously used registertab
	appGUIData->agd_OldActive = appGUIData->agd_RegisterTab->active;

	// Add/allocate any resources allocated by the previously used registertab frame
	// WARNING: This is currently NOT set free by the code flow as of now! (petah)
	kprintf("Calling addGUIItems(%d)... ", appGUIData->agd_RegisterTab->active);
	appGUIData->addGUIItems[appGUIData->agd_RegisterTab->active](appGUIData);
	kprintf("OK!\n\n");

	// Refresh all gadgets ("-1"). Todo: What order should these calls be placed in? (petah)
	RefreshGList(appGUIData->agd_Window->FirstGadget, appGUIData->agd_Window, NULL, -1);
	GT_RefreshWindow(appGUIData->agd_Window, NULL);
    }
    else
	kprintf("processRegisterTab() - no action taken!\n");

}

// setupMenus() - expects an APTR VisualInfo pointer - returns a menu pointer
// if successful, otherwise FALSE
struct Menu *setupMenus()
{
    struct Menu *menuPtr = NULL; // Menu pointer; better set it to NULL, just in case
    ULONG id;

    struct NewMenu *newMenuPtr = windowMenus;

    while(newMenuPtr->nm_Type != NM_END)
    {
	if(newMenuPtr->nm_Label != NM_BARLABEL)
	{
	    id = (ULONG)newMenuPtr->nm_Label;

	    if(newMenuPtr->nm_Type == NM_TITLE)
		newMenuPtr->nm_Label = getCatalog(catalogPtr, id);
	    else
	    {
		newMenuPtr->nm_Label = getCatalog(catalogPtr, id) + 2; // CAUTION: Menus will be crippled if they lack keyboard shortcuts!
		newMenuPtr->nm_CommKey = getCatalog(catalogPtr, id);
		newMenuPtr->nm_UserData = id;
	    }
	}

	newMenuPtr++;
    }

    if((menuPtr = CreateMenus(windowMenus, TAG_END)))
    {
	if(LayoutMenusA(menuPtr, appGUIData->agd_VisualInfo, TAG_END))
	    return(menuPtr);
	else
	{
	    displayError(getCatalog(catalogPtr, MSG_CANT_LAYOUT_MENUS));
	    FreeMenus(menuPtr);
	    return(FALSE);
	}
    }
    else
    {
	displayError(getCatalog(catalogPtr, MSG_CANT_CREATE_MENUS));
	return(FALSE);
    }
}

void updateGUI(UBYTE fontChoice)
{
    extern struct FontPrefs *fontPrefs[3];  // Declared in init.c
    static UBYTE tmpString[64]; // TODO: "Static"? What if this buffer is exceeded? (petah)

    sprintf(tmpString, "%s %d", fontPrefs[fontChoice]->fp_Name, fontPrefs[fontChoice]->fp_TextAttr.ta_YSize);

    GT_SetGadgetAttrs(textGadget, appGUIData->agd_Window, NULL, GTTX_Text, tmpString, TAG_DONE);
}

UWORD preCalculateGUI(struct AppGUIData *appGUIData)
{
    UBYTE a;
    UWORD width = 0, maxWidth = 0, frameHeight = 0;

    // First step: The MX Gadget. Check what MX item is the widest.
    for(a = 0; a <= 2; a++)
    {
	width = calcTextWidth(getCatalog(catalogPtr, fontChoices[a]), appGUIData->agd_Screen, NULL);

	if(width > maxWidth)
	    maxWidth = width;
    }

    offsetX[0] =  maxWidth + MX_WIDTH; // MX_WIDTH is the (static) width of the MX button glyph. Change this?

    // Second step: The "Get" button + spacing to Text string gadget
    offsetX[1] = calcTextWidth(getCatalog(catalogPtr, MSG_GAD_GET), appGUIData->agd_Screen, NULL) +
	GADGET_INNER_SPACING * 2 + GADGET_OUTER_SPACING * 2;

    // Third step: The "Button row" Element.
    maxWidth = 0;

    for(a = 0; a <= 2; a++)
    {
	width = calcTextWidth(getCatalog(catalogPtr, buttonStrings[a]), appGUIData->agd_Screen, NULL);

	if(width > maxWidth)
	    maxWidth = width;
    }

    offsetX[2] = (maxWidth + GADGET_INNER_SPACING * 2) * 3 + (GADGET_OUTER_SPACING * 2) * 2;

    maxWidth = 0;

    // Fourth step: See what element is the widest
    for(a = 0; a <= 2; a++)
	if(offsetX[a] > maxWidth)
	    maxWidth = offsetX[a];

    // Fifth step: Calculate and assign element offsets
    for(a = 0; a <= 2; a++)
	if(offsetX[a] != maxWidth)
	    offsetX[a] = (maxWidth - offsetX[a]) / 2;
	else
	    offsetX[a] = 0;

    // Sixth step: Check whether or not we need to add space to "maxWidth" if the "registertab" is too narrow
    if(appGUIData->agd_RegisterTab->width > maxWidth)
    {
	// It appears as if 'C' can allocate new variables on the fly, just
	// like this - that's pretty neat!
	UWORD oldMaxWidth = maxWidth;

	maxWidth = maxWidth + (appGUIData->agd_RegisterTab->width - maxWidth);

	// Don't take offsetX[2] into account (Save, Use, Cancel-buttons) as these are independent of the RegisterTab layout!
	for(a = 0; a <= 1; a++)
         offsetX[a] += (maxWidth - oldMaxWidth) / 2;
    }

    // Seventh step: See if any extra space is needed between gadgets (on both axis)
    frameHeight = GADGET_OUTER_SPACING + appGUIData->agd_Screen->Font->ta_YSize * 3 + 2 + GADGET_OUTER_SPACING * 2 +
	appGUIData->agd_Screen->Font->ta_YSize + GADGET_INNER_SPACING * 2 + GADGET_OUTER_SPACING;

    // If "frameHeight" is too narrow for the preview picture to fit, calculate an "offsetY" value.
    // Note that we compare "frameHeight" with the PREVIEW_HEIGTH constant + two times the gadget outer
    // spacing value as we need this extra space in the "Preview" registertab.
    if(frameHeight < PREVIEW_HEIGHT + 2 * GADGET_OUTER_SPACING)
    {
	offsetY = (PREVIEW_HEIGHT + 2 * GADGET_OUTER_SPACING - frameHeight) / 2;
	kprintf("offsetY = %ld", offsetY);
    }
    else
	kprintf("Sorry, these are the values: %ld %ld", PREVIEW_HEIGHT + 2 * GADGET_OUTER_SPACING, frameHeight); 

    if(maxWidth + 2 * GADGET_OUTER_SPACING < PREVIEW_HEIGHT + 2 * GADGET_OUTER_SPACING)
    {
	kprintf("\n\nWhatis %ld\n", PREVIEW_WIDTH + 2 * GADGET_OUTER_SPACING);
	kprintf("Whatis %ld\n", maxWidth + 2 * GADGET_OUTER_SPACING);
	extraXSpace = (PREVIEW_WIDTH + 2 * GADGET_OUTER_SPACING - (maxWidth + GADGET_OUTER_SPACING * 2)) / 2;
	kprintf("test = %ld\n", ((206 - (maxWidth + 6)) / 2));
	kprintf("extraXSpace = %ld\n", extraXSpace);

	maxWidth += extraXSpace * 2;
    }

    kprintf("\nmaxWidth = %ld\n", maxWidth);

    kprintf("frameHeight = %ld\n", frameHeight);

    return(maxWidth);
}

// MX-Gadget, Button + Text, Color + Color, Button + Button + Button = 8 gadgets
// setupGadgets() - if failure, this function returns FALSE
//
struct AppGUIData * createGadgets()
{
    UBYTE a = 0, b = 0;
    UWORD width = 0, maxWidth = 0;
    struct Gadget *gList = NULL;   // Gadget list; better set it to NULL, just in case
    struct Gadget *gadget;
    struct NewGadget newGadget;
    static STRPTR mxStrings[4]; // TODO: Do we need to retain this array? Look it up!

    for(a = 0; a <= 2; a++)
	mxStrings[a] = getCatalog(catalogPtr, fontChoices[a]);

    mxStrings[3] = NULL;

    appGUIData->agd_MaxWidth = preCalculateGUI(appGUIData);

    if((gadget = CreateContext(&gList)))
    {
	// These Gadget structure members should be common for all gadgets:
	newGadget.ng_TextAttr = appGUIData->agd_Screen->Font;
	newGadget.ng_VisualInfo = appGUIData->agd_VisualInfo;

	// Create MX Gadget
	newGadget.ng_GadgetID =	MX_CHOOSEFONT;
	newGadget.ng_TopEdge =	WINDOW_OFFSET + GADGET_OUTER_SPACING + appGUIData->agd_Screen->WBorTop + appGUIData->agd_Screen->Font->ta_YSize + 1 + appGUIData->agd_RegisterTab->height + offsetY;
	newGadget.ng_LeftEdge = WINDOW_OFFSET + GADGET_INNER_SPACING + appGUIData->agd_Screen->WBorLeft + offsetX[0] + extraXSpace;
	newGadget.ng_Flags =	NULL;

	if(!(gadget = CreateGadget(MX_KIND, gadget, &newGadget, GTMX_Labels, mxStrings, GTMX_Active, MX_INIT_ACTIVE, TAG_END)))
	    return(FALSE);

	appGUIData->agd_FirstGadget = gadget;

	// Create Font "Get" button Gadget (offsetX disabled - we want this element as wide as the widest element)
	newGadget.ng_GadgetID =	BUTTON_GETFONT;
	newGadget.ng_TopEdge =	newGadget.ng_TopEdge + gadget->Height + GADGET_OUTER_SPACING * 2 + offsetY;
	newGadget.ng_LeftEdge = WINDOW_OFFSET + GADGET_INNER_SPACING + appGUIData->agd_Screen->WBorLeft;
	newGadget.ng_Height = 	appGUIData->agd_Screen->Font->ta_YSize + GADGET_INNER_SPACING * 2;
	newGadget.ng_Width =	calcTextWidth(getCatalog(catalogPtr, MSG_GAD_GET), appGUIData->agd_Screen, NULL) + GADGET_INNER_SPACING * 2;
	newGadget.ng_GadgetText = getCatalog(catalogPtr, MSG_GAD_GET);
	newGadget.ng_Flags =	NULL;

	if(!(gadget = CreateGadget(BUTTON_KIND, gadget, &newGadget, TAG_END)))
	    return(FALSE);

	// Create Font Text Gadget (offsetX disabled - we want this element as wide as the widest element)
	newGadget.ng_GadgetID =       GADGET_ID_TEXT;
	newGadget.ng_LeftEdge =       WINDOW_OFFSET + GADGET_INNER_SPACING + appGUIData->agd_Screen->WBorLeft + gadget->Width + GADGET_OUTER_SPACING * 2;
	newGadget.ng_Height =         appGUIData->agd_Screen->Font->ta_YSize + GADGET_INNER_SPACING * 2;
	newGadget.ng_Width =          appGUIData->agd_MaxWidth - newGadget.ng_Width - GADGET_OUTER_SPACING * 2; // CHANGE THIS!!!
	newGadget.ng_Flags =          NULL;

	if(!(gadget = CreateGadget(TEXT_KIND, gadget, &newGadget, GTTX_Border, TRUE, TAG_END)))
	    return(FALSE);

	// Right now, we need to save this pointer for updating its text later on
	textGadget = gadget;

	appGUIData->agd_BevelBoxOffset = newGadget.ng_TopEdge + newGadget.ng_Height - appGUIData->agd_RegisterTab->height -
	    (appGUIData->agd_Screen->WBorTop + appGUIData->agd_Screen->Font->ta_YSize + 1) - WINDOW_OFFSET + GADGET_OUTER_SPACING;

	// Create three buttons ("Save", "Use" and "Cancel") of the same width. Calculate the width first!
	a = 0;

	while(buttonStrings[a])
	{
	    width = calcTextWidth(getCatalog(catalogPtr, buttonStrings[a]), appGUIData->agd_Screen, NULL);

	    if(width > maxWidth)	// Nicer way of testing and assigning width data?
		maxWidth = width;

	    a++;
	}

	// Common gadget data:
	newGadget.ng_TopEdge =	newGadget.ng_TopEdge + gadget->Height + GADGET_OUTER_SPACING * 2 + 1; // "+ 1" takes the register tab border into account
	newGadget.ng_Height =	appGUIData->agd_Screen->Font->ta_YSize + GADGET_INNER_SPACING * 2;
	newGadget.ng_Width =	maxWidth + GADGET_INNER_SPACING * 2;
	newGadget.ng_Flags =	NULL;

	// Create three gadgets ("Save", "Use", "Cancel") of the same width.
        // First, calculate how much space (stored in 'b') we can put between the gadgets!

	b = (appGUIData->agd_MaxWidth - (3 * newGadget.ng_Width)) / 2;

	for(a = 0; a <= 2; a++)
	{
	    newGadget.ng_GadgetID =	a + BUTTON_SAVE;
	    newGadget.ng_LeftEdge =	WINDOW_OFFSET + appGUIData->agd_Screen->WBorLeft + (newGadget.ng_Width * a) + (b * a) + GADGET_OUTER_SPACING * a + offsetX[2];
	    newGadget.ng_GadgetText = 	getCatalog(catalogPtr, buttonStrings[a]);

	    if(!(gadget = CreateGadget(BUTTON_KIND, gadget, &newGadget, TAG_END)))
		return(FALSE);
	}

    }
    else
    {
	printf("CreateContext() failed!\n");
	return(FALSE);
    }

    appGUIData->agd_MaxHeight = newGadget.ng_TopEdge + newGadget.ng_Height + WINDOW_OFFSET + appGUIData->agd_Screen->WBorBottom;
    appGUIData->agd_GadgetList = gList;

    return(appGUIData);
}

struct AppGUIData * openAppWindow()
{
    zoomCoords.Left = -1; // Starting with Intuition V39, -1 tells AmigaOS/AROS to keep the window X/Y coordinates while zoomed
    zoomCoords.Top = -1;
    zoomCoords.Width = appGUIData->agd_MaxWidth + 2 * WINDOW_OFFSET + appGUIData->agd_Screen->WBorLeft + appGUIData->agd_Screen->WBorRight + GADGET_OUTER_SPACING * 2;
    zoomCoords.Height = appGUIData->agd_Screen->WBorTop + appGUIData->agd_Screen->Font->ta_YSize + 1;

    kprintf("%d %d %d %d\n", zoomCoords.Left, zoomCoords.Top, zoomCoords.Width, zoomCoords.Height);

    appGUIData->agd_Window = OpenWindowTags(NULL,
    	WA_Left,	appGUIData->agd_Screen->MouseX,
	WA_Top,		appGUIData->agd_Screen->MouseY,
	WA_Width,	appGUIData->agd_MaxWidth + 2 * WINDOW_OFFSET + appGUIData->agd_Screen->WBorLeft + appGUIData->agd_Screen->WBorRight + GADGET_OUTER_SPACING * 2,
	WA_Height,	appGUIData->agd_MaxHeight,
	WA_DragBar,	TRUE,
	WA_CloseGadget,	TRUE,
	WA_Zoom,	(IPTR)&zoomCoords,
	WA_DepthGadget,	TRUE,
	WA_Activate,	TRUE,
	WA_Gadgets,	appGUIData->agd_GadgetList,
	WA_Title,	getCatalog(catalogPtr, MSG_WINTITLE),
	WA_PubScreen,	appGUIData->agd_Screen,
	WA_IDCMP,	IDCMP_CLOSEWINDOW | IDCMP_REFRESHWINDOW | IDCMP_VANILLAKEY |
			IDCMP_MOUSEBUTTONS | IDCMP_MENUPICK | BUTTONIDCMP | MXIDCMP,
	TAG_END);

    if(appGUIData->agd_Window)
    {
	UnlockPubScreen(NULL, appGUIData->agd_Screen); // The window itself now holds a lock on the screen

	kprintf("Window width = %ld\n", appGUIData->agd_Window->Width);

	// Setup RegisterTab position and size
	SetRegisterTabPos(appGUIData->agd_RegisterTab, appGUIData->agd_Screen->WBorLeft + WINDOW_OFFSET,
            appGUIData->agd_Screen->WBorTop + appGUIData->agd_Screen->Font->ta_YSize + 1 + WINDOW_OFFSET);

	SetRegisterTabFrameSize(appGUIData->agd_RegisterTab, appGUIData->agd_MaxWidth + GADGET_OUTER_SPACING * 2,
	    appGUIData->agd_BevelBoxOffset);

	kprintf("Real frameheight = %ld\n", appGUIData->agd_RegisterTab->frameheight);
	kprintf("Real frameWidth = %ld", appGUIData->agd_RegisterTab->framewidth);

	// Render the RegisterTab
	RenderRegisterTab(appGUIData->agd_Window->RPort, appGUIData->agd_RegisterTab, TRUE);

	GT_RefreshWindow(appGUIData->agd_Window, NULL); // TODO: What does this do? Look it up! (petah)

	updateGUI(MX_INIT_ACTIVE);

	if(!(appGUIData->agd_Menu = setupMenus()))
	    return(NULL); // No menus, no application
	else
	    SetMenuStrip(appGUIData->agd_Window, appGUIData->agd_Menu);

	return(appGUIData);
    }

    return(NULL); // "else" statement left out to keep GCC quiet ("control reaches end of non-void function")
}
