#include <graphics/gfxbase.h>
#include <intuition/intuitionbase.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h> /* TextLength() */
#include <proto/alib.h> /* CreateGadget() */
#include <proto/iffparse.h>
#include <proto/gadtools.h>
#include <proto/dos.h> /* FreeArgs() */

#include <libraries/gadtools.h> /* Not included automatically by other includes? */
#include <libraries/iffparse.h> /* IFFHandle structure */

#include <dos/dos.h> /* Return codes */

#include <prefs/font.h>

#include <exec/types.h>
#include <exec/memory.h>

#include <stdio.h>

#include "global.h"

#define GADGET_INNER_SPACING 3 /* Should be proportional to font size? */
#define GADGET_OUTER_SPACING 3 /* Should be proportional to font size? */
#define WINDOW_OFFSET 3 /* Should be proportional to font size? */

#define MX_CHOOSEFONT 0
#define BUTTON_GETFONT 1
#define BUTTON_SAVE 2
#define BUTTON_USE 3
#define BUTTON_CANCEL 4

#define MX_INIT_ACTIVE 0 /* MX button that is active at startup */

UBYTE version[] = "$VER: Font 0.5 (5.4.2001)";

struct IntuitionBase *IntuitionBase;
struct Library *GadToolsBase;
struct GfxBase *GfxBase;
struct Library *IFFParseBase;

struct Window *prefsWindow;
struct Screen *publicScreen;

struct Gadget *gadgetList = NULL, *gad, *textGadget;
struct NewGadget newGadget;

struct Menu *winMenus;

struct RDArgs *readArgs;

extern struct FontPrefs *fontPrefs[3]; /* init.c */
extern struct IFFHandle *iffHandle; /* handleiff.c */

IPTR argArray[NUM_ARGS]; /* args.c */

STRPTR fontChoices[] = { "Workbench", "System", "Screen", NULL };
STRPTR buttons[] = { "Save", "Use", "Cancel", NULL };
void *visualInfo;
UWORD offsetX[3]; /* This implementation is getting obsolete! Change this! */

extern struct Menu * setupMenus(APTR);
extern struct RDArgs * getArguments(void);
extern STRPTR aslOpenPrefs(void);
extern STRPTR aslSavePrefs(void);
extern BOOL getFont(struct FontPrefs *);
extern void readIFF(UBYTE *, struct FontPrefs **);
extern void writeIFF(UBYTE *, struct FontPrefs **);
extern void initPrefMem(void);

void updateGUI(UBYTE);
void openWindow(UWORD, UWORD, struct Gadget *);

void quitApp(UBYTE *errorMsg, UBYTE errorCode)
{
 UBYTE a;

 if(errorMsg)
  printf("%s\n", errorMsg);

 /* We don't have to check for a TRUE value. However, there is no reason
    why not to - it might turn out useful in the future. */
 if(readArgs)
  FreeArgs(readArgs);

 if(iffHandle)
  FreeIFF(iffHandle);

 for(a = 0; a <= 2; a++)
  if(fontPrefs[a])
   FreeMem(fontPrefs[a], sizeof(struct FontPrefs));

 if(IFFParseBase)
  CloseLibrary(IFFParseBase);

 if(GfxBase)
  CloseLibrary((struct Library *)GfxBase);

 if(GadToolsBase)
  CloseLibrary((struct Library *)GadToolsBase);

 if(IntuitionBase)
  CloseLibrary((struct Library *)IntuitionBase);

 exit(errorCode);
}

