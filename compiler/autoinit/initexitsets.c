/*
    Copyright © 2009 - 2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: autoinit library - handle init and exit symolsets
*/
#include <aros/startup.h>
#include <aros/symbolsets.h>
#include <setjmp.h>

#define DEBUG 0
#include <aros/debug.h>

int __noinitexitsets __attribute__((weak)) = 0;
extern void *__eh_frame_start;

/*
 * These two functions could have been overriden by libgcc's Dwarf2 unwinder code.
 * (used for example for C++ exception handling).
 * We do this trick in order to avoid linking in the whole unwinder just by referencing
 * these functions. This helps to keep C executables smaller.
 * Note that we call them when libraries are already open because __register_frame()
 * internally uses malloc(). See unwind-dw2-fde.c in gcc source code.
 *
 * TODO: Calling these functions from here means that only plain executables register their
 * exception frames. This means that:
 * a) C++ exceptions won't work inside shared libraries.
 * This can be easily fixed in libraries startup code. But libraries are separate executables, and
 * this would mean that they register their own frames in their own copy of libgcc runtime. As a
 * consequence:
 * b) C++ exceptions can't be thrown outside of shared libraries using this approach.
 * (b) is more difficult to fix. When some program opens the library, it should be able to register
 * its frames and actually process exceptions using program's runtime. This can be implemented
 * in a number of ways, every of them involving development of some ABI standard.
 */
void __attribute__((weak)) __register_frame(void *begin) {}
void __attribute__((weak)) __deregister_frame(void *begin) {}

THIS_PROGRAM_HANDLES_SYMBOLSET(LIBS)
THIS_PROGRAM_HANDLES_SYMBOLSET(CTORS)
THIS_PROGRAM_HANDLES_SYMBOLSET(DTORS)
THIS_PROGRAM_HANDLES_SYMBOLSET(INIT)
THIS_PROGRAM_HANDLES_SYMBOLSET(EXIT)
DEFINESET(CTORS);
DEFINESET(DTORS);
DEFINESET(INIT);
DEFINESET(EXIT);

static void __startup_initexit(void)
{
    D(bug("Entering __startup_initexit\n"));

    if (set_open_libraries())
    {
        __register_frame(&__eh_frame_start);

        if (set_call_funcs(SETNAME(INIT), 1, 1))
	{
            /* ctors/dtors get called in inverse order than init funcs */
            set_call_funcs(SETNAME(CTORS), -1, 0);

            __startup_entries_next();

            set_call_funcs(SETNAME(DTORS), 1, 0);
        }
        set_call_funcs(SETNAME(EXIT), -1, 0);

	__deregister_frame(&__eh_frame_start);
    }
    set_close_libraries();
    
    D(bug("Leaving __startup_initexit\n"));
}

ADD2SET(__startup_initexit, PROGRAM_ENTRIES, -20);
