/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: english
*/

/******************************************************************************


    NAME

        ResList

    SYNOPSIS

        (N/A)

    LOCATION

        C:

    FUNCTION

        Prints a list of all resources.

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

#include <exec/memory.h>
#include <exec/tasks.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <proto/dos.h>

const TEXT version[] = "$VER: reslist 41.2 (20.7.2001)\n";

struct res
{
    STRPTR name;
    APTR address;
};

static int addres(struct Node *r, struct res **l, STRPTR *e)
{
    STRPTR s1, s2;

    (*l)->address = r;

    s1 = r->ln_Name;

    if(s1!=NULL)
    {
        s2=s1;
        while(*s2++)
            ;
        while(s2>s1)
        {
            if(*e<=(STRPTR)l)
                return 0;
            *--(*e)=*--s2;
        }
    }
    if((STRPTR)(*l+1)>*e)
        return 0;

    (*l)->name=*e;
    ++*l;

    return 1;
}

static int fillbuffer(struct res **buffer, IPTR size)
{
    STRPTR end=(STRPTR)*buffer+size;
    struct Node *r;
    Forbid();
    for(r=(struct Node *)SysBase->ResourceList.lh_Head;
        r->ln_Succ!=NULL;
        r=(struct Node *)r->ln_Succ)
        if(!addres(r,buffer,&end))
        {
            Permit();
            return 0;
        }
    Permit();
    return 1;
}

int __nocommandline;

int main(void)
{
    IPTR size;
    struct res *buffer,*ress,*ress2;
    for(size=2048;;size+=2048)
    {
        buffer=AllocVec(size,MEMF_ANY);
        if(buffer==NULL)
        {
            FPuts(Output(),"Not Enough memory for library buffer\n");
            return 20;
        }
        ress=buffer;
        if(fillbuffer(&ress,size))
        {
	    FPuts(Output(),"address\t\tname\n"
                           "------------------------------------------------------------\n");
	    for(ress2=buffer;ress2<ress;ress2++)
	    {
		IPTR args[2];
		args[0] = (IPTR)ress2->address;
		args[1] = (IPTR)ress2->name;

		VPrintf("0x%08.lx\t%s\n", args);
	    }
	    FreeVec(buffer);
            return 0; 
        }
        FreeVec(buffer);
    }
}
