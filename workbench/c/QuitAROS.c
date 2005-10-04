#include <proto/exec.h>

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
