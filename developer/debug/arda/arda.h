/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    ARDA - AROS Remote Debug Agent: shared definitions.

    The agent is a read-only diagnostic server that speaks a line-oriented
    protocol over TCP.  Where the stack lets the tasks of a program share a
    library base (SBTC_CAN_SHARE_LIBRARY_BASES), every connection is served
    by a process of its own, so a command that blocks holds up only its own
    client; otherwise one connection at a time.  See host/PROTOCOL.md for the
    wire format.
*/

#ifndef ARDA_H
#define ARDA_H

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/semaphores.h>
#include <dos/dos.h>

#define ARDA_PROTOCOL        "1"            /* protocol version, see PROTOCOL.md */
#define ARDA_AGENT_VERSION   "0.5"
#define ARDA_DEFAULT_PORT    7777
#define ARDA_DEFAULT_ADDRESS "127.0.0.1"    /* loopback unless told otherwise */
#define ARDA_DEFAULT_IDLE    60             /* seconds before an idle client is dropped */
#define ARDA_SEND_TIMEOUT    10             /* seconds a blocked send may stall */
#define ARDA_MAX_LINE        256            /* longest accepted request line */
#define ARDA_MAX_RESP        65536          /* response buffer, incl. terminator */
#define ARDA_RESP_RESERVE    32             /* kept free for "END TRUNCATED\n" */
#define ARDA_RECV_CHUNK      512
#define ARDA_MAX_JOBS        8              /* control mode: RUN job slots */
#define ARDA_MAX_CLIENTS     4              /* connections served at the same time */
#define ARDA_CTL_CHUNK       65536          /* control mode: file transfer buffer */

/* A response under construction.  Multi-line responses start with a bare
   "OK" line and finish with "END" (or "END TRUNCATED" when the buffer
   overflowed); single-line responses are "OK <text>" or "ERR <code> <text>". */
struct ArdaResp
{
    char   *buf;
    ULONG   len;
    ULONG   cap;
    BOOL    overflow;
    BOOL    multi;
};

struct ArdaClient
{
    int     fd;                             /* -1 when no client */
    char    line[ARDA_MAX_LINE + 1];
    ULONG   linelen;
    BOOL    discard;                        /* dropping an overlong line */
    BOOL    closing;                        /* close after the pending reply */
    ULONG   idle;                           /* seconds since last input */
    char    peer[48];
    /* control mode: a PUT body in progress (raw bytes, not lines) */
    BPTR    rawfile;
    ULONG   rawleft;
    ULONG   rawtotal;
    BOOL    rawfailed;
    char    rawpath[200];                   /* for the flush when the body is complete */
};

/* control mode: a RUN job; the wrapper process writes rc and finished */
#define ARDA_JOB_FREE       0
#define ARDA_JOB_RUNNING    1
#define ARDA_JOB_DONE       2

struct ArdaJob
{
    ULONG           id;
    ULONG           state;
    volatile BOOL   finished;               /* set by the wrapper, last */
    BOOL            orphaned;               /* agent shut down while it ran */
    LONG            rc;
    ULONG           started, ended;         /* agent uptime seconds */
    struct Process *proc;
    char            output[200];
    char            command[256];
};

/* State shared by the main process and the connection processes. Commands
   run one at a time under lock (they share scratch buffers); a command that
   waits (WAIT, a GET body) lets go of it meanwhile. */
struct ArdaShared
{
    struct SignalSemaphore lock;
    BOOL                shared_base;        /* the stack shares SocketBase between processes */
    struct ArdaJob      jobs[ARDA_MAX_JOBS];
    ULONG               jobseq;
    BOOL                reboot;             /* ColdReboot() once the reply is out */
    ULONG               clients;            /* connection processes running */
    struct Task        *owner;              /* the main process */
    ULONG               conns;              /* accepted connections */
    ULONG               rejected;           /* connections refused as busy */
    ULONG               cmds;               /* commands dispatched */
    ULONG               errors;             /* ERR replies sent */
    ULONG               uptime;             /* seconds since start (tick based) */
};

