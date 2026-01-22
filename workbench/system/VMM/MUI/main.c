#define DEBUG 1
#include "defs.h"

char AppWindow [] = "CON:0/0/640/200/VMM Window/CLOSE/AUTO/WAIT";
static UBYTE **ToolTypes;

static BOOL OpenLibs1 (void)

{
if ((IconBase = OpenLibrary ("icon.library", 0L)) == NULL)
  {
  printf ("Couldn't open icon.library\n");
  return (FALSE);
  }

if ((CxBase = OpenLibrary ("commodities.library", 0L)) == NULL)
  {
  printf ("Couldn't open commodities.library\n");
  return (FALSE);
  }

if ((UtilityBase = (struct UtilityBase *)OpenLibrary ("utility.library", 37L)) == NULL)
  {
  printf ("Couldn't open utility.library\n");
  return (FALSE);
  }

return (TRUE);
}

/**********************************************************************/

static BOOL OpenLibs2 (void)

{
if ((AslBase = OpenLibrary ("asl.library", 37L)) == NULL)
  {
  printf ("Couldn't open asl.library\n");
  return (FALSE);
  }

if ((IntuitionBase = (struct IntuitionBase *)OpenLibrary ("intuition.library", 37L)) == NULL)
  {
  printf ("Couldn't open intuition.library\n");
  return (FALSE);
  }

if ((MUIMasterBase = OpenLibrary (MUIMASTER_NAME, 10L)) == NULL)
  {
  printf ("Couldn't open muimaster.library V10\n"
          "You need at least MUI 2.3 to run VMM\n");
  return (FALSE);
  }

LocaleBase = OpenLibrary ("locale.library", 38L);

#if(0)
OpenVMMCatalog (NULL, NULL);
#else
  Locale_Initialize();
#endif
return (TRUE);
}

/**********************************************************************/

static void CloseAll (void)

{
if (ExtCxPort != NULL)
  {
  ExtCxPort->PrefsTask = NULL;
  if (ExtCxPort->ShowSignal != -1L)
    FreeSignal (ExtCxPort->ShowSignal);
  ExtCxPort->ShowSignal = -1;
  }

UninstallAsCommodity ();      /* only does it, if CxParams haven't been
                               * sent to VMM. */
if (ToolTypes != NULL)
  ArgArrayDone ();

#if(0)
CloseVMMCatalog ();
#else
  Locale_Deinitialize();
#endif

if (LocaleBase != NULL)
  CloseLibrary (LocaleBase);
if (MUIMasterBase != NULL)
  CloseLibrary (MUIMasterBase);
if (IntuitionBase != NULL)
  CloseLibrary ((struct Library *)IntuitionBase);
if (AslBase != NULL)
  CloseLibrary (AslBase);
if (UtilityBase != NULL)
  CloseLibrary ((struct Library *)UtilityBase);
if (IconBase != NULL)
  CloseLibrary (IconBase);
if (CxBase != NULL)
  CloseLibrary (CxBase);
}

/**********************************************************************/

int main (LONG argc, UBYTE **argv)

{
BOOL Quit;
BOOL QuestionMark;

if (!OpenLibs1 ())
  {
  CloseAll ();
  return (10);
  }

ToolTypes = ArgArrayInit (argc, argv);

if (ToolTypes == NULL)
  {
  ForceOverwrite = FALSE;
  ShowGUI = TRUE;
  CXPri = 0L;
  CXPopKey = DEFAULT_POPKEY;
  Quit = FALSE;
  QuestionMark = FALSE;
  CfgName = CFG_NAME_USE;
  }
else
  {
  ShowGUI = (Stricmp (ArgString (ToolTypes, "CX_POPUP", "YES"), "YES") == 0);
  CXPri = ArgInt (ToolTypes, "CX_PRIORITY", 0L);
  CXPopKey = ArgString (ToolTypes, "CX_POPKEY", DEFAULT_POPKEY);
  Quit = (FindToolType (ToolTypes, "QUIT") != NULL);
  QuestionMark = (FindToolType (ToolTypes, "?") != NULL);
  CfgName = ArgString (ToolTypes, "SETTINGS", CFG_NAME_USE);
  ForceOverwrite = (FindToolType (ToolTypes, "FORCE") != NULL);
  }

if (QuestionMark)
  {
  printf ("CX_PRIORITY/K/N,CX_POPKEY/K,CX_POPUP/K,SETTINGS/K,QUIT/S,FORCE/S\n");
  CloseAll ();
  return (0);
  }
          
if (Quit)
  {
  if (!StopVMM ())
    printf ("VMM is not running\n");

  CloseAll ();
  return (0);
  }

ExtCxPort = (struct ExtPort*) FindPort (CXPORTNAME);

if (ExtCxPort == NULL)
  {
  ReadHotkeysFromConfig (CfgName);

  /* This sets ExtCxPort */
  if (!InstallAsCommodity ())
    {
    printf ("Couldn't install commodity\n");
    CloseAll ();
    return (10);
    }
  }

ExtCxPort->PrefsTask = FindTask (NULL);
ExtCxPort->ShowSignal = AllocSignal (-1L);

    bug("[VMM:GUI] %s: commodity installed\n", __func__);

if (ShowGUI || VMM_RUNNING)
  {
  if (!OpenLibs2 ())
    {
    CloseAll ();
    return (10);
    }

  if (HandleGUI () == Q_QUITBOTH)
    {
    bug("[VMM:GUI] %s: killing handler ..\n", __func__);

    Forbid ();
    CloseLibrary (MUIMasterBase);
    RemLibrary (MUIMasterBase);
    Permit ();
    MUIMasterBase = NULL;
    StopVMM ();
    }
  }
else
  {
  /* Simply start up VMM */
  bug("[VMM:GUI] %s: reading config...\n", __func__);
  ReadHotkeysFromConfig (CfgName);

  bug("[VMM:GUI] %s: starting VMM\n", __func__);
  StartVMM ();
  }

    bug("[VMM:GUI] %s: closing resources...\n", __func__);

CloseAll ();
    bug("[VMM:GUI] %s: exiting\n", __func__);
  return (0);
}

#if defined(__GNUC__) && !defined(__AROS__)

/* Just a dummy needed by GCC */
void __main (void)

{
}

#endif
