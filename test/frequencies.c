#include <devices/timer.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <proto/timer.h>

#include <stdio.h>

int __nocommandline = 1;

int main(void)
{
    struct MsgPort *mp;
    struct timerequest *req;

    printf("exec.library v%u.%u\n", SysBase->LibNode.lib_Version, SysBase->LibNode.lib_Revision);
    printf("VBlank frequency: %u\n", SysBase->VBlankFrequency);
    printf("PSU    frequency: %u\n", SysBase->PowerSupplyFrequency);
    printf("EClock frequency: %u\n\n", SysBase->ex_EClockFrequency);
    
    mp = CreateMsgPort();
    if (mp) {
	req = (struct timerequest *)CreateIORequest(mp, sizeof(struct timerequest));
	if (req) {
	    if (!OpenDevice("timer.device", UNIT_VBLANK, &req->tr_node, 0))
	    {
		struct Device *TimerBase = req->tr_node.io_Device;
		struct EClockVal clock;
	    
		printf("timer.device v%u.%u\n", TimerBase->dd_Library.lib_Version, TimerBase->dd_Library.lib_Revision);
		printf("EClock frequency reported: %u\n", ReadEClock(&clock));

		CloseDevice(&req->tr_node);
	    }
	    else
		printf("Failed to open timer.device!\n");

		DeleteIORequest(req);
	} else
	    printf("Failed to create IORequest!\n");

	    DeleteMsgPort(mp);
    } else
	printf("Failed to create message port!\n");
    
    return 0;
}
