#include <exec/types.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <intuition/intuitionbase.h>
#include <intuition/preferences.h>
#include "intuition_preferences.h"
#include "intuition_intern.h"

void LoadDefaultPreferences(struct IntuitionBase * IntuitionBase)
{
    BYTE read_preferences = FALSE;
    /*
    ** Load the intuition preferences from a file on the disk
    ** Allocate storage for the preferences, even if it's just a copy
    ** of the default preferences.
    */
    GetPrivIBase(IntuitionBase)->DefaultPreferences = 
            AllocMem(sizeof(struct Preferences),
                     MEMF_CLEAR);

                     
#warning FIXME:Try to load preferences from a file!
    

/*******************************************************************
    DOSBase = OpenLibrary("dos.library",0);
    if (NULL != DOSBase)
    {
      if (NULL != (pref_file = Open("sys:???",MODE_OLDFILE)))
      {
        *
        **  Read it and check whether the file was valid.
        *

        if (sizeof(struct Preferences) ==
            Read(pref_file, 
                 GetPrivIBase(IntuitionBase)->DefaultPreferences,
                 sizeof(struct Preferences)))
          read_preferences = TRUE;

        Close(pref_file);
      }
      CloseLibrary(DOSBase)
    }
****************************************************************/

    if (FALSE == read_preferences)
    {
      /* 
      ** no (valid) preferences file is available.
      */
      memcpy(GetPrivIBase(IntuitionBase)->DefaultPreferences,
             &IntuitionDefaultPreferences,
             sizeof(struct Preferences));
    }


    /*
    ** Activate the preferences...
    */

    GetPrivIBase(IntuitionBase)->ActivePreferences = 
            AllocMem(sizeof(struct Preferences),
                     MEMF_CLEAR);

    SetPrefs(GetPrivIBase(IntuitionBase)->DefaultPreferences,
             sizeof(struct Preferences),
             TRUE);
}
