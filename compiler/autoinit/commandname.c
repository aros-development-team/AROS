/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/tasks.h>
#include <proto/exec.h>

#include "autoinit_intern.h"

/*
 * We can be called several times.
 * In order to avoid unneeded hassle we cache our result here.
 */
static char *_CommandName = NULL;

char *___get_command_name(struct ExecBase *SysBase)
{
    if (!_CommandName)
    {
    	struct Task *me = FindTask(NULL);

	/*
	 * TODO:
	 * In AROS task's name always corresponds to command name.
	 * However at least on AmigaOS v3 this is not true for CLI
	 * proceses. In this case process name is still 'Shell process',
	 * and command name is placed in cli_CommandName
	 * P.S. I may remember something wrong - Pavel Fedin
	 */
	_CommandName = me->tc_Node.ln_Name;
    }

    return _CommandName;
}
