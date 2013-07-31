#include <proto/dos.h>
#include <proto/alib.h>

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

    return 0;
}
