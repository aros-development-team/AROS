/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Alert context parsing routines
    Lang: english
*/

#include <aros/kernel.h>
#include <exec/rawfmt.h>
#include <proto/kernel.h>

#include "exec_intern.h"
#include "exec_util.h"
#include "etask.h"

#define TRACE_DEPTH 6

static const char *modstring  = "\n0x%P %s Segment %lu %s + 0x%P";
static const char *funstring  = "\n0x%P %s Function %s + 0x%P";
static const char *unknownstr = "\n0x%P Address not found";
static const char *invalidstr = "\n0x%P Invalid stack frame address";

/*
 * Make a readable text out of task's alert context
 * The supplied buffer pointer points to the end of already existing
 * text, so we start every our line with '\n'.
 */
void FormatAlertExtra(char *buffer, struct Task *task, struct ExecBase *SysBase)
{
    struct IntETask *iet = GetIntETask(task);
    char *buf = buffer;
    
    switch (iet->iet_AlertType)
    {
    case AT_CPU:
	buf = Alert_AddString(buf, "\nCPU context:\n");
	buf = FormatCPUContext(buf, &iet->iet_AlertData.u.acpu, SysBase);

	break;

    /* TODO: add more types (memory manager is the first candidate) */

    }

    /* If we have AlertStack, compose a backtrace */
    if (iet->iet_AlertStack)
    {
	APTR fp = iet->iet_AlertStack;
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

#ifdef KrnDecodeLocation
		if (KrnDecodeLocation(caller,
				      KDL_ModuleName , &modname, KDL_SegmentNumber, &segnum ,
				      KDL_SegmentName, &segname, KDL_SegmentStart , &segaddr,
				      KDL_SymbolName , &symname, KDL_SymbolStart  , &symaddr,
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
#endif
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
