/*
 * Copyright (C) 2012-2023, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#ifndef AROS_GENMODULE_H
#define AROS_GENMODULE_H

/*
    Include the sub-include file for a particular CPU.
*/
#if defined __aarch64__ 
#   include <aros/aarch64/genmodule.h>
#elif defined __arm__
#   if defined __thumb2__
#	    include <aros/arm/genmodule-thumb2.h>
#   else
#	    include <aros/arm/genmodule.h>
#   endif
#elif defined __i386__
#   include <aros/i386/genmodule.h>
#elif defined __mc68000__
#   include <aros/m68k/genmodule.h>
#elif defined __MORPHOS__
#   include <aros/morphos/genmodule.h>
#elif defined __powerpc__
#   include <aros/ppc/genmodule.h>
#elif defined __riscv64
#   include <aros/riscv64/genmodule.h>
#elif defined __riscv
#   include <aros/riscv/genmodule.h>
#elif defined __x86_64__
#   include <aros/x86_64/genmodule.h>
#else
#   error unsupported CPU type
#endif

#endif /* AROS_GENMODULE_H */