UWORD calcTextDim(UBYTE *text)
{
 struct RastPort tempRastPort;
 struct DrawInfo *drawInfo;
 UWORD width = 0;

 InitRastPort(&tempRastPort);

 printf("calcTextDim() - pubname is >%s<\n", (UBYTE *)argArray[ARG_PUBSCREEN]);

 /* Are we're sharing pointers here? Look it up! */
 if(!(publicScreen = LockPubScreen((UBYTE *)argArray[ARG_PUBSCREEN])))
  quitApp("calcTextDim failed!", RETURN_FAIL);

 drawInfo = GetScreenDrawInfo(publicScreen);

 SetFont(&tempRastPort, drawInfo->dri_Font);

 width = TextLength(&tempRastPort, text, strlen(text));

 FreeScreenDrawInfo(publicScreen, drawInfo);

 UnlockPubScreen(NULL, publicScreen);

 return(width);
}

UWORD preCalcGUI(void)
{
 UWORD a = 0, b = 0, c = 0, maxWidth = 0;

 /* Calculate width of MX gadget */

 while(fontChoices[a])
 {
  b = calcTextDim(fontChoices[a]);

  if(b > c) /* Widest text? If so, set 'c' */
   c = b;

  a++;
 }

 offsetX[0] = c + MX_WIDTH; /* MX gadgets don't have to be fixed in size? If so, adjust accordingly! */

 printf("Width of MX-gadget = %d pixels\n", offsetX[0]);

 /* Calculate width of font text gadget and it's button */
 /* Replace this code with something smarter */

 b = calcTextDim("abcdefghijkl.font") + (GADGET_INNER_SPACING * 2); /* Should we browse through FONTS: and check length of filenames? */
 b = b + GADGET_OUTER_SPACING * 2;
 offsetX[1] = b + calcTextDim("Get...") + (GADGET_INNER_SPACING * 2);

 printf("Width of font-stuff = %d pixels\n", offsetX[1]);

 /* Calculate width of buttons */

 a = 0;
 b = 0;
 c = 0;

 while(buttons[a])
 {
  b = calcTextDim(buttons[a]);

  if(b > c)
   c = b;

  a++;
 }

 offsetX[2] = c * 3 + (3 * GADGET_INNER_SPACING * 2) + (2 * GADGET_OUTER_SPACING * 2);

 printf("Width of buttons = %d pixels\n", offsetX[2]);

 for(a = 0; a <= 2; a++)
 {
  printf("a = %d\n", a);
  if(offsetX[a] > maxWidth)
   maxWidth = offsetX[a];
 }

 printf("maxwidth is %d\n\n", maxWidth);

 for(a = 0; a <= 2; a++)
 {
  offsetX[a] = (maxWidth - offsetX[a]) / 2;
  printf("offsetX[a] = %d\n", offsetX[a]);
 }

 return(maxWidth);
}

