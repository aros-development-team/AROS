#include <proto/alib.h>		// CreateGadget() and stuff
#include <proto/exec.h>		// Wait()
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/graphics.h>

#include <dos/dos.h>		// SIGBREAKF_CTRL_C

#include <prefs/font.h>		// Structure declaration

#include <libraries/gadtools.h>

#include <stdio.h>

#include "global.h"

extern struct Library *GadToolsBase;
extern struct GfxBase *GfxBase;

struct Gadget *textGadget;

extern struct Menu * setupMenus(APTR);
extern STRPTR aslOpenPrefs(void);
extern STRPTR aslSavePrefs(void);
extern BOOL getFont(struct FontPrefs *);
extern void readIFF(UBYTE *, struct FontPrefs **);
extern void writeIFF(UBYTE *, struct FontPrefs **);
extern void initDefaultPrefs(struct FontPrefs **);

UWORD offsetX[3];
UWORD appWinWidth;	// Determinates the final window width. Not pretty - change this!

STRPTR fontChoices[] = { "Workbench", "System", "Screen", NULL };
STRPTR buttonStrings[] = { "Save", "Use", "Cancel", NULL };

struct NewMenu windowMenus[] =
{
 { NM_TITLE,	"Project",		  0, 0, 0, 0, },
 { NM_ITEM,	"Open...",		"O", 0, 0, MENU_ID_OPEN, },
 { NM_ITEM,	"Save As...",		"A", 0, 0, MENU_ID_SAVE, },
 { NM_ITEM,	NM_BARLABEL,		  0, 0, 0, 0, },
 { NM_ITEM,	"Quit",			"Q", 0, 0, MENU_ID_QUIT, },
 { NM_TITLE,	"Edit",			  0, 0, 0, 0, },
 { NM_ITEM,	"Reset to Defaults",	"D", 0, 0, MENU_ID_DEFAULTS, },
 { NM_ITEM,	"Last Saved",		"L", 0, 0, MENU_ID_LASTSAVED, },
 { NM_ITEM,	"Restore",		"R", 0, 0, MENU_ID_RESTORE, },
 { NM_END,	NULL,			  0, 0, 0, 0, },
};

/* setupMenus() - expects an APTR VisualInfo pointer - returns a menu pointer
   if successful, otherwise FALSE */
struct Menu *setupMenus(APTR visualInfo)
{
 struct Menu *menuPtr = NULL; // Menu pointer; better set it to NULL, just in case

 if(menuPtr = CreateMenus(windowMenus, TAG_END))
 {
  if(LayoutMenusA(menuPtr, visualInfo, TAG_END))
   return(menuPtr);
  else
  {
   printf("Unable to layout menus!\n");
   FreeMenus(menuPtr);
   return(FALSE);
  }
 }
 else
 {
  printf("Unable to create menus!\n");
  return(FALSE);
 }
}

void updateGUI(struct AppGUIData *appGUIData, UBYTE fontChoice)
{
 extern struct FontPrefs *fontPrefs[3];  /* init.c */

 GT_SetGadgetAttrs(textGadget, appGUIData->agd_Window, NULL, GTTX_Text, fontPrefs[fontChoice]->fp_Name, TAG_DONE);

 printf("updateGUI | fontChoice is %d\n", fontChoice);
 printf("fontPrefs->fp_Name is >%s<\n", fontPrefs[fontChoice]->fp_Name);
 printf("fontPrefs->fp_TextAttr.ta_YSize is >%d<\n", fontPrefs[fontChoice]->fp_TextAttr.ta_YSize);
}


UWORD calcTextWidth(UBYTE *text, struct Screen *screenPtr)
{
 struct RastPort tempRastPort;
 struct DrawInfo *drawInfo;
 UWORD width = 0;

 InitRastPort(&tempRastPort); // Error check please?

 drawInfo = GetScreenDrawInfo(screenPtr); // Can't fail according to AmigaOS 3.1 Autodocs?

 SetFont(&tempRastPort, drawInfo->dri_Font);

 width = TextLength(&tempRastPort, text, strlen(text));

 FreeScreenDrawInfo(screenPtr, drawInfo);

 return(width);
}

