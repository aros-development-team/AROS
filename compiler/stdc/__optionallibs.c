/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>
#include <proto/locale.h>

/* Internal function __libfindandopen will only open a library when it is
   already in the list of open libraries
*/
struct LocaleBase *LocaleBase = NULL;

static struct Library *__libfindandopen(const char *libname, int version)
{
    struct Node *found;

    Forbid();
    found = FindName(&SysBase->LibList, libname);
    Permit();

    return (found != NULL) ? OpenLibrary(libname, version) : NULL;
}

int __locale_available(void)
{
    if (LocaleBase == NULL)
        LocaleBase = (struct LocaleBase *)__libfindandopen("locale.library", 0);

    return LocaleBase != NULL;
}
