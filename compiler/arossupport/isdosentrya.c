/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/arossupport.h>
#include <proto/dos.h>
#include <proto/utility.h>

#include <aros/system.h>
#include <dos/dos.h>

#include <dos/dosextens.h>
#include <exec/types.h>
#include <utility/utility.h>

#define BUFFER_SIZE   100

/*****************************************************************************

    NAME */

        BOOL IsDosEntryA(

/*  SYNOPSIS */

        char  * Name,
        ULONG   Flags)

/*  LOCATION */

/*  FUNCTION

        There is a need in file/directory processing where an application
        may need to determine whether a path is just a volume/device or
        assignment name.

    INPUTS

        Name  - The path to test.

        Flags - Any combination of the following:

                LDF_ASSIGNS
                LDF_DEVICES
                LDF_VOLUMES

    RESULT

        Boolean True or False.

    NOTES

        Requires the programmer to open the utility.library and initialise
        UtilityBase.

        In future releases the buffer size will be set via a taglist.

    EXAMPLE

        BOOL Success;

        ...

        Success = IsDosEntryA("Work:", LDF_VOLUMES)
        if (Success == TRUE)
        {
          ...
        }  

    BUGS

    SEE ALSO

        <dos/dosextens.h>

    INTERNALS

    HISTORY

        27-Jul-1997     laguest     Initial inclusion into the AROS tree

******************************************************************************/
{
  struct DosList *DList;
  char           *DLName;
  int             Position;
  int             Success;
  BOOL            ReturnValue;
  char            Buffer[BUFFER_SIZE + 1];

  ReturnValue = FALSE;

  Position = SplitName(Name, ':', &Buffer[0], 0, BUFFER_SIZE + 1);
  if (Position != -1 && Name[Position] == NULL)
  {
    DList = AttemptLockDosList(Flags | LDF_READ);
    if (DList != NULL)
    {
      DList = NextDosEntry(DList, Flags);
      while (DList != NULL && ReturnValue == FALSE)
      {
        DLName = AROS_BSTR_ADDR(DList->dol_Name);

        Success = Strnicmp(DLName, &Buffer[0], Position - 1);
        if (Success == 0)
        {
          ReturnValue = TRUE;
        }
        
        DList = NextDosEntry(DList, Flags);
      }
      
      UnLockDosList(Flags | LDF_READ);
    }
  }

  return (ReturnValue);

} /* IsDosEntry */
