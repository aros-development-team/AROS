/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/dos.h>
#include <clib/exec_protos.h>
#include <dos/dos.h>
#include <clib/dos_protos.h>
#include <stdio.h>

int main(void)
{
    BPTR file;
    UBYTE buffer[20];
    LONG r1=0,r2=0,r3=0,r4=0;

    file=Open("testfile",MODE_NEWFILE);
    if(file)
    {
	r1=Write(file,"hello, world\n",13);
	r2=Seek(file,0,OFFSET_BEGINNING);
	r3=Read(file,buffer,19);
	r4=Close(file);
    }
    if(r3>=0)
	buffer[r3]=0;
    printf("%ld %ld %ld %ld \'%s\'\n",r1,r2,r3,r4,buffer);
    return 0;
}
