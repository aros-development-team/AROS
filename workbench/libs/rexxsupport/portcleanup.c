#include <proto/exec.h>
#include <proto/rexxsyslib.h>
#include <rexx/storage.h>
#include <rexx/errors.h>

#include <string.h>

#include "portnode.h"

#include "rexxsupport_intern.h"

void portcleanup(struct Library *RexxSupportBase, struct PortNode *node)
{
    struct RexxMsg *msg; 
    struct PortNodeData *data = &node->data;
  
    Remove(&data->node);
    if (IsListEmpty(&RSBI(RexxSupportBase)->openports))
        RexxSupportBase->lib_OpenCnt--;
  
    RemPort(data->port);

    ForeachNode(&data->msgs, msg)
    {
        if (IsRexxMsg(msg))
        {
	    msg->rm_Result1 = RC_ERROR;
	    msg->rm_Result2 = (IPTR)ERR10_013;
	}
        ReplyMsg((struct Message *)msg);
    }
  
    while ((msg = (struct RexxMsg *)GetMsg(node->data.port)) != NULL)
    {
        if (IsRexxMsg(msg))
        {
	    msg->rm_Result1 = RC_ERROR;
	    msg->rm_Result2 = (IPTR)ERR10_013;
	}
        ReplyMsg((struct Message *)msg);
    }

    DeleteMsgPort(data->port);

    FreeMem(data->node.ln_Name, strlen(data->node.ln_Name)+1);
    FreeMem(node, sizeof(struct PortNode));
}
