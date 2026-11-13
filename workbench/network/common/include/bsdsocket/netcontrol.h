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
    NCMD_SHUTDOWN,      /* orderly stack shutdown (equivalent to CTRL-C) */
    /* The three below take a struct NetControlModuleMsg (mn_Length says so). */
    NCMD_MODULE_QUERY,  /* is the named module of the stack known, started, in use? */
    NCMD_MODULE_START,  /* start it; no error when it already runs */
    NCMD_MODULE_STOP,   /* stop it; refused (EBUSY, EPERM) rather than forced */
    /*
     * A module whose code is in a library file of its own. These two take
     * a struct NetControlModuleMsg as well.
     */
    NCMD_MODULE_LOAD,   /* ncmm_Name: the library file. Load it, start its module;
                           ncmm_Name then holds the module's name */
    NCMD_MODULE_UNLOAD, /* ncmm_Name: the module. Stop it (refused rather than forced)
                           and take its library out of memory */
    /*
     * A module the stack loads on first use (the resolver, the packet
     * services, ...). Takes a struct NetControlModuleMsg as well.
     */
    NCMD_MODULE_DEMAND  /* ncmm_Name: the module. Start it if the stack knows it,
                           else load its library (<name>.atcp) and start
                           it; no error when it runs already */
};

struct NetControlMsg {
    struct Message  ncm_Msg;        /* mn_ReplyPort must be set by the sender */
    ULONG           ncm_Command;    /* NCMD_* */
    LONG            ncm_Result;     /* filled by the stack before ReplyMsg (0 = ok) */
};

/*
 * The stack is built from modules: address families, the packet services,
 * and whatever was added to it (net/netmodule.h in the stack). ncm_Result
 * is 0 or an errno: ENOENT no module of that name, EPERM it can never be
 * stopped, EBUSY it is in use, EINVAL the message is too short.
 *
 * NCMD_MODULE_LOAD: ENOENT no such library file (one named without a path
 * is looked for in the stack's own Libs directory, then in LIBS:), EEXIST
 * it is loaded already or a module of its name is part of the stack,
 * EPROTONOSUPPORT the library and the stack are not made for each other,
 * or what the module's start failed with; on an error nothing stays loaded.
 * NCMD_MODULE_UNLOAD: ENOENT no module of that name, EPERM its code is part
 * of the stack program (or it can never be stopped), EBUSY it is in use -
 * it then stays loaded and running.
 * NCMD_MODULE_DEMAND: ENOENT not installed, or it could not be had before
 * and has not been asked for since (the stack does not read the disk again
 * for it until a RELOAD or a LOAD of it), or its test switch
 * ENV:AROSTCP/Without_<name> is set; ENXIO something it needs is left out
 * by the configuration; or what its start failed with.
 */
#define NCM_MODULE_NAMELEN  32

struct NetControlModuleMsg {
    struct NetControlMsg ncmm_Msg;
    char            ncmm_Name[NCM_MODULE_NAMELEN];  /* in: NUL-terminated */
    ULONG           ncmm_Flags;     /* out: NCMMF_* */
    LONG            ncmm_Users;     /* out: sockets and other users that keep it from stopping */
};

#define NCMMF_STARTED       0x0001
#define NCMMF_PERMANENT     0x0002  /* is never stopped once started */
#define NCMMF_BUILTIN       0x0004  /* its code is part of the stack program */
#define NCMMF_LIBRARY       0x0008  /* its code is in a library file of its own, loaded now */

#endif /* BSDSOCKET_NETCONTROL_H */
