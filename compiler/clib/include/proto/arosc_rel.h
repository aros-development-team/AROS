/*
 * Copyright (C) 2012, The AROS Development Team
 * All right reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#ifndef AROSC_REL_H
#define AROSC_REL_H

#include <exec/types.h>

extern const IPTR __aros_rellib_offset_aroscbase;

#define AROS_RELLIB_OFFSET_AROSC __aros_rellib_offset_aroscbase
#define AROS_RELLIB_BASE_AROSC   __aros_rellib_base_aroscbase

#endif /* AROSC_REL_H */
