/*
    Copyright (C) 1995-2000 AROS - The Amiga Research OS
    $Id$

    Desc: autoinit library - automatic library opening/closing handling
    Lang: english
*/

#include <proto/exec.h>
#include <dos/dos.h>
#include <aros/symbolsets.h>

static void __showerror_fake(int code, char *title, char *format, ...) {};
void (*__showerrorptr)(int code, char *title, char *format, ...) __attribute__((weak)) = &__showerror_fake;

DEFINESET(LIBS);

int set_open_libraries(void)
{
    struct libraryset **set = (struct libraryset **)SETNAME(LIBS);
    int n = 1;
    struct Process *me = (struct Process *)FindTask(0);

    while(set[n])
    {
	*(set[n]->baseptr) = OpenLibrary(set[n]->name, *set[n]->versionptr);
	if (!*(set[n]->baseptr))
	{
	    __showerrorptr(ERROR_INVALID_RESIDENT_LIBRARY,
	                "Library error",
	                "Couldn't open version %ld of library \"%s\".",
		        *set[n]->versionptr, set[n]->name);
	    return 20;
	}

        if (set[n]->postopenfunc)
	{
	    int ret = set[n]->postopenfunc();
	    if (ret)
	    {
	    	__showerrorptr(ERROR_INVALID_RESIDENT_LIBRARY,
		            "Library error",
	                    "Couldn't initialize library \"%s\".",
		            set[n]->name);
	        return ret;
	    }
        }

        n++;
    }
    return 0;
}

void set_close_libraries(void)
{
    struct libraryset **set = (struct libraryset **)SETNAME(LIBS);
    int	n = ((int *)set)[0];

    while (n)
    {
	if (*(set[n]->baseptr))
	{
	    if (set[n]->preclosefunc) set[n]->preclosefunc();
	    CloseLibrary(*(set[n]->baseptr));
        }

	n--;
    }
}
