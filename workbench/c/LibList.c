/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: english
*/

#include <exec/memory.h>
#include <exec/tasks.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <proto/dos.h>

static const char version[] = "$VER: liblist 41.1 (14.3.1997)\n";

struct lib
{
    STRPTR name;
    APTR address;
    WORD version;
    WORD opencnt;
};

static int addlib(struct Library *lib, struct lib **l, STRPTR *e)
{
    STRPTR s1, s2;

    (*l)->address = lib;
    (*l)->version = lib->lib_Version;
    (*l)->opencnt = lib->lib_OpenCnt;

    s1 = lib->lib_Node.ln_Name;

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

static int fillbuffer(struct lib **buffer, IPTR size)
{
    STRPTR end=(STRPTR)*buffer+size;
    struct Library *lib;
    Forbid();
    for(lib=(struct Library *)SysBase->LibList.lh_Head;
        lib->lib_Node.ln_Succ!=NULL;
        lib=(struct Library *)lib->lib_Node.ln_Succ)
        if(!addlib(lib,buffer,&end))
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
    struct lib *buffer,*libs,*libs2;
    for(size=2048;;size+=2048)
    {
        buffer=AllocVec(size,MEMF_ANY);
        if(buffer==NULL)
        {
            FPuts(Output(),"Not Enough memory for library buffer\n");
            return 20;
        }
        libs=buffer;
        if(fillbuffer(&libs,size))
        {
	    FPuts(Output(),"address\t\tversion\topencnt\tname\n"
                           "------------------------------------------------------------\n");
	    for(libs2=buffer;libs2<libs;libs2++)
	    {
		IPTR args[4];
		args[0] = (IPTR)libs2->address;
		args[1] = (IPTR)libs2->version;
		args[2] = (IPTR)libs2->opencnt;
		args[3] = (IPTR)libs2->name;

		VPrintf("0x%08.lx\t%ld\t%ld\t%s\n", args);
	    }
	    FreeVec(buffer);
            return 0; 
        }
        FreeVec(buffer);
    }
}
