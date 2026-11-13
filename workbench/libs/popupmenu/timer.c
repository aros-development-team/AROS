#include <exec/types.h>
#include <devices/timer.h>
#include <proto/exec.h>

void EZDeleteTimer(TimeRequest)
struct timerequest *TimeRequest;
{
	struct MsgPort *TimePort;

	if(TimeRequest)
	{
		if(TimeRequest->tr_node.io_Device)
			CloseDevice(TimeRequest);

		if((TimePort=TimeRequest->tr_node.io_Message.mn_ReplyPort))
			DeletePort(TimePort);

		DeleteExtIO(TimeRequest);
	}
}

struct timerequest *EZCreateTimer(LONG Unit)
{
	struct MsgPort		*TimePort;
	struct timerequest	*TimeRequest;

	if(!(TimePort = (struct MsgPort *)CreatePort(NULL,0)))
		return(NULL);

	if(!(TimeRequest = (struct timerequest *)CreateExtIO(TimePort,sizeof(struct timerequest))))
	{
		DeletePort(TimePort);

		return(NULL);
	}

	if(OpenDevice(TIMERNAME,Unit,TimeRequest,0))
	{
		DeleteExtIO(TimeRequest);
		DeletePort(TimePort);

		return(NULL);
	}

	return(TimeRequest);
}
