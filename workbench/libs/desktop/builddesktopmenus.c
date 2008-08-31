#define MUIMASTER_YES_INLINE_STDARG

#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/classusr.h>
#include <libraries/desktop.h>
#include <libraries/gadtools.h>
#include <utility/tagitem.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#include "desktop_intern.h"
#include "desktop_intern_protos.h"
#include "support.h"

#define DEBUG 1
#include <aros/debug.h>

/*****************************************************************************

    NAME */

        #include <proto/desktop.h>

        AROS_LH0(struct NewMenu *, BuildDesktopMenus,

/*  SYNOPSIS */
     
/*  LOCATION */

        struct DesktopBase *, DesktopBase, 11, Desktop)
/*
   FUNCTION

   INPUTS

   RESULT

   NOTES

   EXAMPLE

   BUGS

   SEE ALSO

   INTERNALS

   HISTORY

   *****************************************************************************
 */
{
    AROS_LIBFUNC_INIT 
    struct DesktopOperationItem *doiD,
                   *doiW,
                   *doiI;
    LONG            i = 0,
        j = 0;
    ULONG           numberDesktopItems = 0,
        numberWindowItems = 0,
        numberIconItems = 0;
    ULONG           subs = 0;
    ULONG           numberMenuItems = 0;
    struct NewMenu *menuDat;

// first, count the number of menu items in each menu
    doiD = GetMenuItemList(DOC_DESKTOPOP);
    if (doiD)
    {
        while (doiD[numberDesktopItems].doi_Code != 0
               && doiD[numberDesktopItems].doi_Name != NULL)
        {
            if (doiD[numberWindowItems].doi_SubItems)
            {
                subs = 0;
                while (doiD[numberWindowItems].doi_SubItems[subs].doi_Code !=
                       0
                       && doiD[numberWindowItems].doi_SubItems[subs].
                       doi_Name != NULL)
                {
                    subs++;
                    numberMenuItems++;
                }
            }

            numberDesktopItems++;
            numberMenuItems++;
        }
    // for the menu title
        if (numberDesktopItems)
            numberMenuItems++;
    }

    doiW = GetMenuItemList(DOC_WINDOWOP);
    if (doiW)
    {
        while (doiW[numberWindowItems].doi_Code != 0)
        {
            if (doiW[numberWindowItems].doi_SubItems)
            {
                subs = 0;
                while (doiW[numberWindowItems].doi_SubItems[subs].doi_Code !=
                       0)
                {
                    subs++;
                    numberMenuItems++;
                }
            }
            numberWindowItems++;
            numberMenuItems++;
        }
        if (numberWindowItems)
            numberMenuItems++;
    }

    doiI = GetMenuItemList(DOC_ICONOP);
    if (doiI)
    {
        while (doiI[numberIconItems].doi_Code != 0
               && doiI[numberIconItems].doi_Name != NULL)
        {
            if (doiI[numberIconItems].doi_SubItems)
            {
                subs = 0;
                while (doiI[numberIconItems].doi_SubItems[subs].doi_Code != 0
                       && doiI[numberIconItems].doi_SubItems[subs].doi_Name !=
                       NULL)
                {
                    subs++;
                    numberMenuItems++;
                }
            }
            numberIconItems++;
            numberMenuItems++;
        }
        if (numberIconItems)
            numberMenuItems++;
    }

    menuDat =
        (struct NewMenu *) AllocVec(sizeof(struct NewMenu) *
                                    (numberMenuItems + 1), MEMF_ANY);

// create the Desktop menu
    if (numberDesktopItems > 0)
    {
        menuDat[i].nm_Type = NM_TITLE;
        menuDat[i].nm_Label = "AROS";
        menuDat[i].nm_CommKey = 0;
        menuDat[i].nm_Flags = 0;
        menuDat[i].nm_MutualExclude = 0;
        menuDat[i].nm_UserData = 0;

        i++;
        j = 0;
        while (doiD[j].doi_Code != 0 && doiD[j].doi_Name != NULL)
            processOperationItem(&i, &j, doiD, menuDat);
    }

    if (numberWindowItems > 0)
    {
        menuDat[i].nm_Type = NM_TITLE;
        menuDat[i].nm_Label = "Window";
        menuDat[i].nm_CommKey = 0;
        menuDat[i].nm_Flags = 0;
        menuDat[i].nm_MutualExclude = 0;
        menuDat[i].nm_UserData = 0;

        i++;
        j = 0;
        while (doiW[j].doi_Code != 0 && doiW[j].doi_Name != NULL)
            processOperationItem(&i, &j, doiW, menuDat);
    }

    if (numberIconItems > 0)
    {
        menuDat[i].nm_Type = NM_TITLE;
        menuDat[i].nm_Label = "Icon";
        menuDat[i].nm_CommKey = 0;
        menuDat[i].nm_Flags = 0;
        menuDat[i].nm_MutualExclude = 0;
        menuDat[i].nm_UserData = 0;

        i++;
        j = 0;
        while (doiI[j].doi_Code != 0 && doiI[j].doi_Name != NULL)
            processOperationItem(&i, &j, doiI, menuDat);
    }

    doExclude(doiD, menuDat, 1);
    doExclude(doiW, menuDat, numberDesktopItems + 1);
    doExclude(doiI, menuDat, numberDesktopItems + numberWindowItems + 1);

    menuDat[i].nm_Type = NM_END;
    menuDat[i].nm_Label = NULL;
    menuDat[i].nm_CommKey = 0;
    menuDat[i].nm_Flags = 0;
    menuDat[i].nm_MutualExclude = 0;
    menuDat[i].nm_UserData = 0;

    i = 0;
    while (menuDat[i].nm_Type != NM_END)
        i++;


    return menuDat;

    AROS_LIBFUNC_EXIT
}              /* BuildDesktopMenus */
