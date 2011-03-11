#include <aros/symbolsets.h>
#include <proto/dos.h>

#include "autoinit_intern.h"

char *_ProgramName = NULL;

int __initprogramname(void)
{
    char *cmd = __get_command_name();

    _ProgramName = FilePart(cmd);

    return 1;
}

ADD2INIT(__initprogramname, 0);
