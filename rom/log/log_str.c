/*
    Copyright (C) 2023-2025, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/timer.h>

#include <proto/log.h>
#include <proto/alib.h>

#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>

#include <resources/log.h>

#include <string.h>

#include "log_intern.h"

#include LC_LIBDEFS_FILE

#define DOSBase LIBBASE->lrb_DosBase
#define TimerBase LIBBASE->lrb_TimerIOReq.tr_node.io_Device
#define UtilityBase LIBBASE->lrb_UtilityBase

AROS_UFH2(void, GM_UNIQUENAME(PutChar),
                   AROS_UFHA(char, ch, D0),
                   AROS_UFHA(struct logRDF *, rdf, A3))
{
    AROS_USERFUNC_INIT

    if(rdf->rdf_Len) {
        rdf->rdf_Len--;
        *rdf->rdf_Buf++ = ch;
    }

    AROS_USERFUNC_EXIT
}

AROS_UFH2(void, GM_UNIQUENAME(CountChars),
          AROS_UFHA(char, ch, D0),
          AROS_UFHA(ULONG *, count, A3))
{
    AROS_USERFUNC_INIT

    if (ch && (ch != '\n'))
        (*count)++;

    AROS_USERFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH4(void, logRawDoFmtA,

/*  SYNOPSIS */
                 AROS_LHA(STRPTR, buf, A0),
                 AROS_LHA(ULONG, len, D0),
                 AROS_LHA(CONST_STRPTR, fmtstr, A1),
                 AROS_LHA(RAWARG, fmtdata, A2),

/*  LOCATION */
                 LIBBASETYPEPTR, LIBBASE, 5, log)
/*  FUNCTION
        Format RawDoFmt() arguments into a bounded character buffer.

    INPUTS
        buf - Writable buffer of at least len bytes.
        len - Buffer capacity, including the terminator.
        fmtstr - NUL-terminated RawDoFmt() format string.
        fmtdata - RAWARG data matching fmtstr and the target ABI.

    RESULT
        None. With non-NULL buf and fmtstr and len greater than zero,
        output is NUL-terminated and truncated to at most len - 1 bytes.
        Otherwise the buffer is unchanged.

    NOTES
        The formatter still processes the complete format after the buffer
        fills. No required-length or truncation result is returned. The
        buffer is caller-owned; no storage is allocated. Use RawDoFmt()
        format and argument conventions, not a printf() va_list.

    SEE ALSO
        logRawDoFmtCBA(), logCopyStrFmtA(), exec.library/RawDoFmt()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct logRDF rdf;

    if(buf && fmtstr && (len > 0)) {
        struct ExecBase *SysBase = GM_SYSBASE_FIELD(LIBBASE);
        rdf.rdf_Len = len;
        rdf.rdf_Buf = buf;
        RawDoFmt(fmtstr, fmtdata, (VOID_FUNC) GM_UNIQUENAME(PutChar), &rdf);
        buf[len-1] = 0;
    }

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH6(void, logRawDoFmtCBA,

/*  SYNOPSIS */
                 AROS_LHA(STRPTR, buf, A0),
                 AROS_LHA(ULONG, len, D0),
                 AROS_LHA(VOID_FUNC, cbfunc, A1),
                 AROS_LHA(void *, cbdata, A2),
                 AROS_LHA(CONST_STRPTR, fmtstr, A3),
                 AROS_LHA(RAWARG, fmtdata, A4),

/*  LOCATION */
                 LIBBASETYPEPTR, LIBBASE, 6, log)
