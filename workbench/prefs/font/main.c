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

UBYTE version[] = "$VER: Font 0.6 (19.4.2001)";

struct IntuitionBase *IntuitionBase;
struct Library *GadToolsBase;
struct GfxBase *GfxBase;
struct Library *IFFParseBase;

struct Window *prefsWindow;
struct Screen *publicScreen;

struct RDArgs *readArgs;

extern struct FontPrefs *fontPrefs[3];	/* init.c */
extern struct IFFHandle *iffHandle;	/* handleiff.c */

IPTR argArray[NUM_ARGS];		/* args.c */

extern void inputLoop(struct AppGUIData *);
extern struct AppGUIData * createGadgets(struct Screen *, APTR);
extern struct Window * openAppWindow(struct Screen *, struct AppGUIData *, APTR);
extern void initDefaultPrefs(struct FontPrefs **);
extern struct Menu * setupMenus(APTR);
extern struct RDArgs * getArguments(void);
extern void readIFF(UBYTE *, struct FontPrefs **);
extern void writeIFF(UBYTE *, struct FontPrefs **);
extern BOOL initPrefMem(void);

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

int main(void)
{
 struct Screen *screenPtr;
 APTR drawInfo;
 struct AppGUIData *appGUIData;
 UBYTE returnCode = RETURN_FAIL;

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

 if(!(initPrefMem()))
  quitApp("Unable to allocate Font Preferences memory!", RETURN_FAIL);

 printf("Welcome to Font Preferences for AROS!\nThis is alpha class software: read CAVEATS before you continue!\n");

 /* If FROM is set, then also check for the USE and SAVE keywords - but only then. There isn't
    any point in just replacing the same settings with the very same values? */
 if(argArray[ARG_FROM])
 {
  readIFF((UBYTE *)argArray[ARG_FROM], fontPrefs);

  /* If USE or SAVE is set, write the FROM file to ENV: and/or ENVARC: and then quit. Is this
     what the "Classic" Font Preferences does? Look it up! (As a side note, if FILE is not
     found, the old settings will be overwritten with default values. Should we avoid this and
     implement some error checking in writeIFF() ? What if FROM is not set? Should we still 
     react for USE and SAVE (which we currently don't)? Request for comments to author! */

  if(argArray[ARG_USE] || argArray[ARG_SAVE])
  {
   writeIFF("ENV:sys/font.prefs", fontPrefs);

   if(argArray[ARG_SAVE])
    writeIFF("ENVARC:sys/font.prefs", fontPrefs);

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
 if((screenPtr = LockPubScreen((UBYTE *)argArray[ARG_PUBSCREEN])))
 {
  if((drawInfo = GetVisualInfo(screenPtr, NULL)))
  {
   if((appGUIData = createGadgets(screenPtr, drawInfo)))
   {
    if((appGUIData = openAppWindow(screenPtr, appGUIData, drawInfo)))
     inputLoop(appGUIData);
    else
     printf("Can't open window!");

    if(appGUIData->agd_GadgetList)
     FreeGadgets(appGUIData->agd_GadgetList);

    if(appGUIData->agd_Menu)
    {  
     ClearMenuStrip(appGUIData->agd_Window); // Really necessary?
     FreeMenus(appGUIData->agd_Menu);
    }

    if(appGUIData->agd_Window)
     CloseWindow(appGUIData->agd_Window);
   }
   else
    printf("Can't setup gadgets!");

   FreeVisualInfo(drawInfo);
   returnCode = RETURN_OK;
  }
  else
   printf("GetVisualInfo() failed!");

  UnlockPubScreen(NULL, screenPtr); // Is this safe to call twice? (First call in openAppWindow())

 }
 else
  printf("Can't lock %s screen!", (UBYTE *)argArray[ARG_PUBSCREEN]);

 quitApp(NULL, returnCode);

 exit(0); /* Suppresses GCC warning */
}
