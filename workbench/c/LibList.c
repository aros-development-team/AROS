/*
    Copyright © 1995-2015, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: english
*/

/******************************************************************************


    NAME

        LibList

    SYNOPSIS

        (N/A)

    LOCATION

        C:

    FUNCTION

        Print list of all libraries.

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

const TEXT version[] = "$VER: LibList 41.3 (11.3.2015)\n";

struct lib
{
    STRPTR name;
    APTR address;
    WORD version;
    WORD revision;
    WORD opencnt;
    UBYTE flags;
};

static int addlib(struct Library *lib, struct lib **l, STRPTR *e)
{
    STRPTR s1, s2;

    (*l)->address = lib;
    (*l)->version = lib->lib_Version;
    (*l)->revision = lib->lib_Revision;
    (*l)->opencnt = lib->lib_OpenCnt;
    (*l)->flags    = lib->lib_Flags;

    s1 = lib->lib_Node.ln_Name;

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

static int fillbuffer(struct lib **buffer, IPTR size)
{
    STRPTR end=(STRPTR)*buffer+size;
    struct Library *lib;
#if defined(__AROSPLATFORM_SMP__)
    void *ExecLockBase = OpenResource("execlock.resource");
#endif

#if defined(__AROSPLATFORM_SMP__)
    if (ExecLockBase)
        ObtainSystemLock(&SysBase->LibList, SPINLOCK_MODE_READ, LOCKF_FORBID);
    else
        Forbid();
#else
    Forbid();
#endif
    for(lib=(struct Library *)SysBase->LibList.lh_Head;
        lib->lib_Node.ln_Succ!=NULL;
        lib=(struct Library *)lib->lib_Node.ln_Succ)
    {
        if(!addlib(lib,buffer,&end))
        {
#if defined(__AROSPLATFORM_SMP__)
            if (ExecLockBase)
                ReleaseSystemLock(&SysBase->LibList, LOCKF_FORBID);
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
        ReleaseSystemLock(&SysBase->LibList, LOCKF_FORBID);
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
    struct lib *buffer,*libs,*libs2;
    LONG error=RETURN_OK;
    
    for(size=2048;;size+=2048)
    {
        buffer=AllocVec(size,MEMF_ANY);
        if(buffer==NULL)
        {
            FPuts(Output(),"Not Enough memory for library buffer\n");
	    SetIoErr(ERROR_NO_FREE_STORE);
	    error = RETURN_FAIL;
            break;
        }
        libs=buffer;
        if(fillbuffer(&libs,size))
        {
#if (__WORDSIZE == 64)
	    FPuts(Output(),"       Address  Version  Rev  OpenCnt  Flags  Name\n");
#else
	    FPuts(Output(),"   Address  Version  Rev  OpenCnt  Flags  Name\n");
#endif
	    for(libs2=buffer;libs2<libs;libs2++)
	    {
#if (__WORDSIZE == 64)
        Printf("0x%012.ix  %7ld %4ld  %7ld   0x%02lx  %s\n",
#else
        Printf("0x%08.ix  %7ld %4ld  %7ld   0x%02lx  %s\n",
#endif
		        libs2->address, (ULONG)libs2->version,
		        (ULONG)libs2->revision,
		        (ULONG)libs2->opencnt,
		        (ULONG)libs2->flags,
		        libs2->name);
                if(SetSignal(0L,SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C)
                {
                    error = RETURN_FAIL;
                    SetIoErr(ERROR_BREAK);
                    break;
                }
	    }
	    FreeVec(buffer);
            break; 
        }
        FreeVec(buffer);
    }

    if (error != RETURN_OK)
    {
	PrintFault(IoErr(), NULL);
    }
    
    return error;
    
}
