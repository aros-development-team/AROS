#include <proto/exec.h>

/**************************************************************************

    NAME
	QuitAROS

    SYNOPSIS
	(N/A)

    LOCATION
	Sys:c

    FUNCTION
           This command is only of interest if you're using a full-screen 
            host system - for ease of use. 
	It quits the Host client...

**************************************************************************/


int main(void)
{
    struct MsgPort *port;
    
    Forbid();
    if ((port = FindPort("AROS Hosted Power Switch")))
    {
    	Signal((struct Task *)port->mp_SigTask, 1L << port->mp_SigBit);
    }
    Permit();
    
    return 0;
}
