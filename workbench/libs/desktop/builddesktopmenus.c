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
#include "support.h"

#define DEBUG 1
#include <aros/debug.h>

/*****************************************************************************

    NAME */

        #include <proto/desktop.h>

        AROS_LH0(struct NewMenu*, BuildDesktopMenus,

/*  SYNOPSIS */

/*  LOCATION */
        struct DesktopBase *, DesktopBase, 11, Desktop)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

	struct DesktopOperationItem *doiD, *doiW, *doiI;
	LONG i=0, j=0;
	ULONG numberDesktopItems=0, numberWindowItems=0, numberIconItems=0;
	ULONG numberMenuItems;
	struct NewMenu *menuDat;

	doiD=GetMenuItemList(DOC_DESKTOPOP);
	if(doiD)
	{
		while(doiD[numberDesktopItems].doi_Code!=0 && doiD[numberDesktopItems].doi_Name!=NULL)
			numberDesktopItems++;
	}

	doiW=GetMenuItemList(DOC_WINDOWOP);
	if(doiW)
	{
		while(doiW[numberWindowItems].doi_Code!=0 && doiW[numberWindowItems].doi_Name!=NULL)
			numberWindowItems++;
	}

	doiI=GetMenuItemList(DOC_ICONOP);
	if(doiI)
	{
		while(doiI[numberIconItems].doi_Code!=0 && doiI[numberIconItems].doi_Name!=NULL)
			numberIconItems++;
	}

	numberMenuItems=numberDesktopItems+(numberDesktopItems>0?1:0)+
					numberWindowItems+(numberWindowItems>0?1:0)+
					numberIconItems+(numberIconItems>0?1:0);

	menuDat=(struct NewMenu*)AllocVec(sizeof(struct NewMenu)*(numberMenuItems+1), MEMF_ANY);

	if(numberDesktopItems>0)
	{
		menuDat[i].nm_Type=NM_TITLE;
		menuDat[i].nm_Label="AROS";
		menuDat[i].nm_CommKey=0;
		menuDat[i].nm_Flags=0;
		menuDat[i].nm_MutualExclude=0;
		menuDat[i].nm_UserData=0;

		i++;
		j=0;
		while(doiD[j].doi_Code!=0 && doiD[j].doi_Name!=NULL)
		{
			menuDat[i].nm_Type=NM_ITEM;
			menuDat[i].nm_Label=doiD[j].doi_Name;
			menuDat[i].nm_CommKey=0;
			menuDat[i].nm_Flags=0;
			menuDat[i].nm_MutualExclude=0;
			menuDat[i].nm_UserData=doiD[j].doi_Code;
			i++;
			j++;
		}
	}

	if(numberWindowItems>0)
	{
		menuDat[i].nm_Type=NM_TITLE;
		menuDat[i].nm_Label="Window";
		menuDat[i].nm_CommKey=0;
		menuDat[i].nm_Flags=0;
		menuDat[i].nm_MutualExclude=0;
		menuDat[i].nm_UserData=0;

		j=0;
		i++;
		while(doiW[j].doi_Code!=0 && doiW[j].doi_Name!=NULL)
		{
			menuDat[i].nm_Type=NM_ITEM;
			menuDat[i].nm_Label=doiW[j].doi_Name;
			menuDat[i].nm_CommKey=0;
			menuDat[i].nm_Flags=0;
			menuDat[i].nm_MutualExclude=0;
			menuDat[i].nm_UserData=doiW[j].doi_Code;
			i++;
			j++;
		}
	}

	if(numberIconItems>0)
	{
		menuDat[i].nm_Type=NM_TITLE;
		menuDat[i].nm_Label="Icon";
		menuDat[i].nm_CommKey=0;
		menuDat[i].nm_Flags=0;
		menuDat[i].nm_MutualExclude=0;
		menuDat[i].nm_UserData=0;

		i++;
		j=0;
		while(doiI[j].doi_Code!=0 && doiI[j].doi_Name!=NULL)
		{
			menuDat[i].nm_Type=NM_ITEM;
			menuDat[i].nm_Label=doiI[j].doi_Name;
			menuDat[i].nm_CommKey=0;
			menuDat[i].nm_Flags=0;
			menuDat[i].nm_MutualExclude=0;
			menuDat[i].nm_UserData=doiI[j].doi_Code;
			i++;
			j++;
		}
	}

	menuDat[i].nm_Type=NM_END;
	menuDat[i].nm_Label=NULL;
	menuDat[i].nm_CommKey=0;
	menuDat[i].nm_Flags=0;
	menuDat[i].nm_MutualExclude=0;
	menuDat[i].nm_UserData=0;

	return menuDat;

    AROS_LIBFUNC_EXIT
} /* BuildDesktopMenus */




