#ifndef BSDSOCKET_NETCONTROL_H
#define BSDSOCKET_NETCONTROL_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: AROSTCP out-of-band control channel (reconfigure / shutdown).
*/

#include <exec/types.h>
#include <exec/ports.h>         /* struct Message - the control message embeds one */

/*
 * Out-of-band control channel for the stack's main task (AROS addition).
 *
 * The main task's CTRL-C/CTRL-F break bits already mean "shut down" (and the
 * CTRL-F handshake), so a distinct control request - above all a configuration
 * RELOAD that must run in the main task's context, where interface and route
 * teardown/rebuild is serialised against SANA I/O - needs its own channel.
 * This drives the reconfigure the bsdsocket SBTC_RECONFIG_* tags report.
 *
 * The channel is a public, named Exec MsgPort owned by the main task.  A sender
 * (network prefs, an ARexx command, ...) FindPort()s it, PutMsg()es a
 * NetControlMsg with mn_ReplyPort set, and WaitPort()s the reply; ncm_Result
 * carries the outcome (0 == ok).
 *
 * This lives in its own header (not socketbasetags.h) on purpose: socketbasetags.h
 * is pulled in by <netdb.h> for every networking program, and embedding a
 * by-value struct Message there forces <exec/ports.h> on all of them - which
 * clashes with programs that remap Message (e.g. sockperf's -DMessage).
 */
#define AROSTCP_CTRLPORT_NAME   "AROSTCP.ctrl"

/* ncm_Command */
enum {
    NCMD_NOP = 0,
    NCMD_RELOAD,        /* reconfigure in place (stop services, reload config, start) */
    NCMD_SHUTDOWN       /* orderly stack shutdown (equivalent to CTRL-C) */
};

struct NetControlMsg {
    struct Message  ncm_Msg;        /* mn_ReplyPort must be set by the sender */
    ULONG           ncm_Command;    /* NCMD_* */
    LONG            ncm_Result;     /* filled by the stack before ReplyMsg (0 = ok) */
};

#endif /* BSDSOCKET_NETCONTROL_H */
