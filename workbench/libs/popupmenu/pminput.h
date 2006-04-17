#ifndef PM_INPUT_H
#define PM_INPUT_H

#define PM_MSG_TIMER        1
#define PM_MSG_RAWMOUSE     2
#define PM_MSG_DOWN         3
#define PM_MSG_UP           4
#define PM_MSG_SELECT       5
#define PM_MSG_MULTISELECT  6
#define PM_MSG_TERMINATE    7
#define PM_MSG_OPENSUB      8
#define PM_MSG_CLOSESUB     9
#define PM_MSG_DEBUGINFO    10

struct PM_InpMsg {
    struct Message      msg;        // Message struct
    UWORD               Kind;       // Kind of message
    UWORD               Code;       // InputEvent code
    UWORD               Qual;       // Qualifier
    UWORD               Res;        // Reserved

};

struct PM_InputHandler {
    struct MsgPort      *mp;        // Replyport
    struct MsgPort      *port;      // Port for input events
    struct IOStdReq     *ior;       // IO Request
    struct Interrupt    intr;       // Interrupt structure
    int                 error;      // OpenDevice error
};

void PM_RemoveHandler(struct PM_InputHandler *pmh);
struct PM_InputHandler *PM_InstallHandler(int pri);

#endif

