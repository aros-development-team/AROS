/*
    Copyright (C) 2010-2021, The AROS Development Team. All rights reserved.

    Desc: Alert context parsing routines
*/

#include <exec/rawfmt.h>
#include <libraries/debug.h>
#include <proto/debug.h>

#define DEBUG_NOPRIVATEINLINE
#include "debug_intern.h"
#include "exec_intern.h"
#include "exec_util.h"
#include "etask.h"
#include "memory.h"

#define TRACE_DEPTH 10
#define DISS_DEPTH 5

static const char modstring[]  = "\n0x%P %s Segment %lu %s + 0x%P";
static const char funstring[]  = "\n0x%P %s Function %s + 0x%P";
static const char unknownstr[] = "\n0x%P Address not found";
static const char invalidstr[] = "\n0x%P Invalid stack frame address";
static const char illegalstr[] = "\n0x%P ******************************** <illegal address>";

/*
 * Make a readable text out of task's alert context
 * The supplied buffer pointer points to the end of already existing
 * text, so we start every our line with '\n'.
 */
void FormatAlertExtra(char *buffer, APTR stack, UBYTE type, APTR data, struct ExecBase *SysBase)
{
        struct Library *DebugBase = PrivExecBase(SysBase)->DebugBase;
    char *buf = buffer;
    APTR dissCtx;

    switch (type)
    {
    case AT_CPU:
                buf = Alert_AddString(buf, "\nCPU context:\n");
                buf = FormatCPUContext(buf, data, SysBase);
                break;

    case AT_MUNGWALL:
        buf = Alert_AddString(buf, "\nMungwall data:\n");
        buf = FormatMWContext(buf, data, SysBase);
        break;

    case AT_MEMORY:
        buf = Alert_AddString(buf, "\nMemory manager data:\n");
        buf = FormatMMContext(buf, data, SysBase);
        break;

    }

        if (DebugBase && (((struct DebugBase *)DebugBase)->db_Flags & DBFF_DISASSEMBLE))
        {
                struct ExceptionContext *ctx = (struct ExceptionContext *)data;

                buf = Alert_AddString(buf, "\n\nDisassembly:\n");

                if (TypeOfMem((APTR)ctx->PC))
                {
                        struct TagItem instrTags[] =
                        {
                                { DCIT_Instruction_Offset,      0 },
                                { DCIT_Instruction_HexStr,      0 },
                                { DCIT_Instruction_Asm,         0 },
                                { TAG_DONE,                             0 }
                        };
                        char insPad[(__WORDSIZE/2) + 1];
                        APTR tmpbuf, symaddr, symend = NULL;
                        char *symname = NULL;
                        ULONG x,inslen;

                        if (DecodeLocation((APTR)ctx->PC,
                                                        DL_SymbolName, &symname,
                                                        DL_SymbolStart, &symaddr,
                                                        DL_SymbolEnd, &symend,
                                                        TAG_DONE))
                        {
                                if (symname)
                                        buf = NewRawDoFmt("\n%p <%s>:\n", RAWFMTFUNC_STRING, buf, symaddr, symname) - 1;
                                if (symaddr < (APTR)(ctx->PC - ((DISS_DEPTH / 2) * sizeof(IPTR))))
                                {
                                        buf = NewRawDoFmt("-skipping %p -> %p:\n", RAWFMTFUNC_STRING, buf, symaddr, (APTR)(ctx->PC - ((DISS_DEPTH / 2) * sizeof(IPTR)))) - 1;
                                        symaddr = (APTR)(ctx->PC - ((DISS_DEPTH / 2) * sizeof(IPTR)));
                                }
                                if (!symend || (symend > (APTR)(ctx->PC + ((DISS_DEPTH / 2) * sizeof(IPTR)))))
                                        symend = (APTR)(ctx->PC + ((DISS_DEPTH / 2) *sizeof(IPTR)));
                        }
                        else
                        {
                                symaddr = (APTR)ctx->PC;
                                symend = (APTR)(ctx->PC + (DISS_DEPTH *sizeof(IPTR)));
                        }
                        if ((dissCtx = InitDisassembleCtx(symaddr, symend, (APTR)ctx->PC)) != NULL)
                        {
                                if (!symname)
                                        buf = NewRawDoFmt("\n%p:\n", RAWFMTFUNC_STRING, buf, symaddr) - 1;

                                while (DisassembleCtx(dissCtx) != 0) {
                                        if (GetCtxInstructionA(dissCtx, instrTags))
                                        {
                                                /* output the offset */
                                                buf = NewRawDoFmt("%4d%s: ", RAWFMTFUNC_STRING, buf,
                                                                                                         instrTags[0].ti_Data, (instrTags[0].ti_Data == 0) ? "*" : " ") - 1;

                                                /* output the raw instruction hexcode */
                                                tmpbuf = NewRawDoFmt("%s", RAWFMTFUNC_STRING, buf, instrTags[1].ti_Data);
                                                inslen = (ULONG)((IPTR)tmpbuf - (IPTR)buf);
                                                buf = tmpbuf - 1;
                                                for (x = 0; x < ((__WORDSIZE/2) - inslen); x++)
                                                {
                                                        insPad[x] = ' ';
                                                }
                                                insPad[x] = '\0';

                                                /* output the padding, and disassembled code */
                                                buf = NewRawDoFmt("%s - %s\n", RAWFMTFUNC_STRING, buf,
                                                                                                         insPad,
                                                                                                         instrTags[2].ti_Data) - 1;
                                        }
                                }
                                FreeDisassembleCtx(dissCtx);
                        }
                }
                else
                {
                        buf = NewRawDoFmt(illegalstr, RAWFMTFUNC_STRING, buf, (APTR)ctx->PC);
                }
        }

    /* If we have AlertStack, compose a backtrace */
    if (stack)
    {
        APTR fp = stack;
        ULONG i;

        buf = Alert_AddString(buf, "\nStack trace:");

        for (i = 0; i < TRACE_DEPTH; i++)
        {
            /* Safety check: ensure that frame pointer address is valid */
            if (TypeOfMem(fp))
            {
                APTR caller = NULL;
                char *modname, *segname, *symname;
                void *segaddr, *symaddr;
                unsigned int segnum;

                fp = UnwindFrame(fp, &caller);

                if (DebugBase && DecodeLocation(caller,
                                                DL_ModuleName , &modname, DL_SegmentNumber, &segnum ,
                                                DL_SegmentName, &segname, DL_SegmentStart , &segaddr,
                                                DL_SymbolName , &symname, DL_SymbolStart  , &symaddr,
                                                TAG_DONE))
                {
                    if (symaddr)
                    {
                        if (!symname)
                            symname = "- unknown -";

                        buf = NewRawDoFmt(funstring, RAWFMTFUNC_STRING, buf, caller, modname, symname, caller - symaddr);
                    }
                    else
                    {
                        if (!segname)
                            segname = "- unknown -";

                        buf = NewRawDoFmt(modstring, RAWFMTFUNC_STRING, buf, caller, modname, segnum, segname, caller - segaddr);
                    }
                }
                else
                    buf = NewRawDoFmt(unknownstr, RAWFMTFUNC_STRING, buf, caller);
            }
            else
            {
                /* Invalid address stops unwinding */
                buf = NewRawDoFmt(invalidstr, RAWFMTFUNC_STRING, buf, fp);
                break;
            }

            /* Stop if we have no more frames */
            if (!fp)
                break;

            /*
             * After NewRawDoFmt() returned pointer points to the location AFTER
             * NULL terminator, so we need to step back in order to append one
             * more line
             */
            buf--;
        }
    }
}