void setupGadgets(void)
{
 UBYTE a = 0, b = 0, c = 0;
 UWORD offsetY = 0, winXSize = 0;

 gad = CreateContext(&gadgetList);

 /* ARG_PUBSCREEN is set (currently) set to NULL by Fonts Preferences.
    If not set, it'll default to Workbench (NULL). If we can't open
    the specified screen, should we try the Workbench screen instead? */
 if(!(publicScreen = LockPubScreen((UBYTE *)argArray[ARG_PUBSCREEN])))
 {
  /* Fix: Must be able to say "Workbench" */
  printf("Can't lock %s screen!\n", (UBYTE *)argArray[ARG_PUBSCREEN]);
  quitApp(NULL, RETURN_FAIL);
 }

 if(!(visualInfo = GetVisualInfo(publicScreen, TAG_END)))
  quitApp("Can't get VisualInfo!\n", RETURN_FAIL);

 winXSize = preCalcGUI() + 2 * WINDOW_OFFSET + publicScreen->WBorLeft + publicScreen->WBorRight;

 newGadget.ng_GadgetID = MX_CHOOSEFONT;
 newGadget.ng_TextAttr = publicScreen->Font;
 newGadget.ng_VisualInfo = visualInfo;
 newGadget.ng_LeftEdge = WINDOW_OFFSET + offsetX[0] + publicScreen->WBorLeft;
 newGadget.ng_TopEdge = WINDOW_OFFSET + (publicScreen->WBorTop + publicScreen->Font->ta_YSize + 1);
 newGadget.ng_Flags = NULL;

 if(!(gad = CreateGadget(MX_KIND, gad, &newGadget, GTMX_Labels, fontChoices, GTMX_Active, MX_INIT_ACTIVE, TAG_END)))
  quitApp("Can't setup gadgets!", RETURN_FAIL); /* Must release stuff first!!! */

 offsetY = newGadget.ng_TopEdge + gad->Height;
 printf("offsetY = %d\n", offsetY);

 newGadget.ng_TopEdge = offsetY + GADGET_OUTER_SPACING * 2;
 newGadget.ng_GadgetID = BUTTON_GETFONT;
 newGadget.ng_LeftEdge = WINDOW_OFFSET + publicScreen->WBorLeft;
 newGadget.ng_Width = calcTextDim("Get...") + GADGET_INNER_SPACING * 2;
 newGadget.ng_GadgetText = "Get...";
 newGadget.ng_Height = publicScreen->Font->ta_YSize + GADGET_INNER_SPACING;

 if(!(gad = CreateGadget(BUTTON_KIND, gad, &newGadget, TAG_END)))
   quitApp("Can't setup gadgets!", RETURN_FAIL); /* Must release stuff first!!! */

 newGadget.ng_LeftEdge = WINDOW_OFFSET + GADGET_INNER_SPACING * 2 + GADGET_OUTER_SPACING * 2 + calcTextDim("Get...") +publicScreen->WBorLeft;  newGadget.ng_TopEdge = offsetY + GADGET_OUTER_SPACING * 2;
 newGadget.ng_Width = winXSize - newGadget.ng_Width - WINDOW_OFFSET * 2 - publicScreen->WBorRight - publicScreen->WBorLeft - GADGET_OUTER_SPACING * 2;
 newGadget.ng_Height = publicScreen->Font->ta_YSize + GADGET_INNER_SPACING;

 if(!(gad = CreateGadget(TEXT_KIND, gad, &newGadget, GTTX_Border, TRUE, TAG_END)))
   quitApp("Can't setup gadgets!", RETURN_FAIL); // Must release stuff first!!!   

 // We need this pointer for updating the textdisplay after the user has chosen a font   
 textGadget = gad;

 offsetY = newGadget.ng_TopEdge + gad->Height + WINDOW_OFFSET;
 printf("offsetY = %d\n", offsetY);
 /* Create three gadgets of the same width. When we're done with the while() loop, C will contain the width of the
    widest gadget. For this to work (safely?), the "buttons" array has to be NULL-terminated! */
 while(buttons[a])
 {
  b = calcTextDim(buttons[a]);
  if(b > c) /* If this is the widest text then 'c' should hold the current maximum width */
   c = b;
  a++;
 }

 c = c + GADGET_INNER_SPACING * 2; /* Add the inner spacing to the maximum width */
 a = 0;

 newGadget.ng_TopEdge = offsetY + GADGET_OUTER_SPACING * 2;

 while(buttons[a])
 {
  newGadget.ng_GadgetID = a + BUTTON_SAVE; /* Note the ascending order of these button IDs - break them, and this routine will fail! */
  newGadget.ng_LeftEdge = a * c + (GADGET_OUTER_SPACING * 2) * a + WINDOW_OFFSET + offsetX[2] + publicScreen->WBorLeft;
  newGadget.ng_Width = c;
  newGadget.ng_Height = publicScreen->Font->ta_YSize + GADGET_INNER_SPACING;
  newGadget.ng_GadgetText = buttons[a];

  if(!(gad = CreateGadget(BUTTON_KIND, gad, &newGadget, TAG_END)))
   quitApp("Can't setup gadgets!", RETURN_FAIL); /* Must release stuff first!!! */

  a++;
 }

 updateGUI(MX_INIT_ACTIVE);

 openWindow(winXSize, newGadget.ng_TopEdge + newGadget.ng_Height + WINDOW_OFFSET + publicScreen->WBorBottom, gadgetList); /* Should we make this calculation any fancier? Yes, probably - this is not pretty */

 UnlockPubScreen(NULL, publicScreen); /* The window holds a lock - release the screen sooner! */

 FreeGadgets(gadgetList);
}

