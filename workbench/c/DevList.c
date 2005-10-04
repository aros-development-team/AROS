/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: english
*/

#include <exec/memory.h>
#include <exec/tasks.h>
#include <exec/execbase.h>
#include <exec/devices.h>
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <proto/dos.h>

static const char version[] = "$VER: liblist 41.1 (14.3.1997)\n";

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
    Forbid();
    for(dev=(struct Device *)SysBase->DeviceList.lh_Head;
        dev->dd_Library.lib_Node.ln_Succ!=NULL;
        dev=(struct Device *)dev->dd_Library.lib_Node.ln_Succ)
        if(!adddev(dev,buffer,&end))
        {
            Permit();
            return 0;
        }
    Permit();
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
	    FPuts(Output(),"address\t\tversion\trev\topencnt\tflags\tname\n"
                           "------------------------------------------------------------\n");
	    for(devs2=buffer;devs2<devs;devs2++)
	    {
		IPTR args[6];
		args[0] = (IPTR)devs2->address;
		args[1] = (IPTR)devs2->version;
                args[2] = (IPTR)devs2->revision;
		args[3] = (IPTR)devs2->opencnt;
                args[4] = (IPTR)devs2->flags;
		args[5] = (IPTR)devs2->name;

		VPrintf("0x%08.lx\t%ld\t%ld\t%ld\t\t0x%lx\t%s\n", args);
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
