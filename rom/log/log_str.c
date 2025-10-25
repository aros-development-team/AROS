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

AROS_LH4(void, logRawDoFmtA,
         AROS_LHA(STRPTR, buf, A0),
         AROS_LHA(ULONG, len, D0),
         AROS_LHA(CONST_STRPTR, fmtstr, A1),
         AROS_LHA(RAWARG, fmtdata, A2),
         LIBBASETYPEPTR, LIBBASE, 5, log)
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

AROS_LH6(void, logRawDoFmtCBA,
         AROS_LHA(STRPTR, buf, A0),
         AROS_LHA(ULONG, len, D0),
         AROS_LHA(VOID_FUNC, cbfunc, A1),
         AROS_LHA(void *, cbdata, A2),
         AROS_LHA(CONST_STRPTR, fmtstr, A3),
         AROS_LHA(RAWARG, fmtdata, A4),
         LIBBASETYPEPTR, LIBBASE, 6, log)
{
    AROS_LIBFUNC_INIT

    if(buf && fmtstr && (len > 0)) {
        struct ExecBase *SysBase = GM_SYSBASE_FIELD(LIBBASE);
        RawDoFmt(fmtstr, fmtdata, cbfunc, cbdata);
        buf[len-1] = 0;
    }

    AROS_LIBFUNC_EXIT
}

AROS_LH1(STRPTR, logCopyStr,
         AROS_LHA(CONST_STRPTR, name, A0),
         LIBBASETYPEPTR, LIBBASE, 4, log)
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

AROS_LH2(STRPTR, logCopyStrFmtA,
         AROS_LHA(CONST_STRPTR, fmtstr, A0),
         AROS_LHA(RAWARG, fmtdata, A1),
         LIBBASETYPEPTR, LIBBASE, 7, log)
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

AROS_LH2(int, logCmpStr,
         AROS_LHA(CONST_STRPTR, str1, A0),
         AROS_LHA(CONST_STRPTR, str2, A1),
         LIBBASETYPEPTR, LIBBASE, 8, log)
{
    AROS_LIBFUNC_INIT

    return strcmp(str1, str2);

    AROS_LIBFUNC_EXIT
}
