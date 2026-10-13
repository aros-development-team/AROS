#ifndef LIBRARIES_BONAMIBASE_H
#define LIBRARIES_BONAMIBASE_H
/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 *
 * bonamibase.h - bonami.library's base and the handles it gives out
 */

#ifndef EXEC_LIBRARIES_H
#include <exec/libraries.h>
#endif

#define BONAMINAME "bonami.library"

struct BonamiBase
{
    struct Library LibNode;
};

typedef APTR MDNSRegisterRef;

#endif /* LIBRARIES_BONAMIBASE_H */
