#ifndef DRADIOTOOL_H
#define DRADIOTOOL_H

/* dradio vendor specific requests */

#define CMDID_GETSTEREO 0
#define CMDID_SETFREQ   1
#define CMDID_POWER     2

#define PLLFREQ  10700000
#define PLLSTEP   12500
#define MIN_FREQ  60000
#define MAX_FREQ 160000

struct NepClassDRadio
{
    struct Node         nch_Node;         /* Node linkage */
    struct PsdDevice   *nch_Device;       /* Up linkage */
    struct PsdConfig   *nch_Config;       /* Up linkage */
    struct PsdInterface *nch_Interface;   /* Up linkage */
    struct PsdPipe     *nch_EP0Pipe;      /* Endpoint 0 pipe */
    struct Hook         nch_ReleaseHook;  /* Hook for release function */
    struct Task        *nch_Task;         /* this task */
    struct MsgPort     *nch_TaskMsgPort;  /* io message port */
    UBYTE               nch_Buf[8];       /* receive buffer */
};

#endif /* DRADIOTOOL_H */
