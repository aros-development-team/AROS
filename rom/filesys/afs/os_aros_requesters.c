/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include "os_aros_support.h"

#if !AFS_INLINE_REQUESTERS
#define AFS_REQUESTER_LINKAGE
#include "os_aros_requesters.h"
#undef AFS_REQUESTER_LINKAGE
#endif
