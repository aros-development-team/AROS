/*
    Copyright © 2021-2025, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

#include <aros/debug.h>
#include <proto/debug.h>
#include <proto/exec.h>
#include <proto/kernel.h>

#include <aros/kernel.h>

#include "debug_intern.h"

static LONG Debug_ResolveOne(APTR _base, APTR addr, struct KrnSymInfo *out)
{
    struct DebugBase *DebugBase;
    if ((DebugBase = (struct DebugBase *)_base) != NULL) {
        const STRPTR mod = NULL, seg = NULL, sym = NULL;
        APTR seg_start = NULL, seg_end = NULL, sym_start = NULL, sym_end = NULL;
        BPTR seg_bptr = 0;
        ULONG seg_num = 0;

        struct TagItem tags[] = {
            { DL_ModuleName,     (IPTR)&mod      },
            { DL_SegmentName,    (IPTR)&seg      },
            { DL_SegmentPointer, (IPTR)&seg_bptr },
            { DL_SegmentNumber,  (IPTR)&seg_num  },
            { DL_SegmentStart,   (IPTR)&seg_start},
            { DL_SegmentEnd,     (IPTR)&seg_end  },
            { DL_SymbolName,     (IPTR)&sym      },
            { DL_SymbolStart,    (IPTR)&sym_start},
            { DL_SymbolEnd,      (IPTR)&sym_end  },
            { TAG_DONE, 0 }
        };

        if (DecodeLocationA(addr, tags)) {
            out->mod_name  = mod;
            out->seg_name  = seg;
            out->sym_name  = sym;
            out->seg_start = seg_start;
            out->seg_end   = seg_end;
            out->sym_start = sym_start;
            out->sym_end   = sym_end;
            out->seg_bptr  = seg_bptr;
            out->seg_num   = seg_num;
            return 1;
        }
    }
    return 0;
}

static int Debugx86_Init(struct DebugBase *DebugBase)
{
    DebugBase->db_Flags |= DBFF_DISASSEMBLE;

    if (KernelBase) {
        KrnRegisterSymResolver(Debug_ResolveOne, (APTR)DebugBase);
    }
    return TRUE;
}

static int Debugx86_Expunge(struct DebugBase *DebugBase)
{
    if (KernelBase) {
        KrnUnregisterSymResolver(Debug_ResolveOne);
    }
    return TRUE;
}

ADD2INITLIB(Debugx86_Init, 10)
ADD2EXPUNGELIB(Debugx86_Expunge, 10)
