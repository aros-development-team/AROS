/*
    Copyright (C) 2010, The AROS Development Team. All rights reserved.

    Desc: BCPL support
*/
#define DEBUG 0
#include <aros/debug.h>
#include <aros/asmcall.h>

#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/rdargs.h>
#include <dos/dosextens.h>
#include <proto/dos.h>

#include <string.h>

/*****************************************************************************

    NAME */

        AROS_UFH4(ULONG, BCPL_ReadArgs,
/*  SYNOPSIS */
        AROS_UFHA(BSTR, btemplate, D1),
        AROS_UFHA(BPTR, bvec, D2),
        AROS_UFHA(ULONG, upb, D3),
        AROS_UFHA(struct DosLibrary *, DOSBase, A6))

/*  LOCATION
        BCPL Vector offset 0x138

    FUNCTION
        Parse command-line arguments according to a BCPL template and
        return their values in a caller-provided BCPL vector. String
        results are copied into the unused portion of that vector and
        represented by BCPL pointers.

    INPUTS
        btemplate - BCPL string containing the ReadArgs() option template.
        bvec      - BCPL pointer to writable vector storage. The vector is
                    cleared before parsing and also holds converted strings.
        upb       - Number of IPTR-sized units available in the vector.
        DOSBase   - DOS library base supplied by the BCPL calling convention.

    RESULT
        Number of vector units occupied by the parsed arguments and their
        converted strings, or zero if parsing, allocation or string storage
        fails. On success IoErr() is cleared.

    NOTES
        The function uses ReadArgs() for template parsing. BCPL string
        results occupy additional vector storage, so the caller must
        provide room beyond the argument slots.

    SEE ALSO
        dos.library/ReadArgs(), dos.library/FreeArgs()

*****************************************************************************/
{
    AROS_USERFUNC_INIT

    STRPTR template;
    IPTR *vec = BADDR(bvec);
    struct RDArgs *rd;
    BSTR bstr;
    CONST_STRPTR cp;
    int arg, args, seen_key, seen_enough, svec;
#define RA_FLAG(x)      ((x)&0x1f)

    template = AllocVec(AROS_BSTR_strlen(btemplate)+1,MEMF_ANY);
    if (template == NULL) {
        SetIoErr(ERROR_NO_FREE_STORE);
        return 0;
    }

    memcpy(template, AROS_BSTR_ADDR(btemplate), AROS_BSTR_strlen(btemplate));
    template[AROS_BSTR_strlen(btemplate)]=0;

    D(bug("-- Template \"%s\"\n", template));

    /* Count args in the template */
    for (args = 1, cp = template; *cp; cp++) {
        if (*cp == ',')
            args++;
    }

    /* BCPL readargs appers to want the vector to be zeroed */
    memset(vec, 0, upb * sizeof(IPTR));

    rd = ReadArgs(template, vec, NULL);
    if (rd == NULL) {
        FreeVec(template);
        return 0;
    }

    svec = args;
    seen_enough = 0;
    for (arg = 0, seen_key = 0, cp = template; ; cp++) {
        int len, left;

        if (*cp == ',' || (cp - template) >= AROS_BSTR_strlen(btemplate)) {
            if (!seen_enough && vec[arg] != 0) {
                /* Ok, it's probably a string. Convert it to BCPL */
                len = strlen((STRPTR)vec[arg]);
                left = (upb - svec)*sizeof(IPTR);
                if (AROS_BSTR_MEMSIZE4LEN(len) > left) {
                    SetIoErr(ERROR_NO_FREE_STORE);
                    FreeArgs(rd);
                    FreeVec(template);
                    return 0;
                }

                bstr = MKBADDR(&vec[svec]);
                D(bug("-- Convert arg %d (%p) \"%s\" to BCPL 0x%x at %p\n", arg, (APTR)vec[arg], (APTR)vec[arg], bstr, &vec[svec]));
                memcpy(AROS_BSTR_ADDR(bstr), (APTR)vec[arg], len);
                AROS_BSTR_setstrlen(bstr, len);
                vec[arg] = bstr;
                svec += AROS_ALIGN(AROS_BSTR_MEMSIZE4LEN(len))/sizeof(IPTR);
            }
            if (*cp != ',')
                break;
            arg++;
            seen_key = 0;
            continue;
        }

        if (!seen_key && *cp == '/') {
            seen_key = 1;
            seen_enough = 0;
            continue;
        }

        if (!seen_key)
            continue;

        if (seen_enough)
            continue;

        if (RA_FLAG(*cp) == RA_FLAG('N')) {
            seen_enough=1;
            continue;
        }
        if (RA_FLAG(*cp) == RA_FLAG('S')) {
            seen_enough=1;
            continue;
        }
        if (RA_FLAG(*cp) == RA_FLAG('T')) {
            seen_enough=1;
            continue;
        }
    }

    FreeVec(template);
    FreeArgs(rd);

    SetIoErr(0);
    return svec;

    AROS_USERFUNC_EXIT
}