/* One connection's context (the main process has one too). */
struct Arda
{
    struct ArdaShared  *sh;
    int                 ls;                 /* listening socket, -1 if none */
    ULONG               port;
    ULONG               idle_limit;         /* 0 disables the idle timeout */
    BOOL                quiet;
    BOOL                running;
    BOOL                control;            /* CONTROL switch: files, env, jobs, reboot */
    BOOL                replied;            /* a handler sent its own reply (raw body) */
    LONG                sockerr;            /* this process's socket errno (error hook) */
    char               *ctlbuf;             /* control mode: file transfer buffer */
    struct ArdaClient   client;
    struct ArdaResp     resp;
    char                respbuf[ARDA_MAX_RESP];
};

/* arda.c */
void arda_log(struct Arda *a, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
LONG arda_errno(void);                      /* the calling process's socket errno */
void arda_lock(struct Arda *a);
void arda_unlock(struct Arda *a);
BOOL arda_client_start(struct Arda *a, int fd, const char *peer);

/* arda_net.c */
BOOL arda_net_listen(struct Arda *a, const char *addr, ULONG port);
void arda_net_accept(struct Arda *a);
void arda_net_close_client(struct Arda *a, const char *why);
BOOL arda_net_send(struct Arda *a, int fd, const char *data, ULONG len);
void arda_net_input(struct Arda *a);
void arda_net_reply(struct Arda *a);        /* send a->resp to the client */

/* arda_cmd.c */
void arda_resp_reset(struct ArdaResp *r);
void arda_resp_ok(struct ArdaResp *r, const char *text);
void arda_resp_err(struct ArdaResp *r, int code, const char *text);
void arda_resp_begin(struct ArdaResp *r);
void arda_resp_line(struct ArdaResp *r, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
void arda_resp_end(struct ArdaResp *r);
void arda_dispatch(struct Arda *a, char *line);
void arda_safe_str(char *dst, ULONG cap, const char *src);

/* arda_sys.c */
void arda_sys_init(struct Arda *a);
void arda_sys_exit(void);
void arda_cmd_sysinfo(struct Arda *a, char *args);
void arda_cmd_memory(struct Arda *a, char *args);
void arda_cmd_tasks(struct Arda *a, char *args);

/* arda_log.c */
void  arda_logsrc_init(struct Arda *a);
void  arda_logsrc_exit(void);
ULONG arda_logsrc_sigmask(void);
void  arda_logsrc_poll(struct Arda *a);
void  arda_log_event(const char *text);
void  arda_cmd_log(struct Arda *a, char *args);

/* arda_netdiag.c */
void  arda_cmd_net(struct Arda *a, char *args);

/* arda_ctl.c (control mode) */
void  arda_ctl_init(struct Arda *a);
void  arda_ctl_exit(struct Arda *a);
void  arda_ctl_poll(struct Arda *a);
ULONG arda_ctl_raw_input(struct Arda *a, const char *data, ULONG n);
void  arda_ctl_raw_abort(struct Arda *a);
void  arda_ctl_flush(const char *path);
void  arda_cmd_put(struct Arda *a, char *args);
void  arda_cmd_get(struct Arda *a, char *args);
void  arda_cmd_dir(struct Arda *a, char *args);
void  arda_cmd_delete(struct Arda *a, char *args);
void  arda_cmd_mkdir(struct Arda *a, char *args);
void  arda_cmd_setenv(struct Arda *a, char *args);
void  arda_cmd_getenv(struct Arda *a, char *args);
void  arda_cmd_run(struct Arda *a, char *args);
void  arda_cmd_jobs(struct Arda *a, char *args);
void  arda_cmd_job(struct Arda *a, char *args);
void  arda_cmd_wait(struct Arda *a, char *args);
void  arda_cmd_reboot(struct Arda *a, char *args);

/* arda_mem.c */
void  arda_cmd_symbol(struct Arda *a, char *args);
void  arda_cmd_peek(struct Arda *a, char *args);
void  arda_cmd_stack(struct Arda *a, char *args);

/* arda_dbg.c */
void  arda_cmd_break(struct Arda *a, char *args);
void  arda_cmd_backtrace(struct Arda *a, char *args);
void  arda_cmd_address(struct Arda *a, char *args);
void  arda_cmd_modules(struct Arda *a, char *args);
void  arda_cmd_libs(struct Arda *a, char *args);
void  arda_cmd_devices(struct Arda *a, char *args);
void  arda_cmd_resources(struct Arda *a, char *args);
void  arda_cmd_ports(struct Arda *a, char *args);
void  arda_cmd_semaphores(struct Arda *a, char *args);

#endif /* ARDA_H */
