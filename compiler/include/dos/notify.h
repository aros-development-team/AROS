#ifndef DOS_NOTIFY_H
#define DOS_NOTIFY_H

/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Notification handling
    Lang: english
*/

#ifndef EXEC_PORTS_H
#   include <exec/ports.h>
#endif

#ifndef EXEC_TASKS_H
#   include <exec/tasks.h>
#endif

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

struct NotifyMessage
{
    struct Message nm_ExecMessage;

    ULONG                  nm_Class; /* see below */
    UWORD                  nm_Code;  /* see below */
    struct NotifyRequest * nm_NReq;

    ULONG                  nm_DoNotTouch;
    ULONG                  nm_DoNotTouch2;
};

struct NotifyRequest
{
    UBYTE * nr_Name;
    UBYTE * nr_FullName;
    ULONG   nr_UserData;
    ULONG   nr_Flags;

    union
    {
        struct
        {
            struct MsgPort * nr_Port;
        } nr_Msg;
        struct
        {
            struct Task * nr_Task;
            UBYTE         nr_SignalNum;
            UBYTE         nr_pad[3];
        } nr_Signal;
    } nr_stuff;

    ULONG            nr_Reserved[4];
    ULONG            nr_MsgCount;
    struct MsgPort * nr_Handler;
};

#define NOTIFY_CLASS 0x40000000
#define NOTIFY_CODE  0x1234

#define NRB_SEND_MESSAGE        0
#define NRF_SEND_MESSAGE   (1L<<0)
#define NRB_SEND_SIGNAL         1
#define NRF_SEND_SIGNAL    (1L<<1)
#define NRB_WAIT_REPLY          3
#define NRF_WAIT_REPLY     (1L<<3)
#define NRB_NOTIFY_INTIAL       4
#define NRF_NOTIFY_INITIAL (1L<<4)
/* the following definitions are for use by handlers only! */
#define NR_HANDLER_FLAGS 0xffff0000
#define NRB_MAGIC               31
#define NRF_MAGIC          (1L<<31)

#endif /* DOS_NOTIFY_H */
