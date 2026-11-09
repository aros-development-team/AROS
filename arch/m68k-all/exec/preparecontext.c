/* Prepare the initial task frame in the format used by Switch/Dispatch. */

#include <exec/execbase.h>
#include <exec/memory.h>
#include <utility/tagitem.h>
#include <aros/m68k/cpucontext.h>
#include <proto/kernel.h>
#include <string.h>

#include "exec_intern.h"
#include "exec_util.h"

#define _PUSH(sp, val) *--sp = (IPTR)(val)
#define M68K_TASK_FRAME_SIZE 66

/* The argument tags are handled as a range below */
_Static_assert(TASKTAG_ARG2 == TASKTAG_ARG1 + 1 && TASKTAG_ARG3 == TASKTAG_ARG1 + 2 &&
               TASKTAG_ARG4 == TASKTAG_ARG1 + 3 && TASKTAG_ARG5 == TASKTAG_ARG1 + 4 &&
               TASKTAG_ARG6 == TASKTAG_ARG1 + 5 && TASKTAG_ARG7 == TASKTAG_ARG1 + 6 &&
               TASKTAG_ARG8 == TASKTAG_ARG1 + 7,
               "TASKTAG_ARG1 to TASKTAG_ARG8 must be consecutive");

BOOL PrepareContext(struct Task *task, APTR entryPoint, APTR fallBack,
                    const struct TagItem *tagList, struct ExecBase *SysBase)
{
    IPTR args[8] = {0};
    WORD numargs = 0;
    IPTR *sp = task->tc_SPReg;
    struct ExceptionContext *ctx;
    UBYTE *frame;

    if (!(task->tc_Flags & TF_ETASK))
        return FALSE;

    ctx = KrnCreateContext();
    task->tc_UnionETask.tc_ETask->et_RegFrame = ctx;
    if (!ctx)
        return FALSE;

    while (tagList) {
        switch (tagList->ti_Tag) {
        case TAG_MORE:
            tagList = (const struct TagItem *)tagList->ti_Data;
            continue;
        case TAG_SKIP:
            tagList += tagList->ti_Data;
            break;
        case TAG_DONE:
            tagList = NULL;
            break;
        default:
            /* TASKTAG_ARG1 to TASKTAG_ARG8 are consecutive tags */
            if (tagList->ti_Tag >= TASKTAG_ARG1 && tagList->ti_Tag <= TASKTAG_ARG8) {
                WORD arg = tagList->ti_Tag - TASKTAG_ARG1;

                args[arg] = (IPTR)tagList->ti_Data;
                if (arg >= numargs)
                    numargs = arg + 1;
            }
            break;
        }
        if (tagList)
            tagList++;
    }

    while (numargs--)
        _PUSH(sp, args[numargs]);
    _PUSH(sp, fallBack);

    frame = (UBYTE *)sp - M68K_TASK_FRAME_SIZE;
    memset(frame, 0, M68K_TASK_FRAME_SIZE);
    *(IPTR *)(frame + 0) = (IPTR)entryPoint;
    *(UWORD *)(frame + 4) = 0;

    /* Retain a conventional context for FPU/AMMX storage and diagnostics. */
    ctx->pc = (IPTR)entryPoint;
    ctx->a[7] = (IPTR)sp;
    ctx->sr = 0;

    task->tc_SPReg = frame;
    return TRUE;
}
