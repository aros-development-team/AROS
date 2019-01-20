/*
    Copyright © 2019, The AROS Development Team. All rights reserved.
    $Id$
*/
 
#include <aros/debug.h>
 
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/arosx.h>
 
struct Library *AROSXBase;
 
int main (int argc, char *argv[]) {
 
    AROSXBase = OpenLibrary("arosx.library", 0);
 
    if (AROSXBase) {
        Printf("arosx.library opened, version %ld.%ld\n", AROSXBase->lib_Version, AROSXBase->lib_Revision);

        CloseLibrary(AROSXBase);
    } else {
        PutStr("arosx.library failed to open.\n");
    }
 
    return 0;
}
