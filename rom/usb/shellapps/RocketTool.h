#ifndef ROCKETTOOL_H
#define ROCKETTOOL_H

/* dradio vendor specific requests */

struct NepClassRocket
{
    struct Node         nch_Node;         /* Node linkage */
    struct PsdDevice   *nch_Device;       /* Up linkage */
    struct PsdConfig   *nch_Config;       /* Up linkage */
    struct PsdInterface *nch_IF0;         /* Up linkage */
    struct PsdInterface *nch_IF1;         /* Up linkage */
    struct PsdPipe     *nch_EP0Pipe;      /* Endpoint 0 pipe */
    struct Hook         nch_ReleaseHook;  /* Hook for release function */
    struct Task        *nch_Task;         /* this task */
    struct MsgPort     *nch_TaskMsgPort;  /* io message port */
    UBYTE               nch_Buf[64];      /* sending buffer */
};

#endif /* ROCKETTOOL_H */
