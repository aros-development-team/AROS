/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: Assign CLI command
    Lang: english
*/

#include <stdio.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
#include <utility/tagitem.h>

static const char version[] = "$VER: assign 41.2 (4.7.1997)\n";

void dolist()
{
    struct DosList *dlist, *curlist;
    int count;

    dlist = LockDosList(LDF_VOLUMES|LDF_ASSIGNS|LDF_DEVICES|LDF_READ);
    printf("Volumes:\n");
    curlist = dlist;
    while ((curlist = NextDosEntry(curlist, LDF_VOLUMES)))
    {
        printf("%s\n", curlist->dol_DevName);
	/* !!! mounted !!! */
    }
    printf("\nDirectories:\n");
    curlist = dlist;
    while ((curlist = NextDosEntry(curlist, LDF_ASSIGNS)))
    {
        printf(curlist->dol_DevName);
	for (count=15-strlen(curlist->dol_DevName); count>0; count--)
	    printf(" ");
	printf("\n"); /* !!! print directory !!! */
    }
    /* !!! late/nonbinding !!! */
    printf("\nDevices:\n");
    count = 0;
    curlist = dlist;
    while ((curlist = NextDosEntry(curlist, LDF_DEVICES)))
    {
        printf("%s ", curlist->dol_DevName);
	count++;
	if (count == 5)
	{
	    printf("\n");
	    count = 0;
	}
    }
    if (count < 5)
        printf("\n");
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
	while(*s)
	    if((*s)++==':')
		s[-1]=0;
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
    return error;
}
