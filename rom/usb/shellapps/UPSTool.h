#ifndef UPSTOOL_H
#define UPSTOOL_H

struct NepClassUPS
{
    struct Node         nch_Node;         /* Node linkage */
    struct PsdDevice   *nch_Device;       /* Up linkage */
    struct PsdConfig   *nch_Config;       /* Up linkage */
    struct PsdInterface *nch_IF0;         /* Up linkage */
    struct PsdEndpoint *nch_IntEP;        /* Int IN Endpoint */
    struct PsdPipe     *nch_EP0Pipe;      /* Endpoint 0 pipe */
    struct PsdPipe     *nch_IntPipe;      /* Int IN pipe */
    struct Hook         nch_ReleaseHook;  /* Hook for release function */
    struct Task        *nch_Task;         /* this task */
    struct MsgPort     *nch_TaskMsgPort;  /* io message port */
    UBYTE               nch_Buf[64];      /* sending buffer */
    UBYTE               nch_Reply[64];    /* receiving buffer */
};

#endif /* UPSTOOL_H */
