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
#include <libraries/locale.h>	/* Tagitems */

#include <proto/locale.h>

#include <dos/dos.h> /* Return codes */

#include <prefs/font.h>

#include <exec/types.h>
#include <exec/memory.h>

#include <stdio.h>

#include "global.h"

#define CATCOMP_NUMBERS	// Enable numbers (use CATCOMP_ARRAY instead?)
#define CATCOMP_STRINGS	// Enable default strings (necessary?)
#define CATCOMP_ARRAY
#include "fontprefs_strings.h"

UBYTE version[] = "$VER: Font 0.11 (30.09.2001)";

/* Our local library bases. They don't need to be set to NULL. David Gerber has pointed out that
   ANSI-C requires automated NULL initialization when nothing else is specified. However, if the
   library pointers are declared as "extern" it seems that some AROS link module will initialize
   them automatically which is a bad thing as our Open/CloseLibrary() pair will override whatever
   data already present in the library pointers.
*/

struct IntuitionBase *IntuitionBase = NULL;
struct GfxBase *GfxBase = NULL;
struct Library *GadToolsBase = NULL;
struct LocaleBase *LocaleBase = NULL;
struct Library *IFFParseBase = NULL;
struct Library *AslBase = NULL;

/* For now, library version 37 are required. It should be sufficient, but it hasn't been
   more thoroughly checked!
*/
struct libInfo
{
 APTR		lT_Library;
 STRPTR		lT_Name;
 ULONG		lT_Version;
}
libTable[] =
{
 { &IntuitionBase,	"intuition.library",	37L},
 { &GadToolsBase,	"gadtools.library",	37L},
 { &GfxBase,		"graphics.library",	37L},
 { &IFFParseBase,	"iffparse.library",	37L},
 { &AslBase,		"asl.library",		37L},
 { NULL }
};

struct Window *prefsWindow;
struct Screen *publicScreen;
struct Catalog *catalogPtr;
struct RDArgs *readArgs;

extern struct FontPrefs *fontPrefs[3];	/* init.c */
extern struct IFFHandle *iffHandle;	/* handleiff.c */

IPTR argArray[NUM_ARGS];		/* args.c */

extern void inputLoop(struct AppGUIData *);
extern struct AppGUIData * createGadgets(struct Screen *, APTR, struct Catalog *);
extern struct Window * openAppWindow(struct Screen *, struct AppGUIData *, APTR, struct Catalog *);
extern void initDefaultPrefs(struct FontPrefs **);
extern struct Menu * setupMenus(APTR);
extern struct RDArgs * getArguments(void);
extern void readIFF(UBYTE *, struct FontPrefs **);
extern void writeIFF(UBYTE *, struct FontPrefs **);
extern BOOL initPrefMem(void);

STRPTR getCatalog(struct Catalog *catalogPtr, ULONG id)
{
 STRPTR string;
    
 if(catalogPtr)
  string = GetCatalogStr(catalogPtr, id, CatCompArray[id].cca_Str);
 else
  string = CatCompArray[id].cca_Str;

 return(string);
}

void quitApp(UBYTE *errorMsg, UBYTE errorCode)
{
 UBYTE a;
 struct libInfo *tmpLibInfo = libTable;

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

 if(LocaleBase)
 {
  CloseCatalog(catalogPtr); // Passing NULL should be valid, look it up!
  CloseLibrary((struct Library *)LocaleBase);
 }

 printf("Closing libraries...\n");


 while(tmpLibInfo->lT_Name)
 {
  if((*(struct Library **)tmpLibInfo->lT_Library))
  {
   printf("Closing >%s<... ", tmpLibInfo->lT_Name);
   CloseLibrary((*(struct Library **)tmpLibInfo->lT_Library));
   printf("%s is closed!\n", tmpLibInfo->lT_Name);
  }
  tmpLibInfo++;
 }

 printf("Good bye!\n");

 exit(errorCode);
}

int main(void)
{
 struct Screen *screenPtr;
 APTR drawInfo;
 struct AppGUIData *appGUIData; // Not an ideal solution. This should be extern to save some stack memory!
 UBYTE returnCode = RETURN_FAIL;
 struct libInfo *tmpLibInfo = libTable;
 UBYTE tmpString[128]; // What if library name plus error message exceeds 128 bytes?

 if(!(LocaleBase = (struct LocaleBase *)OpenLibrary("locale.library", 0L)))
  printf("Warning: Can't open locale.library!"); // Non runtime critical error

 // We should check for the proper catalog version here using the OC_Version tag!
 // NULL is not runtime critical but should be dealt with in a smarter fashion!
 catalogPtr = OpenCatalog(NULL, "Sys/fontprefs.catalog", OC_BuiltInLanguage, "english", TAG_DONE);

 while(tmpLibInfo->lT_Library)
 {
  printf("Opening %s V%ld - ", tmpLibInfo->lT_Name, tmpLibInfo->lT_Version);

  if(!((*(struct Library **)tmpLibInfo->lT_Library = OpenLibrary(tmpLibInfo->lT_Name, tmpLibInfo->lT_Version))))
  {
   sprintf(tmpString, getCatalog(catalogPtr, MSG_CANT_OPEN_LIB), tmpLibInfo->lT_Name,tmpLibInfo->lT_Version);
   quitApp(tmpString, RETURN_FAIL);
  }
  else
   printf("%s opened!\n", tmpLibInfo->lT_Name);

  tmpLibInfo++;
 }

 printf("All libraries are opened!\n");

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
   if((appGUIData = createGadgets(screenPtr, drawInfo, catalogPtr)))
   {
    if((appGUIData = openAppWindow(screenPtr, appGUIData, drawInfo, catalogPtr)))
     inputLoop(appGUIData); // This is our main I/O event loop!
    else
     quitApp(getCatalog(catalogPtr, MSG_CANT_CREATE_WIN), RETURN_FAIL);

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
    quitApp(getCatalog(catalogPtr, MSG_CANT_CREATE_GADGET), RETURN_FAIL);

   FreeVisualInfo(drawInfo);
   returnCode = RETURN_OK;

   if(appGUIData) // This should always be TRUE by now!
    FreeMem(appGUIData, sizeof(struct AppGUIData));
  }
  else
   quitApp(getCatalog(catalogPtr, MSG_CANT_GET_VI), RETURN_FAIL);

  UnlockPubScreen(NULL, screenPtr); // Is this safe to call twice? (First call in openAppWindow())
 }
 else
  quitApp(getCatalog(catalogPtr, MSG_CANT_LOCK_SCR), RETURN_FAIL);

 quitApp(NULL, returnCode);

 exit(0); /* Suppresses GCC warning */
}
