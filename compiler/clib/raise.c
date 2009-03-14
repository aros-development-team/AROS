#include <unistd.h>
#include <signal.h>

/*****************************************************************************

    NAME */

	int raise(

/*  SYNOPSIS */
	int signal)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    kill(getpid(), signal);
    return 0; // disable compiler warning
}
