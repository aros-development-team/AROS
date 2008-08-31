/*
   Copyright © 1995-2003, The AROS Development Team. All rights reserved.
   $Id$ 
 */

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/classusr.h>
#include <libraries/desktop.h>
#include <libraries/mui.h>
#include <utility/tagitem.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#include "desktop_intern.h"
#include "support.h"

#include "iconclass.h"
#include "iconobserver.h"
#include "iconcontainerclass.h"
#include "iconcontainerobserver.h"
#include "observer.h"

#define DEBUG 1
#include <aros/debug.h>

/*****************************************************************************

    NAME */

        #include <proto/desktop.h>

        AROS_LH1(struct DesktopOperationItem *, GetMenuItemList,

/*  SYNOPSIS */

	AROS_LHA(ULONG, operationType, D0),
     
/*  LOCATION */
        
	struct DesktopBase *, DesktopBase, 10, Desktop)
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
    struct DesktopOperationItem *doi = NULL;
    struct DesktopOperation *dop,
                   *subdop;
    LONG            items = 0,
        index = 0,
        subindex = 0;
    //LONG            itemNumber = 1;

    dop = (struct DesktopOperation *) DesktopBase->db_OperationList.lh_Head;
    while (dop->do_Node.ln_Succ)
    {
        items++;
        dop = (struct DesktopOperation *) dop->do_Node.ln_Succ;
    }

    if (items)
    {
        doi =
            (struct DesktopOperationItem *)
            AllocVec(sizeof(struct DesktopOperationItem) * (items + 1),
                     MEMF_ANY);
    }
    else
        return doi;

    dop = (struct DesktopOperation *) DesktopBase->db_OperationList.lh_Head;
    while (dop->do_Node.ln_Succ)
    {
        if (dop->do_Code & operationType)
        {
            doi[index].doi_Code = dop->do_Code;
            doi[index].doi_Number = dop->do_Number;
            doi[index].doi_Name = dop->do_Name;

            doi[index].doi_Flags = 0;
            if (dop->do_Flags & DOF_CHECKED)
                doi[index].doi_Flags |= DOIF_CHECKED;
            if (dop->do_Flags & DOF_CHECKABLE)
                doi[index].doi_Flags |= DOIF_CHECKABLE;
            if (dop->do_Flags & DOF_MUTUALEXCLUDE)
                doi[index].doi_Flags |= DOIF_MUTUALEXCLUDE;

            doi[index].doi_MutualExclude = dop->do_MutualExclude;

        // this bit does the subitem array... at the moment only
        // one level of subitems are supported.. it would be
        // a nice option to have more, so this part will have to be changed
        // to be recursive
            items = 0;
            subindex = 0;
        // subdop=DesktopBase->db_OperationList.lh_Head;
            subdop = (struct DesktopOperation *) dop->do_SubItems.lh_Head;
            if (subdop->do_Node.ln_Succ)
                items++;
            while (subdop->do_Node.ln_Succ)
            {
                if (subdop->do_Code & operationType)
                    items++;
                subdop = (struct DesktopOperation *) subdop->do_Node.ln_Succ;
            }

            if (items)
            {
                doi[index].doi_SubItems =
                    (struct DesktopOperationItem *)
                    AllocVec(sizeof(struct DesktopOperationItem) * (items),
                             MEMF_ANY);

                subdop = (struct DesktopOperation *) dop->do_SubItems.lh_Head;
                while (subdop->do_Node.ln_Succ)
                {
                    doi[index].doi_SubItems[subindex].doi_Code =
                        subdop->do_Code;
                    doi[index].doi_SubItems[subindex].doi_Number =
                        subdop->do_Number;
                    doi[index].doi_SubItems[subindex].doi_Name =
                        subdop->do_Name;
                    doi[index].doi_SubItems[subindex].doi_MutualExclude =
                        subdop->do_MutualExclude;
                    doi[index].doi_SubItems[subindex].doi_Flags = 0;
                    if (subdop->do_Flags & DOF_CHECKED)
                        doi[index].doi_SubItems[subindex].doi_Flags |=
                            DOIF_CHECKED;
                    if (subdop->do_Flags & DOF_CHECKABLE)
                        doi[index].doi_SubItems[subindex].doi_Flags |=
                            DOIF_CHECKABLE;
                    if (subdop->do_Flags & DOF_MUTUALEXCLUDE)
                        doi[index].doi_SubItems[subindex].doi_Flags |=
                            DOIF_MUTUALEXCLUDE;
                    doi[index].doi_SubItems[subindex].doi_SubItems = NULL;

                    subdop =
                        (struct DesktopOperation *) subdop->do_Node.ln_Succ;
                    subindex++;
                }

                doi[index].doi_SubItems[subindex].doi_Number = 0;
                doi[index].doi_SubItems[subindex].doi_Code = 0;
                doi[index].doi_SubItems[subindex].doi_Name = NULL;
            }
            else
                doi[index].doi_SubItems = NULL;

            index++;
        }

        dop = (struct DesktopOperation *) dop->do_Node.ln_Succ;
    }

    doi[index].doi_Number = 0;
    doi[index].doi_Code = 0;
    doi[index].doi_Name = NULL;

// kprintf("set doi[%d] to NULL\n", index);

// ///////////
/*
   { LONG numberWindowItems=0, subs=0;

   while(doi[numberWindowItems].doi_Code!=0 &&
   doi[numberWindowItems].doi_Name!=NULL) { kprintf("processing [%d] (addr):
   %d\n", numberWindowItems, doi[numberWindowItems].doi_Name);
   if(doi[numberWindowItems].doi_SubItems) { subs=0; kprintf("subitem.code
   is: %d\n", doi[numberWindowItems].doi_SubItems[subs].doi_Code);
   while(doi[numberWindowItems].doi_SubItems[subs].doi_Code!=0) { kprintf("
   processing sub (addr): %d\n",
   doi[numberWindowItems].doi_SubItems[subs].doi_Name); subs++;
   kprintf("subitem.code is: %d\n",
   doi[numberWindowItems].doi_SubItems[subs].doi_Code); } if(subs) subs++; }
   numberWindowItems++; } } 
 */
// ///////////


    return doi;

    AROS_LIBFUNC_EXIT
}              /* CreateWorkbenchObjectA */
