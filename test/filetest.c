/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.4  1998/10/20 16:46:38  hkiel
    Amiga Research OS

    Revision 1.3  1998/04/13 22:50:02  hkiel
    Include <proto/exec.h>

    Revision 1.2  1996/08/01 17:41:39  digulla
    Added standard header for all files

    Desc:
    Lang:
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
