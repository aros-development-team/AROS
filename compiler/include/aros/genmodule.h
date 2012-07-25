/*
 * Copyright (C) 2012, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#ifndef AROS_GENMODULE_H
#define AROS_GENMODULE_H

/*
    Iinclude the sub-include file for a particular CPU.
*/
#if defined __i386__
#   include <aros/i386/genmodule.h>
#elif defined __x86_64__
#   include <aros/x86_64/genmodule.h>
#elif defined __mc68000__
#   include <aros/m68k/genmodule.h>
#elif defined __MORPHOS__
#   include <aros/morphos/genmodule.h>
#elif defined __powerpc__
#   include <aros/ppc/genmodule.h>
#elif defined __arm__
#	include <aros/arm/genmodule.h>
#else
#   error unsupported CPU type
#endif

#endif /* AROS_GENMODULE_H */