/*  FUNCTION
        Run RawDoFmt() with a caller-supplied output routine.

    INPUTS
        buf - Writable buffer of at least len bytes.
        len - Capacity of buf; must be greater than zero to invoke output.
        cbfunc - RawDoFmt()-compatible output routine.
        cbdata - Output routine's context, passed unchanged to RawDoFmt().
        fmtstr - NUL-terminated RawDoFmt() format string.
        fmtdata - RAWARG data matching fmtstr and the target ABI.

    RESULT
        None. After formatting, buf[len - 1] is set to NUL. NULL buf,
        NULL fmtstr or zero len suppresses the call and leaves buf unchanged.

    NOTES
        buf and len are not passed to the output routine and do not bound
        its writes. The routine must implement its own capacity checks
        through cbdata. It receives RawDoFmt() output, including the final
        NUL, using the RawDoFmt callback ABI. Execution is synchronous;
        context and output storage must remain valid until return.
        Any allocation, waiting or side effects of the callback are the
        callback's responsibility.

    SEE ALSO
        logRawDoFmtA(), exec.library/RawDoFmt()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    if(buf && fmtstr && (len > 0)) {
        struct ExecBase *SysBase = GM_SYSBASE_FIELD(LIBBASE);
        RawDoFmt(fmtstr, fmtdata, cbfunc, cbdata);
        buf[len-1] = 0;
    }

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(STRPTR, logCopyStr,

/*  SYNOPSIS */
                 AROS_LHA(CONST_STRPTR, name, A0),

/*  LOCATION */
                 LIBBASETYPEPTR, LIBBASE, 4, log)
/*  FUNCTION
        Allocate a copy of a NUL-terminated string.

    INPUTS
        name - Source string, or NULL.

    RESULT
        A caller-owned copy including the terminating NUL, or NULL for a
        NULL source or allocation failure. Release the copy with FreeVec().

    NOTES
        The source is not modified or retained. An empty source produces
        an allocated empty string. Call from task context.

    SEE ALSO
        logCopyStrFmtA(), exec.library/FreeVec()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    STRPTR rs = NULL;

    if (name) {
        struct ExecBase *SysBase = GM_SYSBASE_FIELD(LIBBASE);
        rs = AllocVec((ULONG) strlen(name) + 1, MEMF_ANY);
        if(rs)
            strcpy(rs, name);
    }
    return(rs);

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH2(STRPTR, logCopyStrFmtA,

/*  SYNOPSIS */
                 AROS_LHA(CONST_STRPTR, fmtstr, A0),
                 AROS_LHA(RAWARG, fmtdata, A1),

/*  LOCATION */
                 LIBBASETYPEPTR, LIBBASE, 7, log)
/*  FUNCTION
        Allocate a string formatted with RawDoFmt().

    INPUTS
        fmtstr - NUL-terminated RawDoFmt() format string, or NULL.
        fmtdata - RAWARG data matching fmtstr and the target ABI.

    RESULT
        A caller-owned NUL-terminated string, or NULL for a NULL format
        or allocation failure. Release the string with FreeVec().

    NOTES
        Formatting runs twice: once to calculate capacity and once to
        write the result. Keep the format, argument data and referenced
        strings unchanged across both passes. Call from task context.

    BUGS
        The counting pass excludes newline characters, but the output
        pass writes them. Output containing newlines is therefore truncated;
        newlines are not simply removed. No truncation status is returned.

    SEE ALSO
        logRawDoFmtA(), logCopyStr(), exec.library/FreeVec()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    STRPTR buf = NULL;

    if (fmtstr) {
        struct ExecBase *SysBase = GM_SYSBASE_FIELD(LIBBASE);
        ULONG len = 0;

        RawDoFmt(fmtstr, fmtdata, (VOID_FUNC) GM_UNIQUENAME(CountChars), &len);
        buf = AllocVec(len + 1, MEMF_ANY);
        if(buf) {
            logRawDoFmtA(buf, len+1, fmtstr, fmtdata);
        }
    }

    return(buf);

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH2(int, logCmpStr,

/*  SYNOPSIS */
                 AROS_LHA(CONST_STRPTR, str1, A0),
                 AROS_LHA(CONST_STRPTR, str2, A1),

/*  LOCATION */
                 LIBBASETYPEPTR, LIBBASE, 8, log)
/*  FUNCTION
        Compare two NUL-terminated strings using strcmp().

    INPUTS
        str1 - Non-NULL first string.
        str2 - Non-NULL second string.

    RESULT
        Zero if equal, a negative value if str1 sorts before str2, or a
        positive value if it sorts after str2. Only the sign is specified.

    NOTES
        Comparison is case-sensitive and stops at the first unequal byte
        or terminator. Neither string is modified or retained. NULL is not
        accepted.

    SEE ALSO
        logCopyStr()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return strcmp(str1, str2);

    AROS_LIBFUNC_EXIT
}