UWORD preCalculateGUI(struct Screen *publicScreen)
{
 UBYTE a;
 UWORD width = 0, maxWidth = 0;

 // First step: The MX Gadget. Check what MX item is the widest.
 for(a = 0; a <= 2; a++)
 {
  width = calcTextWidth(fontChoices[a], publicScreen);

  if(width > maxWidth)
   maxWidth = width;
 }

 offsetX[0] =  maxWidth + MX_WIDTH; // MX_WIDTH is the (static) width of the MX button glyph. Change this?

 // Second step: The "Get" button + spacing to Text string gadget
 offsetX[1] = calcTextWidth("Get...", publicScreen) + GADGET_INNER_SPACING * 2 + GADGET_OUTER_SPACING * 2;

 // Third step: The "Button row" Element.
 maxWidth = 0;

 for(a = 0; a <= 2; a++)
 {
  width = calcTextWidth(buttonStrings[a], publicScreen);

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

 return(maxWidth);
}

// MX-Gadget, Button + Text, Color + Color, Button + Button + Button = 8 gadgets
// setupGadgets() - if failure, this function returns FALSE
//
struct AppGUIData * createGadgets(struct Screen *publicScreen, APTR visualInfo)
{
 UBYTE a = 0;
 UWORD width = 0, maxWidth = 0;
 struct AppGUIData *appGUIData;
 struct Gadget *gList = NULL;   // Gadget list; better set it to NULL, just in case
 struct Gadget *gadget;
 struct NewGadget newGadget;

 appGUIData->agd_MaxWidth = preCalculateGUI(publicScreen);

 if(gadget = CreateContext(&gList))
 {
  // These Gadget structure members should be common for all gadgets:
  newGadget.ng_TextAttr = publicScreen->Font;
  newGadget.ng_VisualInfo = visualInfo;

  // Create MX Gadget
  newGadget.ng_GadgetID =	MX_CHOOSEFONT;
  newGadget.ng_TopEdge =	WINDOW_OFFSET + publicScreen->WBorTop + publicScreen->Font->ta_YSize + 1;
  newGadget.ng_LeftEdge = 	WINDOW_OFFSET + publicScreen->WBorLeft + offsetX[0];
  newGadget.ng_Flags =		NULL;

  if(!(gadget = CreateGadget(MX_KIND, gadget, &newGadget, GTMX_Labels, fontChoices, GTMX_Active, MX_INIT_ACTIVE, TAG_END)))
   return(FALSE);

  // Create Font "Get" button Gadget (offsetX disabled - we want this element as wide as the widest element)
  newGadget.ng_GadgetID =	BUTTON_GETFONT;
  newGadget.ng_TopEdge =	newGadget.ng_TopEdge + gadget->Height + GADGET_OUTER_SPACING * 2;
  newGadget.ng_LeftEdge = 	WINDOW_OFFSET + publicScreen->WBorLeft/* + offsetX[1];*/;
  newGadget.ng_Height = 	publicScreen->Font->ta_YSize + GADGET_INNER_SPACING * 2;
  newGadget.ng_Width =		calcTextWidth("Get...", publicScreen) + GADGET_INNER_SPACING * 2;
  newGadget.ng_GadgetText =	"Get...";
  newGadget.ng_Flags =		NULL;

  if(!(gadget = CreateGadget(BUTTON_KIND, gadget, &newGadget, TAG_END)))
   return(FALSE);

  // Create Font Text Gadget (offsetX disabled - we want this element as wide as the widest element)
  newGadget.ng_GadgetID =       GADGET_ID_TEXT;
  newGadget.ng_LeftEdge =       WINDOW_OFFSET + publicScreen->WBorLeft + gadget->Width + GADGET_OUTER_SPACING * 2; /*+ offsetX[1];*/
  newGadget.ng_Height =         publicScreen->Font->ta_YSize + GADGET_INNER_SPACING * 2;
  newGadget.ng_Width =          appGUIData->agd_MaxWidth - newGadget.ng_Width - GADGET_OUTER_SPACING * 2; // CHANGE THIS!!!
  newGadget.ng_Flags =          NULL;

  if(!(gadget = CreateGadget(TEXT_KIND, gadget, &newGadget, GTTX_Border, TRUE, TAG_END)))
   return(FALSE);

  // Right now, we need to save this pointer for updating its text later on
  textGadget = gadget;

/*
  // Create Palette Gadget (foreground)

  // Create Palette Gadget (background)
*/

  appGUIData->agd_BevelBoxOffset = newGadget.ng_TopEdge + newGadget.ng_Height + GADGET_OUTER_SPACING * 2;

  // Create three buttons ("Save", "Use" and "Cancel") of the same width. Calculate the width first!

  while(buttonStrings[a])
  {
   width = calcTextWidth(buttonStrings[a], publicScreen);

   if(width > maxWidth)	// Nicer way of testing and assigning width data?
    maxWidth = width;

   a++;
  }

  // Common gadget data:
  newGadget.ng_TopEdge =        newGadget.ng_TopEdge + gadget->Height + GADGET_OUTER_SPACING * 2;
  newGadget.ng_Height =         publicScreen->Font->ta_YSize + GADGET_INNER_SPACING * 2;
  newGadget.ng_Width =          maxWidth + GADGET_INNER_SPACING * 2;
  newGadget.ng_Flags =		NULL;

  // Create three gadgets
  for(a = 0; a <= 2; a++)
  {
   newGadget.ng_GadgetID =	a + BUTTON_SAVE;
   newGadget.ng_LeftEdge =	WINDOW_OFFSET + publicScreen->WBorLeft + (newGadget.ng_Width * a) + (a * GADGET_OUTER_SPACING * 2) + offsetX[2];
   newGadget.ng_GadgetText = 	buttonStrings[a];

   if(!(gadget = CreateGadget(BUTTON_KIND, gadget, &newGadget, TAG_END)))
    return(FALSE);
  }

 }
 else
 {
  printf("CreateContext() failed!\n");
  return(FALSE);
 }

 appGUIData->agd_MaxHeight = newGadget.ng_TopEdge + newGadget.ng_Height + WINDOW_OFFSET + publicScreen->WBorBottom;
 appGUIData->agd_GadgetList = gList;

 return(appGUIData);
}

struct AppGUIData * openAppWindow(struct Screen *publicScreen, struct AppGUIData *appGUIData, APTR drawInfo)
{
 appGUIData->agd_Window = OpenWindowTags(NULL,	WA_Left,	10,	// Change this! (To what?)
						WA_Top,		10,	// Change this! (To what?)
						WA_Width,	appGUIData->agd_MaxWidth + 2 * WINDOW_OFFSET + publicScreen->WBorLeft + publicScreen->WBorRight,
						WA_Height,	appGUIData->agd_MaxHeight,
						WA_DragBar,	TRUE,
						WA_CloseGadget,	TRUE,
						// Add WA_Zoom <something>
						WA_DepthGadget,	TRUE,
						WA_Activate,	TRUE,
						WA_Gadgets,	appGUIData->agd_GadgetList,
						WA_Title,	"Font Preferences",
						WA_PubScreen,	publicScreen,
						WA_IDCMP,	IDCMP_CLOSEWINDOW | IDCMP_REFRESHWINDOW |
								IDCMP_MENUPICK | BUTTONIDCMP | MXIDCMP,
						TAG_END);

 if(appGUIData->agd_Window)
 {
  UnlockPubScreen(NULL, publicScreen);	// The window itself now holds a lock on the screen

  GT_RefreshWindow(appGUIData->agd_Window, NULL); // What does this do?

  updateGUI(appGUIData, MX_INIT_ACTIVE);

  // This is all crazy, alter the code flow!
  if(!(appGUIData->agd_Menu = setupMenus(drawInfo)))
  {
   printf("setupMenus() failed!\n");
   return(NULL); // No menus, no application
  }
  else
   SetMenuStrip(appGUIData->agd_Window, appGUIData->agd_Menu);

  return(appGUIData);
 }

 return(NULL); // "else" statement left out to keep GCC quiet ("control reaches end of non-void function")
}

void inputLoop(struct AppGUIData *appGUIData)
{
 ULONG signals, class;
 UWORD code, menuNumber;
 UBYTE fontChoice = 0; /* The BYTE holding what font we're editing right now */
 BOOL running = TRUE;
 STRPTR fileName;
 struct MenuItem *menuItem;
 struct IntuiMessage *intuiMessage;
 struct Gadget *gadget;
 extern struct FontPrefs *fontPrefs[3];  /* init.c */

 while(running)
 {
  signals = Wait(1L << appGUIData->agd_Window->UserPort->mp_SigBit | SIGBREAKF_CTRL_C);

  if(signals & (1L << appGUIData->agd_Window->UserPort->mp_SigBit))
  {
   // We got a IDCMP message, analyze it and take action

   while(NULL != (intuiMessage = GT_GetIMsg(appGUIData->agd_Window->UserPort)))
   {
    class = intuiMessage->Class;
    code = intuiMessage->Code;
    menuNumber = intuiMessage->Code;
    gadget = (struct Gadget *)intuiMessage->IAddress;

    GT_ReplyIMsg(intuiMessage);

    switch(class)
    {
     case IDCMP_CLOSEWINDOW :
      running = FALSE;
     break;

     case IDCMP_MENUPICK :
      printf("menupick\n");

      // Make sure to process every single menu choice made. RKRM: Libraries p. 176
      while(menuNumber != MENUNULL)
      {
       menuItem = ItemAddress(appGUIData->agd_Menu, menuNumber);

       printf("%d chosen!\n", (UBYTE)GTMENUITEM_USERDATA(menuItem));

       // The output from GTMENUITEM_USERDATA must be casted
       switch((UBYTE)GTMENUITEM_USERDATA(menuItem))
       {
        case MENU_ID_OPEN :
         if(fileName = aslOpenPrefs())
         {
          printf("reading %s...\n", fileName);
          readIFF(fileName, fontPrefs);
          updateGUI(appGUIData, fontChoice); /* For the moment, always update the GUI */
         }
        break;

        case MENU_ID_SAVE :
         if(fileName = aslSavePrefs())
         {
          printf("saving %s...\n", fileName);
          writeIFF(fileName, fontPrefs);
         }
        break;

        case MENU_ID_QUIT :
         printf("Menu quit!\n");
         running = FALSE;
        break;

        case MENU_ID_DEFAULTS :
         printf("Menu defaults!\n");
         initDefaultPrefs(fontPrefs);
         updateGUI(appGUIData, fontChoice); /* For the moment, always update the GUI */
        break;

        case MENU_ID_LASTSAVED :
         printf("Last saved!\n");
         readIFF("ENVARC:sys/font.prefs", fontPrefs);
         updateGUI(appGUIData, fontChoice); /* For the moment, always update the GUI */
        break;

        case MENU_ID_RESTORE :
         printf("Menu restore!\n");
         readIFF("ENV:sys/font.prefs", fontPrefs);
         updateGUI(appGUIData, fontChoice); /* For the moment, always update the GUI */
        break;

        default :
         printf("Warning: Unknown menu item!\n");
        break;
       }

       menuNumber = menuItem->NextSelect;
      }
     break;

     case IDCMP_GADGETUP :
      switch(gadget->GadgetID)
      {
       case BUTTON_GETFONT :
        printf("getfont\n");
        
        if(getFont(fontPrefs[fontChoice]))
         updateGUI(appGUIData, fontChoice);
       break;

       case BUTTON_SAVE :
        printf("save\n");
        writeIFF("ENV:sys/font.prefs", fontPrefs);
        writeIFF("ENVARC:sys/font.prefs", fontPrefs);
        running = FALSE;
       break;

       case BUTTON_USE :
        printf("use\n");
        writeIFF("ENV:sys/font.prefs", fontPrefs);
        running = FALSE;
       break;

       case BUTTON_CANCEL :
        printf("cancel\n");
        running = FALSE;
       break;

       default :
        printf("Warning: Unknown button pressed!\n");
       break;
      }
     break;

     case IDCMP_GADGETDOWN : // This message applies for the "MX" gadget
      printf("IDCMP_GADGETDOWN!\n");

      switch(gadget->GadgetID)
      {
       case MX_CHOOSEFONT :
        if(code != fontChoice) /* No reason to update GUI if nothing has changed */
        {
         fontChoice = code;
         updateGUI(appGUIData, fontChoice);
        }
       break;

       default :
        printf("Warning: Unknown kind of IDCMP_GADGETDOWN message!\n");
       break;
      }
     break;

     case IDCMP_REFRESHWINDOW :
      printf("IDCMP_REFRESHWINDOW - not implemented yet!\n");
     break;

     default :
      printf("Unknown IDCMP message!\n");
     break;
    }
   }
  }
  if(signals & SIGBREAKF_CTRL_C)
  {
   printf("CTRL-C signal detected!\n");
   running = FALSE;
  }
 }
}
