/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>

/* Internal function __libfindandopen will only open a library when it is
   already in the list of open libraries
*/
static struct Library *__libfindandopen(const char *libname, int version)
{
    struct Node *found;

    Forbid();
    found = FindName(&SysBase->LibList, libname);
    Permit();

    return (found != NULL) ? OpenLibrary(libname, version) : NULL;
}
