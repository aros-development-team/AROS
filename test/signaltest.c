/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.4  1998/10/20 16:46:39  hkiel
    Amiga Research OS

    Revision 1.3  1998/04/13 22:50:02  hkiel
    Include <proto/exec.h>

    Revision 1.2  1996/08/01 17:41:40  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#include <proto/exec.h>
#include <clib/exec_protos.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
    int sig[32];
    int i;

    for(i=0;i<32;i++)
    {
	sig[i]=AllocSignal(-1);
	printf("%d\n",sig[i]);
    }

    for(i=0;i<32;i++)
	FreeSignal(sig[i]);

    for(i=0;i<32;i++)
    {
	sig[i]=AllocSignal(i);
	printf("%d\n",sig[i]);
    }

    for(i=0;i<32;i++)
	FreeSignal(sig[i]);

    return 0;
}
