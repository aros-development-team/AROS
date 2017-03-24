/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <exec/resident.h>
#include <exec/execbase.h>
#include <exec/io.h>
#include <aros/libcall.h>
#include <clib/exec_protos.h>
#include <stdio.h>
#include "dummydev_gcc.h"

extern const struct Resident resident;

int main(void)
{
    int r1=0,r2=0,r3=0,e1=0,e2=0,e31=0,e32=0;
    struct dummybase *dummy;

    if(!(resident.rt_Flags&RTF_AUTOINIT))
	return 20;
    dummy=(struct dummybase *)MakeLibrary(((APTR *)resident.rt_Init)[1],
					  ((APTR *)resident.rt_Init)[2],
					  ((APTR *)resident.rt_Init)[3],
					  (IPTR)((APTR *)resident.rt_Init)[0],1);
    if(dummy!=NULL)
    {
	Forbid();
	if(FindName(&SysBase->DeviceList,(STRPTR)&dummy->device.dd_Library.lib_Node.ln_Name)==NULL)
	{
	    AddDevice(&dummy->device);
	    {
		struct MsgPort *port;
		struct dummyrequest *dr;

		port=CreateMsgPort();
		if(port!=NULL)
		{
		    dr=(struct dummyrequest *)CreateIORequest(port,sizeof(struct dummyrequest));
		    if(dr!=NULL)
		    {
			if(!OpenDevice("dummy.device",0,(struct IORequest *)dr,0))
			{
			    dr->iorequest.io_Command=0x1;
			    DoIO((struct IORequest *)dr);
			    r1=dr->id;
			    e1=dr->iorequest.io_Error;
			    dr->iorequest.io_Command=0x1;
			    SendIO((struct IORequest *)dr);
			    WaitIO((struct IORequest *)dr);
			    r2=dr->id;
			    e2=dr->iorequest.io_Error;
			    dr->iorequest.io_Command=0x1;
			    SendIO((struct IORequest *)dr);
			    e31=AbortIO((struct IORequest *)dr);
			    WaitIO((struct IORequest *)dr);
			    r3=dr->id;
			    e32=dr->iorequest.io_Error;

			    CloseDevice((struct IORequest *)dr);
			}
			DeleteIORequest((struct IORequest *)dr);
		    }
		    DeleteMsgPort(port);
		}
	    }
	    /* Don't use RemLibrary() - it calls UnLoadSeg(). */
	    Remove(&dummy->device.dd_Library.lib_Node);
	}
	Permit();
	FreeMem((char *)dummy-dummy->device.dd_Library.lib_NegSize,
		dummy->device.dd_Library.lib_NegSize+dummy->device.dd_Library.lib_PosSize);

	printf("Synchronous:\t%d (%d)\n",r1,e1);
	printf("Asynchronous:\t%d (%d)\n",r2,e2);
	printf("Aborted:\t%d (%d,%d)\n",r3,e31,e32);
    }
    return 0;
}
