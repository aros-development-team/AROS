/*
 * Copyright (C) 2012, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#ifndef DUMMYBASE_H
#define DUMMYBASE_H

#include <exec/libraries.h>

struct DummyBase {
    struct Library db_Lib;
    int lastval;
};

#endif /* DUMMYBASE_H */