void updateGUI(UBYTE fontChoice)
{
 GT_SetGadgetAttrs(textGadget, prefsWindow, NULL, GTTX_Text, fontPrefs[fontChoice]->fp_Name, TAG_DONE);
 printf("updateGUI | fontChoice is %d\n", fontChoice);
 printf("fontPrefs->fp_Name is >%s<\n", fontPrefs[fontChoice]->fp_Name);
 printf("fontPrefs->fp_TextAttr.ta_YSize is >%d<\n", fontPrefs[fontChoice]->fp_TextAttr.ta_YSize);
}

void openWindow(UWORD winWidth, UWORD winHeight, struct Gadget *winGadgets)
{
 ULONG signals, class;
 UWORD code, menuNumber;
 UBYTE fontChoice = 0; /* The BYTE holding what font we're editing right now */
 BOOL running = TRUE;
 struct MenuItem *menuItem;
 struct IntuiMessage *intuiMessage;
 struct Gadget *gadget;
 APTR visualInfoPtr;
 STRPTR fileName;

 /*
 if(!(publicScreen = LockPubScreen(NULL))) // We now hold two locks on this screen. Is this valid? Change!
  quitApp("Unable to lock Workbench screen!", RETURN_FAIL);
*/
 prefsWindow = OpenWindowTags(NULL,	WA_Left,	10,
					WA_Top,		10,
					WA_Width,	winWidth,
					WA_Height,	winHeight,
					WA_DragBar,	TRUE,
					WA_CloseGadget, TRUE,
					WA_DepthGadget, TRUE,
					WA_Activate,	TRUE,
					WA_Gadgets,	winGadgets,
					WA_Title,	"Font Preferences",
					WA_PubScreen,	publicScreen,
					WA_IDCMP, 	IDCMP_CLOSEWINDOW | IDCMP_REFRESHWINDOW |
							IDCMP_MENUPICK | BUTTONIDCMP | MXIDCMP,
					TAG_END);
 /* Check if window really opened! */

 /*UnlockPubScreen(NULL, publicScreen);*/

 if(visualInfoPtr = GetVisualInfo(prefsWindow->WScreen, TAG_END))
 {
  if(!(winMenus = setupMenus(visualInfoPtr)))
   printf("setupMenus() failed!\n");
  else
   SetMenuStrip(prefsWindow, winMenus);

  FreeVisualInfo(visualInfoPtr); /* Is it safe to free the visual info already? */
 }
 else
  printf("Can't get visual info for windows!\n");

 GT_RefreshWindow(prefsWindow, NULL);

 while(running)
 {
  signals = Wait(1L << prefsWindow->UserPort->mp_SigBit | SIGBREAKF_CTRL_C);

  if(signals & (1L << prefsWindow->UserPort->mp_SigBit))
  {
   /* We got a IDCMP message, analyze it and take action */

   while(NULL != (intuiMessage = GT_GetIMsg(prefsWindow->UserPort)))
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

      /* Make sure to process every single menu choice made. RKRM: Libraries p. 176 */
      while(menuNumber != MENUNULL)
      {
       menuItem = ItemAddress(winMenus, menuNumber);

       printf("%d chosen!\n", (UBYTE)GTMENUITEM_USERDATA(menuItem));

       /* The output from GTMENUITEM_USERDATA must be casted */
       switch((UBYTE)GTMENUITEM_USERDATA(menuItem))
       {
	case MENU_ID_OPEN :
	 if(fileName = aslOpenPrefs())
         {
          printf("reading %s...\n", fileName);
          readIFF(fileName, fontPrefs);
          updateGUI(fontChoice); /* For the moment, always update the GUI */
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
         updateGUI(fontChoice);
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

     case IDCMP_GADGETDOWN : /* MX Gadget */
      printf("IDCMP_GADGETDOWN!\n");

      switch(gadget->GadgetID)
      {
       case MX_CHOOSEFONT :
        if(code != fontChoice) /* No reason to update GUI if nothing has changed */
        {
         fontChoice = code;
         updateGUI(fontChoice);
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

 if(winMenus)
 {
  ClearMenuStrip(prefsWindow); /* Really necessary? */
  FreeMenus(winMenus);
 }

 CloseWindow(prefsWindow);
}

int main(void)
{
 struct Screen *screenPtr;

 if(!(IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 0L)))
  quitApp("Can't open intuition.library!", RETURN_FAIL);

 if(!(GadToolsBase = OpenLibrary("gadtools.library", 0L)))
  quitApp("Can't open gadtools.library!", RETURN_FAIL);

 if(!(GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 0L)))
  quitApp("Can't open graphics.library!", RETURN_FAIL);

 if(!(IFFParseBase = OpenLibrary("iffparse.library", 0L)))
  quitApp("Can't open iffparse.library!", RETURN_FAIL);

 /* Get arguments. If failure, should we quit or not? If started from
    shell, do we assume the user wants to run Font Preferences on a "non
    GUI interactive basis"? Request for comments to author! */
 if(!(readArgs = getArguments()))
  printf("Warning: getArguments() failed!\n");

 initPrefMem();

 printf("Welcome to Font Preferences for AROS!\nThis is alpha class software: read CAVEATS before you continue!\n");

 /* If FROM is set, then also check for the USE and SAVE keywords - but only then. There isn't
    any point in just replacing the same settings with the very same values? */
 if(argArray[ARG_FROM])
 {
  printf("Reading Preferences from >%s<...\n", (UBYTE *)argArray[ARG_FROM]);
  readIFF((UBYTE *)argArray[ARG_FROM], fontPrefs);

  /* If USE or SAVE is set, write the FROM file to ENV: and/or ENVARC: and then quit. Is this
     what the "Classic" Font Preferences does? Look it up! (As a side note, if FILE is not
     found, the old settings will be overwritten with default values. Should we avoid this and
     implement some error checking in writeIFF() ? Request for comments to author! */
  if(argArray[ARG_USE] || argArray[ARG_SAVE])
  {
   writeIFF("ENV:sys/font.prefs", fontPrefs);

   printf("Wrote ENV: settings!\n");

   if(argArray[ARG_SAVE])
   {
    writeIFF("ENVARC:sys/font.prefs", fontPrefs);
    printf("Wrote ENVARC: settings!\n");
   }
   /* Don't launch the rest of the program, just exit */
   quitApp(NULL, RETURN_OK);
  }

 }
 else
  readIFF("ENV:sys/font.prefs", fontPrefs);

 /* What is "EDIT" supposed to do? Look it up! */
 if(argArray[ARG_EDIT])
  printf("EDIT keyword set!\n");

 /* We're about to setup the GUI. We need a screen lock to obtain various information of
    our screen. */

 /* ARG_PUBSCREEN is set (currently) set to NULL by Fonts Preferences.
    If not set, it'll default to Workbench (NULL). If we can't open
    the specified screen, should we try the Workbench screen instead? */
 if(screenPtr = LockPubScreen((UBYTE *)argArray[ARG_PUBSCREEN]))
 {
  setupGadgets();

  UnlockPubScreen(NULL, screenPtr);
 }
 else
 {
  printf("Can't lock %s screen!\n", (UBYTE *)argArray[ARG_PUBSCREEN]);
  quitApp(NULL, RETURN_FAIL);
 }

 quitApp(NULL, RETURN_OK);

 exit(0); /* Suppresses GCC warning */
}
