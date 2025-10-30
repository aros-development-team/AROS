/*
    Copyright © 2021-2025, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <aros/libcall.h>
#include <libraries/debug.h>

#include <udis86.h>

#include <proto/debug.h>

struct DisData
{
    struct Library *DebugBase;
    const char *target;
};

static const char *resolve(struct ud *u, uint64_t addr, int64_t *offset)
{
    struct DisData *disData = (struct DisData *)ud_get_user_opaque_data(u);
    struct Library *DebugBase = disData->DebugBase;
    const char *retval = NULL, *symname;
    APTR loc;
	void *symaddr;

    D(bug("[debug] %s(%p)\n", __func__, (APTR)addr);)

#if __WORDSIZE==32
    loc = (APTR)((IPTR)addr & 0xFFFFFFFF);
#else
    loc = (APTR)addr;
#endif

    struct TagItem locTags[] =
    {
        { DL_SymbolName,    (IPTR)&symname  },
        { DL_SymbolStart,   (IPTR)&symaddr  },
        { TAG_DONE,         0               }
    };

	if (DecodeLocationA(loc, locTags)) {
        if (symname)
            retval = symname;
        *offset = addr - (IPTR)symaddr;
	}
    else
    {
        *offset = addr - (__WORDSIZE/2);
    }

    if (!retval)
    {
        if (disData->target)
            retval = disData->target;
        else
            retval = "target";
    }
	return retval;
}

AROS_LH3(APTR, InitDisassembleCtx,
        AROS_LHA(APTR, start, A0),
        AROS_LHA(APTR, end, A1),
        AROS_LHA(APTR, pc, A2),
        struct Library *, DebugBase, 9, Debug)
{
    AROS_LIBFUNC_INIT

    ud_t * ud_obj = AllocVec(sizeof(ud_t), MEMF_CLEAR);
    if (ud_obj)
    {
        struct DisData *disData = AllocVec(sizeof(struct DisData), MEMF_ANY);
        if (disData) {
            const char *symname;
            struct TagItem locTags[] =
            {
                { DL_SymbolName,    (IPTR)&symname  },
                { TAG_DONE,         0               }
            };
            DecodeLocationA(start, locTags);
            disData->DebugBase = DebugBase;
            if (symname)
                disData->target = symname;
            ud_init(ud_obj);
            ud_set_mode(ud_obj, __WORDSIZE);
            ud_set_input_buffer(ud_obj, start, (IPTR)end - (IPTR)start);
            ud_set_pc(ud_obj, (IPTR)start - (IPTR)pc);
    #if (0)
            ud_set_syntax(ud_obj, UD_SYN_INTEL); // NASM-like
    #else
            ud_set_syntax(ud_obj, UD_SYN_ATT); // GAS-like
    #endif
            ud_set_vendor(ud_obj, UD_VENDOR_ANY);
            ud_set_sym_resolver(ud_obj, &resolve);
            ud_set_user_opaque_data(ud_obj, disData);
        }
        else
        {
            FreeVec(ud_obj);
            ud_obj = NULL;
        }
    }
	return ud_obj;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(IPTR, DisassembleCtx,
        AROS_LHA(APTR, ctx, A0),
        struct Library *, DebugBase, 10, Debug)
{
    AROS_LIBFUNC_INIT

	return ud_disassemble(ctx);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(IPTR, GetCtxInstructionA,
        AROS_LHA(APTR, ctx, A0),
        AROS_LHA(struct TagItem *, tags, A1),
        struct Library *, DebugBase, 11, Debug)
{
    AROS_LIBFUNC_INIT

    struct TagItem *tag, *tstate = tags;
    IPTR retval = 0;

    while ((tag = LibNextTagItem(&tstate)))
    {
        switch (tag->ti_Tag)
        {
        case DCIT_Instruction_Offset:
            tag->ti_Data = (IPTR)ud_insn_off(ctx);
            retval++;
            break;

        case DCIT_Instruction_HexStr:
            tag->ti_Data = (IPTR)ud_insn_hex(ctx);
            retval++;
            break;

        case DCIT_Instruction_Asm:
            tag->ti_Data = (IPTR)ud_insn_asm(ctx);
            retval++;
            break;
        }
    }
	return retval;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, FreeDisassembleCtx,
        AROS_LHA(APTR, ctx, A0),
        struct Library *, DebugBase, 12, Debug)
{
    AROS_LIBFUNC_INIT

    struct DisData *disData = (struct DisData *)ud_get_user_opaque_data(ctx);
    FreeVec(disData);
    FreeVec(ctx);

    AROS_LIBFUNC_EXIT
}
