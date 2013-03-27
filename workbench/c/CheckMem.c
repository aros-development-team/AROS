/******************************************************************************


    NAME

        CheckMem

    SYNOPSIS

        (N/A)

    LOCATION

        C:

    FUNCTION

        Triggers a memory check.

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

#include <exec/memory.h>
#include <proto/exec.h>

int main(void)
{
    AvailMem(MEMF_CLEAR);
    
    return 0;
}
