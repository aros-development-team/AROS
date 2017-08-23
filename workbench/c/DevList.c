/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: english
*/

/******************************************************************************


    NAME

        DevList

    SYNOPSIS

        (N/A)

    LOCATION

        C:

    FUNCTION

        Prints a list of all devices.

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
#include <exec/devices.h>
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <proto/dos.h>

#if defined(__AROSPLATFORM_SMP__)
#include <aros/types/spinlock_s.h>
#include <proto/execlock.h>
#include <resources/execlock.h>
#endif

const TEXT version[] = "$VER: DevList 41.1 (14.3.1997)\n";

struct dev
{
    STRPTR name;
    APTR address;
    WORD version;
    WORD revision;
    ULONG flags;
    WORD opencnt;
};

static int adddev(struct Device *dev, struct dev **l, STRPTR *e)
{
    STRPTR s1, s2;

    (*l)->address  = dev;
    (*l)->version  = dev->dd_Library.lib_Version;
    (*l)->revision = dev->dd_Library.lib_Revision;
    (*l)->opencnt  = dev->dd_Library.lib_OpenCnt;
    (*l)->flags    = dev->dd_Library.lib_Flags;

    s1 = dev->dd_Library.lib_Node.ln_Name;

    if(s1!=NULL)
    {
        s2=s1;
        while(*s2++)
            ;
        while(s2>s1)
        {
            if(*e<=(STRPTR)(*l+2))
                return 0;
            *--(*e)=*--s2;
        }
    }
    if((STRPTR)(*l+2)>*e)
        return 0;

    (*l)->name=*e;
    ++*l;

    return 1;
}

static int fillbuffer(struct dev **buffer, IPTR size)
{
    STRPTR end=(STRPTR)*buffer+size;
    struct Device *dev;
#if defined(__AROSPLATFORM_SMP__)
    void *ExecLockBase = OpenResource("execlock.resource");
#endif

#if defined(__AROSPLATFORM_SMP__)
    if (ExecLockBase)
        ObtainSystemLock(&SysBase->DeviceList, SPINLOCK_MODE_READ, LOCKF_FORBID);
    else
        Forbid();
#else
    Forbid();
#endif
    for(dev=(struct Device *)SysBase->DeviceList.lh_Head;
        dev->dd_Library.lib_Node.ln_Succ!=NULL;
        dev=(struct Device *)dev->dd_Library.lib_Node.ln_Succ)
        if(!adddev(dev,buffer,&end))
        {
#if defined(__AROSPLATFORM_SMP__)
            if (ExecLockBase)
                ReleaseSystemLock(&SysBase->DeviceList, LOCKF_FORBID);
            else
                Permit();
#else
            Permit();
#endif
            return 0;
        }
#if defined(__AROSPLATFORM_SMP__)
            if (ExecLockBase)
                ReleaseSystemLock(&SysBase->DeviceList, LOCKF_FORBID);
            else
                Permit();
#else
            Permit();
#endif
    return 1;
}

int __nocommandline = 1;

int main(void)
{
    IPTR size;
    struct dev *buffer,*devs,*devs2;
    LONG error=RETURN_OK;

    for(size=2048;;size+=2048)
    {
        buffer=AllocVec(size,MEMF_ANY);
        if(buffer==NULL)
        {
            FPuts(Output(),"Not Enough memory for device buffer\n");
            return 20;
        }
        devs=buffer;
        if(fillbuffer(&devs,size))
        {
#if (__WORDSIZE == 64)
        FPuts(Output(),"       Address  Version  Rev  OpenCnt  Flags  Name\n");
#else
        FPuts(Output(),"   Address  Version  Rev  OpenCnt  Flags  Name\n");
#endif
	    for(devs2=buffer;devs2<devs;devs2++)
	    {
#if (__WORDSIZE == 64)
		Printf("0x%012.ix  %7ld %4ld  %7ld   0x%02lx  %s\n",
#else
		Printf("0x%08.ix  %7ld %4ld  %7ld   0x%02lx  %s\n",
#endif
		        devs2->address, devs2->version,
		        devs2->revision, devs2->opencnt,
		        devs2->flags, devs2->name);
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
    return(error);
}
