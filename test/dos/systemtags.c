/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/dos.h>
#include <proto/alib.h>

#include "systemtags.h"

#define DEBUG 0
#include <aros/debug.h>

int main(void)
{
    LONG result;

    /* Execute and wait */
    result = SystemTags("Echo Synchronous Test", NULL);
    Printf("Execution results %ld == 0 ?\n", result);

    /* Start asynchronous */
    result = SystemTags("Echo Asynchronous Test",
        SYS_Asynch, TRUE, SYS_Input, SYS_DupStream, SYS_Output, SYS_DupStream,
        TAG_DONE
    );
    Printf("Execution results %ld == 0 ?\n", result);

    /* Start non-existing command asynchronous */
    result = SystemTags("DoesntExist",
        SYS_Asynch, TRUE, SYS_Input, SYS_DupStream, SYS_Output, SYS_DupStream,
        TAG_DONE
    );
    Printf("Execution results %ld == 0 ?\n", result);

    /* Test with duplicated Input/Output stream */
    result = SystemTags("systemtags_slave dont print",
        SYS_Input, SYS_DupStream, SYS_Output, SYS_DupStream,
        TAG_DONE
    );
    Printf("Execution results %ld == 0 ?\n", result);

    /* Test passing of Input/Output streams */
    struct STData data;
    data.input = Input();
    data.output = Output();
    D(bug("[systemtags]Input: %p, Output: %p\n", data.input, data.output));
    UBYTE buf[30];
    __sprintf(buf, "systemtags_slave %p", &data);
    result = SystemTags(buf, SYS_Input, BNULL, SYS_Output, BNULL,
                        TAG_DONE
    );
    Printf("Execution results %ld == 0 ?\n", result);

    return 0;
}
