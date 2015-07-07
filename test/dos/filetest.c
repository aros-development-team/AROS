/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/dos.h>
#include <clib/exec_protos.h>
#include <dos/dos.h>
#include <clib/dos_protos.h>
#include <stdio.h>

static void hexdump(UBYTE *data, LONG len)
{
    LONG i;

    for (i = 0; i < len; i++)
        printf("%02X ", data[i]);

    printf("\n");
}

int main(int argc, char **argv)
{
    BPTR file;
    UBYTE buffer[20];
    LONG r1=0,r2=0,r3=0,r4=0;
    char *name = "testfile";
    
    if (argc > 1)
	name = argv[1];

    file=Open(name, MODE_NEWFILE);
    if(file)
    {
	r1=Write(file,"hello, world\n",13);
	r2=Seek(file,0,OFFSET_BEGINNING);
	r3=Read(file,buffer,19);
	r4=Close(file);
    }
    if(r3>=0)
	buffer[r3]=0;

    printf("Results: %d %d %d %d \'%s\'\n",(int)r1,(int)r2,(int)r3,(int)r4,buffer);
    hexdump(buffer, r3);
    
    return 0;
}
