/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Architecture independent placeholder for the C runtime floating point
    environment start-up.

    It only provides the __stdc_mathinit marker symbol that the (architecture
    independent) stdc start-up code references, so that this module is linked
    into every executable that uses stdc. Architectures whose FPU has to be
    configured explicitly to provide the C standard default floating point
    environment (e.g. the round-to-nearest rounding direction) override this
    file and additionally install an ADD2INIT() handler that performs the
    set-up.
*/

#include <exec/types.h>

ULONG __stdc_mathinit = 0;
