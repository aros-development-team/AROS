/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: Assign CLI command
    Lang: english
*/

#include <aros/config.h>

#if !(AROS_FLAVOUR & AROS_FLAVOUR_NATIVE)
#define ENABLE_RT 1
#endif

#include <stdio.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
#include <utility/tagitem.h>

#include <aros/rt.h>

static const char version[] = "$VER: assign 41.4 (24.7.1997)\n";

void dolist()
{
    struct DosList *dlist, *curlist;
    int count;

    dlist = LockDosList(LDF_VOLUMES|LDF_ASSIGNS|LDF_DEVICES|LDF_READ);
    VPrintf("Volumes:\n", NULL);
    curlist = dlist;
    while ((curlist = NextDosEntry(curlist, LDF_VOLUMES)))
    {
	VPrintf("%s\n", (IPTR *)&(curlist->dol_DevName));
	/* !!! mounted !!! */
    }
    VPrintf("\nDirectories:\n", NULL);
    curlist = dlist;
    while ((curlist = NextDosEntry(curlist, LDF_ASSIGNS)))
    {
	VPrintf(curlist->dol_DevName, NULL);
	for (count=15-strlen(curlist->dol_DevName); count>0; count--)
	    VPrintf(" ", NULL);
	VPrintf("\n", NULL); /* !!! print directory !!! */
    }
    /* !!! late/nonbinding !!! */
    VPrintf("\nDevices:\n", NULL);
    count = 0;
    curlist = dlist;
    while ((curlist = NextDosEntry(curlist, LDF_DEVICES)))
    {
	VPrintf("%s ", (IPTR *)&(curlist->dol_DevName));
	count++;
	if (count == 5)
	{
	    VPrintf("\n", NULL);
	    count = 0;
	}
    }
    if (count < 5)
	VPrintf("\n", NULL);
    UnLockDosList(LDF_VOLUMES|LDF_ASSIGNS|LDF_DEVICES|LDF_READ);
}


int doassign(STRPTR name, STRPTR target)
{
    int error = RETURN_OK;
    BPTR dir;

    dir=Lock(target,SHARED_LOCK);
    if(dir)
    {
	STRPTR s=name;
	while (*s)
	{
	    if (*s == ':')
	    {
		*s = 0;
		break;
	    }
	    s ++;
	}

	if (!AssignLock(name,dir))
	    error = RETURN_FAIL;
    } else
	error = RETURN_FAIL;
    return error;
}


int main (int argc, char ** argv)
{
    STRPTR args[3]={ NULL, NULL, NULL };
    struct RDArgs *rda;
    int error=RETURN_OK;

    RT_Init();

    rda=ReadArgs("NAME,TARGET,LIST/S",(IPTR *)args,NULL);
    if(rda!=NULL)
    {
	if (args[0] != NULL && args[1] != NULL)
	    error = doassign(args[0], args[1]);
	if (args[0] == NULL || args[2] != NULL)
	    dolist();
	FreeArgs(rda);
    }else
	error=RETURN_FAIL;
    if(error)
	PrintFault(IoErr(),"Assign");
    RT_Exit();
    return error;
}
