/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id: getnextappicon.c 19008 2003-07-30 22:53:38Z craid-hjb $

    Desc: Accesses AppIcon information from workbench.library.
    Lang: English
*/

#include <exec/types.h>
#include <exec/ports.h>
#include <utility/tagitem.h>
#include <dos/dos.h>
#include <graphics/gfx.h>

#include <proto/utility.h>

#include "workbench_intern.h"
#include <workbench/workbench.h>

#define DEBUG 0
#include <aros/debug.h>
/*****************************************************************************

    NAME */
    #include <proto/workbench.h>

        AROS_LH2(struct DiskObject *, GetNextAppIcon,
/*  SYNOPSIS */
        AROS_LHA(struct DiskObject *, lastdiskobj,  A0),
        AROS_LHA(char *,              text,         A1),

/*  LOCATION */
        struct WorkbenchBase *, WorkbenchBase, 10, Workbench)

/*  FUNCTION

    Accesses AppIcon information from workbench.library. This function 
    is meant for iterations through wblibs´ AppIcon storage as needed 
    to display those. 
        Initialised with a NULL as the first argument, it iterates
    through all AppIcons stored in workbench.library by returning the
    pointer to the next AppIcon´s DiskObject structure and copies the
    name of the given AppIcon to the given array. The function returns
    a NULL if the end of AppIcon list was reached or if no AppIcons 
    were stored.

    INPUTS

    lastdiskobj  --  NULL (initial value) or pointer to a DiskObject 
                     structure stored in workbench.library
    text         --  char array pointer to store AppIcon´s name in

    RESULT

    A pointer to an DiskObject structure -- which should be used within
    the next function call to access the next AppIcon -- or NULL if no
    AppIcons were found or end of list was reached.

    NOTES

    EXAMPLE

      struct DiskObject *_nb_dob = NULL;
      char text[32];

      while ( _nb_dob = GetNextAppIcon(_nb_dob, &text) )
      {
          printf("appicon found: %s \n", text);
      }


    BUGS

    SEE ALSO

    AddAppIconA(), RemoveAppIcon(), WorkbenchControlA(), DrawIconStateA()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* get wblibs list of appicons */
    struct List *appiconlist = NULL;
    appiconlist = &WorkbenchBase->wb_AppIcons;


    /* return NULL if list empty or argument dob is already from last appicon in list*/
    if ( !IsListEmpty(appiconlist) )
    {
        /* return NULL if last entry reached */
        if (lastdiskobj == ((struct AppIcon *)(appiconlist->lh_TailPred))->ai_DiskObject) return NULL;

        struct Node *currentnode = appiconlist->lh_Head;
        currentnode = GetHead(appiconlist);

        /* iterations through the list */
        do
        {
            /* check if given DiskObject was found and return it´s successor */
            if ( ((struct AppIcon*)currentnode)->ai_DiskObject == lastdiskobj)
            {
                currentnode = GetSucc(currentnode);

                bug("[getnextappicon] appicon found: %s \n", ((struct AppIcon*)currentnode)->ai_Text );

                strcpy(text, ((struct AppIcon*)currentnode)->ai_Text );
                return ((struct AppIcon*)currentnode)->ai_DiskObject;
            }


        } while ( currentnode = GetSucc(currentnode) );

        /* return first entry if NULL was given or DiskObject could not be found */
        strcpy(text, ((struct AppIcon *)(appiconlist->lh_Head))->ai_Text );
        return ((struct AppIcon *)(appiconlist->lh_Head))->ai_DiskObject;

    }
  
    return NULL;

    AROS_LIBFUNC_EXIT
} /* GetNextAppIcon() */
