/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

/* There are some functions that are never used, 
   but are required for proper linking against libjpeg.  
   
   I could implement dummy stub functions for them, but that
   would occupy space, I'm therefore simply going to define
   them as absolute symbols, which will not occupy other
   space than the one they require in the symbol table.  */

#include <aros/system.h>

/* This function type has these parameters so that gcc doesn't complain
   about fprintf being declared with a signature incompatible with the 
   one of the builtin fprinf function.  */
typedef int intfunc(void *, const char *, ...);

#define MAKE_FAKE_FUNC(sym)                         \
    AROS_MAKE_ASM_SYM(intfunc, sym, sym, 0xBADBAD); \
    AROS_EXPORT_ASM_SYM(sym);
    
MAKE_FAKE_FUNC(ferror);
MAKE_FAKE_FUNC(fflush);
MAKE_FAKE_FUNC(fprintf);
MAKE_FAKE_FUNC(fread);
MAKE_FAKE_FUNC(fwrite);
