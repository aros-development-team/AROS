/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <exec/io.h>
#include <exec/memory.h>
#include <devices/trackdisk.h>

#include <proto/exec.h>

#include <sys/time.h>

#include <stdio.h>
#include <stdlib.h>

/* 
   Yes, I know. This is the ugliest hack you have ever seen.
   However, it does its job, and you are not supposed to be running
   this anyways :)
*/

UBYTE *buffer;
UBYTE *buff2;
struct MsgPort *mp;
struct IOExtTD *io;

void verify( void )
{
    ULONG foo,bar;
    
    printf("Comparing\n");
    for (foo=0;foo<256;foo++)
    {
	for (bar=0;bar<512;bar++)
	{
	    if (buffer[(foo*512)+bar] != buff2[(foo*512)+bar])
	    {
		printf("Mismatch in sector %d\n",(int)bar);
	    }
	}
    }
}

int main ( int argc, char *argv[] )
{
    ULONG x,y;
    struct timeval tv1,tv2;
    double elapsed;
    ULONG base;

    if (argc != 2)
    {
	printf("Usage: testide <startblock>\n\n");
	printf("Be warned, this tool WILL irrevocably destroy data on\n");
	printf("the primary master IDE hard disk. Do not run this\n");
	printf("on a computer holding any important data whatsoever.\n");
	return 0;
    }

    base = atoi(argv[1])*512;	
    
    printf("ide.device test tool\n");
    printf("Allocating two 128 KiB buffers\n");

    buffer = AllocMem(131072,MEMF_PUBLIC);
    buff2 = AllocMem(131072,MEMF_PUBLIC);

    printf("Initializing buffer\n");
    for (x=0;x<256;x++)
	for (y=0;y<512;y++)
	    buffer[(x*512)+y] = x;

    printf("Creating MsgPort\n");
    mp = CreateMsgPort();
    if (!mp)
    {
	printf("Failed, aborting\n");
	return 1;
    }

    printf("Creating IORequest\n");
    io = (struct IOExtTD *)CreateIORequest(mp,sizeof(struct IOExtTD));
    if (!io)
    {
	printf("Failed, aborting\n");
	return 1;
    }

    printf("Opening ide.device\n");
    if (OpenDevice("ide.device",0L,(struct IORequest *)io,0L))
    {
	printf("Failed, aborting\n");
	return 1;
    }
    
    printf("Writing single blocks\n");
    for (x=0;x<256;x++)
    {
	io->iotd_Req.io_Length = 512;
	io->iotd_Req.io_Data = (buffer+(x*512));
	io->iotd_Req.io_Offset = (x*512)+base;
	io->iotd_Req.io_Command = CMD_WRITE;
	DoIO((struct IORequest *)io);
    }

    printf("Reading single blocks\n");
    for (x=0;x<256;x++)
    {
	io->iotd_Req.io_Length = 512;
	io->iotd_Req.io_Data = (buff2+(x*512));
	io->iotd_Req.io_Offset = (x*512)+base;
	io->iotd_Req.io_Command = CMD_READ;
	DoIO((struct IORequest *)io);
    }

    verify();

    printf("Writing entire buffer\n");
    io->iotd_Req.io_Length = 131072;
    io->iotd_Req.io_Data = buffer;
    io->iotd_Req.io_Offset = base;
    io->iotd_Req.io_Command = CMD_WRITE;
    DoIO((struct IORequest *)io);

    verify();

    printf("Writing single blocks\n");
    for (x=0;x<256;x++)
    {
	io->iotd_Req.io_Length = 512;
	io->iotd_Req.io_Data = (buffer+(x*512));
	io->iotd_Req.io_Offset = (x*512)+base;
	io->iotd_Req.io_Command = CMD_WRITE;
	DoIO((struct IORequest *)io);
    }
    
    printf("Reading entire buffer\n");
    io->iotd_Req.io_Length = 131072;
    io->iotd_Req.io_Data = buff2;
    io->iotd_Req.io_Offset = base;
    io->iotd_Req.io_Command = CMD_READ;
    DoIO((struct IORequest *)io);

    verify();

    printf("Benching\n");

    gettimeofday(&tv1,NULL);
    for (x=0;x<80;x++)
    {
	io->iotd_Req.io_Length = 131072;
	io->iotd_Req.io_Data = (buffer+(x*512));
	io->iotd_Req.io_Offset = (x*512)+base;
	io->iotd_Req.io_Command = CMD_READ;
	DoIO((struct IORequest *)io);
    }
    gettimeofday(&tv2,NULL);
    elapsed = ((double)(((tv2.tv_sec * 1000000) + tv2.tv_usec) - ((tv1.tv_sec * 1000000) + tv1.tv_usec)))/1000000.;

    printf(" Read 10 MiB in %f seconds (%f MiB/s\n",elapsed,(10/elapsed));

    gettimeofday(&tv1,NULL);
    for (x=0;x<80;x++)
    {
	io->iotd_Req.io_Length = 131072;
	io->iotd_Req.io_Data = (buffer+(x*512));
	io->iotd_Req.io_Offset = (x*512)+base;
	io->iotd_Req.io_Command = CMD_WRITE;
	DoIO((struct IORequest *)io);
    }
    gettimeofday(&tv2,NULL);
    elapsed = ((double)(((tv2.tv_sec * 1000000) + tv2.tv_usec) - ((tv1.tv_sec * 1000000) + tv1.tv_usec)))/1000000.;

    printf("Wrote 10 MiB in %f seconds (%f MiB/s\n",elapsed,(10/elapsed));
    return 0;
}

