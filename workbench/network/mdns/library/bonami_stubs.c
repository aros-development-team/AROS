/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 * Copyright 2026 Jaime Dias
 */

/* Reserve upstream Bonami's remaining ABI slots until their engines are
 * implemented. These stubs never deliver hooks or take ownership of data. */
#include <exec/types.h>
#include <utility/hooks.h>
#include <aros/libcall.h>
#include "bonami_intern.h"

/*****************************************************************************

    NAME */
        AROS_LH3(LONG, MakeTXTRecordA,

/*  SYNOPSIS */
        AROS_LHA(struct TagItem *, tags, A0),
        AROS_LHA(APTR *, record, A1),
        AROS_LHA(ULONG *, recordLen, A2),

/*  LOCATION */
        struct BonamiIntern *, BonamiBase, 14, Bonami)

/*  FUNCTION
        Compatibility stub for the upstream Bonami ABI. This registration
        release does not implement this operation. No callback is invoked,
        output buffer modified, or memory/reference freed.

    RESULT
        MDNSERR_NOTREADY.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    (void)BonamiBase;
    (void)tags;
    (void)record;
    (void)recordLen;
    return MDNSERR_NOTREADY;
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(void, FreeTXTRecord,

/*  SYNOPSIS */
        AROS_LHA(APTR, record, A0),

/*  LOCATION */
        struct BonamiIntern *, BonamiBase, 15, Bonami)

/*  FUNCTION
        Compatibility stub for the upstream Bonami ABI. This registration
        release does not implement this operation. No callback is invoked,
        output buffer modified, or memory/reference freed.

    RESULT
        None; this call does nothing.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    (void)BonamiBase;
    (void)record;
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(LONG, GetMDNSStatusA,

/*  SYNOPSIS */
        AROS_LHA(struct TagItem *, tags, A0),

/*  LOCATION */
        struct BonamiIntern *, BonamiBase, 16, Bonami)

/*  FUNCTION
        Compatibility stub for the upstream Bonami ABI. This registration
        release does not implement this operation. No callback is invoked,
        output buffer modified, or memory/reference freed.

    RESULT
        MDNSERR_NOTREADY.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    (void)BonamiBase;
    (void)tags;
    return MDNSERR_NOTREADY;
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH4(LONG, InquiryFromNIPCTagsA,

/*  SYNOPSIS */
        AROS_LHA(struct Hook *, hook, A0),
        AROS_LHA(ULONG, maxTime, D0),
        AROS_LHA(ULONG, maxResponses, D1),
        AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
        struct BonamiIntern *, BonamiBase, 17, Bonami)

/*  FUNCTION
        Compatibility stub for the upstream Bonami ABI. This registration
        release does not implement this operation. No callback is invoked,
        output buffer modified, or memory/reference freed.

    RESULT
        MDNSERR_NOTREADY.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    (void)BonamiBase;
    (void)hook;
    (void)maxTime;
    (void)maxResponses;
    (void)tags;
    return MDNSERR_NOTREADY;
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(LONG, PublishNIPCEntitiesA,

/*  SYNOPSIS */
        AROS_LHA(struct TagItem *, tags, A0),

/*  LOCATION */
        struct BonamiIntern *, BonamiBase, 18, Bonami)

/*  FUNCTION
        Compatibility stub for the upstream Bonami ABI. This registration
        release does not implement this operation. No callback is invoked,
        output buffer modified, or memory/reference freed.

    RESULT
        MDNSERR_NOTREADY.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    (void)BonamiBase;
    (void)tags;
    return MDNSERR_NOTREADY;
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH0(void, UnpublishNIPCEntitiesA,

/*  SYNOPSIS */

/*  LOCATION */
        struct BonamiIntern *, BonamiBase, 19, Bonami)

/*  FUNCTION
        Compatibility stub for the upstream Bonami ABI. This registration
        release does not implement this operation. No callback is invoked,
        output buffer modified, or memory/reference freed.

    RESULT
        None; this call does nothing.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    (void)BonamiBase;
    AROS_LIBFUNC_EXIT
}

