/*
    Copyright © 1995-2015, The AROS Development Team. All rights reserved.
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

#if defined(__AROSPLATFORM_SMP__)
#include <aros/types/spinlock_s.h>
#include <proto/execlock.h>
#include <resources/execlock.h>
#endif

const TEXT version[] = "$VER: reslist 41.3 (11.3.2015)\n";

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
            if(*e<=(STRPTR)*l)
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
#if defined(__AROSPLATFORM_SMP__)
	void *ExecLockBase = OpenResource("execlock.resource");
#endif
    STRPTR end=(STRPTR)*buffer+size;
    struct Node *r;
#if defined(__AROSPLATFORM_SMP__)
    if (ExecLockBase)
        ObtainSystemLock(&SysBase->ResourceList, SPINLOCK_MODE_READ, LOCKF_FORBID);
    else
        Forbid();
#else
    Forbid();
#endif
    for(r=(struct Node *)SysBase->ResourceList.lh_Head;
        r->ln_Succ!=NULL;
        r=(struct Node *)r->ln_Succ)
    {
        if(!addres(r,buffer,&end))
        {
#if defined(__AROSPLATFORM_SMP__)
            if (ExecLockBase)
                ReleaseSystemLock(&SysBase->ResourceList, LOCKF_FORBID);
            else
                Permit();
#else
            Permit();
#endif
            return 0;
        }
    }
#if defined(__AROSPLATFORM_SMP__)
    if (ExecLockBase)
        ReleaseSystemLock(&SysBase->ResourceList, LOCKF_FORBID);
    else
        Permit();
#else
    Permit();
#endif
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
#if (__WORDSIZE == 64)
	    FPuts(Output(),"       Address  Name\n");
#else
	    FPuts(Output(),"   Address  Name\n");
#endif
	    for(ress2=buffer;ress2<ress;ress2++)
	    {
#if (__WORDSIZE == 64)
                Printf("0x%012.ix  %s\n",
#else
                Printf("0x%08.ix  %s\n",
#endif
                    ress2->address, ress2->name);
	    }
	    FreeVec(buffer);
            return 0; 
        }
        FreeVec(buffer);
    }
}
