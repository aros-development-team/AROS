#ifndef DOS_NOTIFY_H
#define DOS_NOTIFY_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Notification handling.
    Lang: English
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

/**********************************************************************
 **************************** NotifyRequest ***************************
 **********************************************************************/

/* General notification structure as passed to StartNotify() and EndNotify().
   After passing it to StartNotify() the first time, this structure becomes
   READ-ONLY! */
struct NotifyRequest
{
    STRPTR  nr_Name;     /* Name of the watched file. */
    STRPTR  nr_FullName; /* Fully qualified name of the watched file. This is
                            READ-ONLY! */
    IPTR    nr_UserData; /* Fill in with your own data. */
    ULONG   nr_Flags;    /* see below */

    /* The following union specified the way to notify the application, if
       the watched file changes. IF NRF_SEND_MESSAGE is set, nr_Msg is
       used, when NRF_SEND_SIGNAL is set, nr_Signal is used. */
    union
    {
        struct
        {
            struct MsgPort * nr_Port; /* Port to send message to. */
        } nr_Msg;
        struct
        {
            struct Task * nr_Task;      /* Task to notify. */
            UBYTE         nr_SignalNum; /* Signal number to set. */
            UBYTE         nr_pad[3];    /* PRIVATE */
        } nr_Signal;
    } nr_stuff;

    ULONG            nr_Reserved[4]; /* PRIVATE! Set to 0 for now. */

    /* The following fields are for PRIVATE use by handlers. */
    ULONG            nr_MsgCount; /* Number of unreplied messages. */

    /* This used to be   struct MsgPort *nr_Handler   but as AROS filesystems
       are different and this is a PRIVATE field anyway no Amiga programs
       should use it, so I changed it   --  SDuvan */
    struct Device   *nr_Device;
};

/* nr_Flags */
/* The two following flags specify by which means the watching task is to be
   notified. */
#define NRB_SEND_MESSAGE  0 /* Send a message to the specified message port. */
#define NRB_SEND_SIGNAL   1 /* Set a signal of the specified task. */
#define NRB_WAIT_REPLY    3 /* Wait for a reply by the application before
                               going on with watching? */
#define NRB_NOTIFY_INITIAL 4 /* Notify if the file/directory exists when
                                the notification request is posted */

#define NRF_SEND_MESSAGE   (1L<<NRB_SEND_MESSAGE)
#define NRF_SEND_SIGNAL    (1L<<NRB_SEND_SIGNAL)
#define NRF_WAIT_REPLY     (1L<<NRB_WAIT_REPLY)
#define NRF_NOTIFY_INITIAL (1L<<NRB_NOTIFY_INITIAL)

/* The following flags are for use by handlers only! */
#define NR_HANDLER_FLAGS 0xffff0000
#define NRB_MAGIC               31
#define NRF_MAGIC          (1L<<31)

/**********************************************************************
 **************************** NotifyMessage ***************************
 **********************************************************************/

/* The NotifyMessage if send to the message port specified in
   NotifyRequest->nr_Msg->nr_Port, if NRF_SEND_MESSAGE was set in
   NotifyRequest->nr_Flags and the watched file changes. */
struct NotifyMessage
{
      /* Embedded message structure as defined in <exec/ports.h>. */
    struct Message nm_ExecMessage;

    ULONG                  nm_Class; /* see below */
    UWORD                  nm_Code;  /* see below */
      /* The notify structure that was passed to StartNotify(). */
    struct NotifyRequest * nm_NReq;

    /* The following two fields are for PRIVATE use by handlers. */
    IPTR nm_DoNotTouch;
    IPTR nm_DoNotTouch2;
};

/* nm_Class. Do not use, yet. */
#define NOTIFY_CLASS 0x40000000

/* nm_Code. Do not use, yet. */
#define NOTIFY_CODE  0x1234

#endif /* DOS_NOTIFY_H */
