/*
 * Copyright (C) 2011, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 *
 * This file contains the global variables that are referenced
 * by most implementations of kernel.resource
 *
 * Of note, it is overriden by the m68k architecture, which does
 * not use .bss nor .data, for performance reasons related to
 * slow MEMF_CHIP RAM.
 */
#include <aros/symbolsets.h>

/*
 * Some globals we can't live without.
 * IMPORTANT: BootMsg should survive warm restarts, this is why it's placed in .data.
 */
struct KernelBase *KernelBase;
__attribute__((section(".data"))) struct TagItem *BootMsg;
