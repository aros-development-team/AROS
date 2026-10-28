/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    ARDA - response construction and command dispatch.

    Commands are matched case-insensitively on their first word; the rest
    of the line is handed to the handler as the argument string.  Handlers
    only ever read system state and only ever write into the bounded
    response buffer.
*/

#include "arda.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/* ---- response buffer ------------------------------------------------- */

void arda_resp_reset(struct ArdaResp *r)
{
    r->len      = 0;
    r->overflow = FALSE;
    r->multi    = FALSE;
    if (r->cap)
        r->buf[0] = '\0';
}

static void resp_append(struct ArdaResp *r, const char *s, ULONG n)
{
    if (r->overflow)
        return;
    if (r->len + n + ARDA_RESP_RESERVE > r->cap)
    {
        r->overflow = TRUE;
        return;
    }
    memcpy(r->buf + r->len, s, n);
    r->len += n;
    r->buf[r->len] = '\0';
}

void arda_resp_line(struct ArdaResp *r, const char *fmt, ...)
{
    char tmp[1024];
    va_list ap;
    int n;

    va_start(ap, fmt);
    n = vsnprintf(tmp, sizeof(tmp), fmt, ap);
    va_end(ap);
    if (n < 0)
        return;
    if ((size_t)n >= sizeof(tmp))
        n = sizeof(tmp) - 1;
    resp_append(r, tmp, n);
    resp_append(r, "\n", 1);
}

void arda_resp_ok(struct ArdaResp *r, const char *text)
{
    arda_resp_reset(r);
    arda_resp_line(r, "OK %s", (text && *text) ? text : "DONE");
}

void arda_resp_err(struct ArdaResp *r, int code, const char *text)
{
    arda_resp_reset(r);
    arda_resp_line(r, "ERR %d %s", code, text);
}

void arda_resp_begin(struct ArdaResp *r)
{
    arda_resp_reset(r);
    r->multi = TRUE;
    arda_resp_line(r, "OK");
}

void arda_resp_end(struct ArdaResp *r)
{
    if (!r->multi)
        return;
    if (r->overflow)
    {
        /* drop the partial last line, then use the reserved tail */
        while (r->len > 0 && r->buf[r->len - 1] != '\n')
            r->len--;
        r->overflow = FALSE;
        memcpy(r->buf + r->len, "END TRUNCATED\n", 14);
        r->len += 14;
    }
    else
    {
        resp_append(r, "END\n", 4);
    }
    r->buf[r->len] = '\0';
    r->multi = FALSE;
}

/* Copy a possibly-hostile string into a bounded buffer, replacing anything
   that would break the line protocol (quotes, control characters). */
void arda_safe_str(char *dst, ULONG cap, const char *src)
{
    ULONG i = 0;

    if (!src)
        src = "";
    for (; src[i] && i + 1 < cap; i++)
    {
        unsigned char c = (unsigned char)src[i];
        dst[i] = (c < 0x20 || c > 0x7e || c == '"') ? '?' : (char)c;
    }
    dst[i] = '\0';
}

/* ---- commands --------------------------------------------------------- */

typedef void (*ArdaHandler)(struct Arda *a, char *args);

struct ArdaCommand
{
    const char  *name;
    ArdaHandler  fn;
    const char  *help;
};

static void cmd_ping(struct Arda *a, char *args)
{
    arda_resp_ok(&a->resp, "PONG");
}

static void cmd_version(struct Arda *a, char *args)
{
    arda_resp_ok(&a->resp, a->control ? "ARDA/" ARDA_PROTOCOL " agent " ARDA_AGENT_VERSION " control"
                                      : "ARDA/" ARDA_PROTOCOL " agent " ARDA_AGENT_VERSION);
}

static const struct ArdaCommand commands[];

static void cmd_help(struct Arda *a, char *args)
{
    const struct ArdaCommand *c;

    arda_resp_begin(&a->resp);
    for (c = commands; c->name; c++)
        arda_resp_line(&a->resp, "%-8s %s", c->name, c->help);
    arda_resp_end(&a->resp);
}

static void cmd_status(struct Arda *a, char *args)
{
    arda_resp_begin(&a->resp);
    arda_resp_line(&a->resp, "PROTOCOL=%s", ARDA_PROTOCOL);
    arda_resp_line(&a->resp, "AGENT=%s", ARDA_AGENT_VERSION);
    arda_resp_line(&a->resp, "UPTIME=%lu", (unsigned long)a->uptime);
    arda_resp_line(&a->resp, "PORT=%lu", (unsigned long)a->port);
    arda_resp_line(&a->resp, "IDLE_LIMIT=%lu", (unsigned long)a->idle_limit);
    arda_resp_line(&a->resp, "PEER=%s", a->client.peer);
    arda_resp_line(&a->resp, "CONNECTIONS=%lu", (unsigned long)a->conns);
    arda_resp_line(&a->resp, "REJECTED=%lu", (unsigned long)a->rejected);
    arda_resp_line(&a->resp, "COMMANDS=%lu", (unsigned long)a->cmds);
    arda_resp_line(&a->resp, "ERRORS=%lu", (unsigned long)a->errors);
    arda_resp_line(&a->resp, "MAX_LINE=%d", ARDA_MAX_LINE);
    arda_resp_line(&a->resp, "MAX_RESPONSE=%d", ARDA_MAX_RESP);
    arda_resp_line(&a->resp, "CONTROL=%d", a->control ? 1 : 0);
    arda_resp_end(&a->resp);
}

static void cmd_quit(struct Arda *a, char *args)
{
    arda_resp_ok(&a->resp, "BYE");
    a->client.closing = TRUE;
}

static const struct ArdaCommand commands[] =
{
    { "PING",    cmd_ping,    "liveness check, replies OK PONG"              },
    { "VERSION", cmd_version, "protocol and agent version"                   },
    { "HELP",    cmd_help,    "list the supported commands"                  },
    { "STATUS",  cmd_status,  "agent self status and counters"               },
    { "SYSINFO", arda_cmd_sysinfo, "OS, kernel and CPU information"          },
    { "MEMORY",  arda_cmd_memory,  "memory statistics and memory headers"    },
    { "TASKS",   arda_cmd_tasks,   "task snapshot; optional name filter"     },
    { "LOG",     arda_cmd_log,     "STATUS | READ [n] [SINCE=seq] [SRC=x] | CLEAR" },
    { "NET",     arda_cmd_net,     "STATUS | INTERFACES | ROUTES | ARP | STATS [IP|TCP|UDP]" },
    { "QUIT",    cmd_quit,    "close this connection"                        },
    /* control mode (CONTROL switch): these change state */
    { "PUT",     arda_cmd_put,    "(control) <path> <size>: write the raw body that follows to a file" },
    { "GET",     arda_cmd_get,    "(control) <path>: OK SIZE <n> then the file's raw bytes" },
    { "DIR",     arda_cmd_dir,    "(control) [path]: directory entries"      },
    { "DELETE",  arda_cmd_delete, "(control) <path>: delete a file"          },
    { "MKDIR",   arda_cmd_mkdir,  "(control) <path>: create a directory"     },
    { "SETENV",  arda_cmd_setenv, "(control) <name> [value]: set or unset a global env variable" },
    { "GETENV",  arda_cmd_getenv, "(control) <name>: read a global env variable" },
    { "RUN",     arda_cmd_run,    "(control) <output-file> <command line>: start a job, OK JOB <n>" },
    { "JOBS",    arda_cmd_jobs,   "(control) list jobs"                      },
    { "JOB",     arda_cmd_job,    "(control) <n>: one job's record"          },
    { "WAIT",    arda_cmd_wait,   "(control) <n> [secs]: wait for a job, OK RC <rc>" },
    { "REBOOT",  arda_cmd_reboot, "(control) cold reboot after the reply"    },
    { NULL,      NULL,        NULL                                           }
};

/* ---- dispatch --------------------------------------------------------- */

void arda_dispatch(struct Arda *a, char *line)
{
    const struct ArdaCommand *c;
    char *word, *args, *p;

    /* trim leading blanks; an empty line is silently ignored */
    while (*line == ' ' || *line == '\t')
        line++;
    if (*line == '\0')
        return;

    word = line;
    for (p = line; *p && *p != ' ' && *p != '\t'; p++)
        *p = toupper((unsigned char)*p);
    if (*p)
    {
        *p++ = '\0';
        while (*p == ' ' || *p == '\t')
            p++;
    }
    args = p;
    /* trim trailing blanks from the arguments */
    p = args + strlen(args);
    while (p > args && (p[-1] == ' ' || p[-1] == '\t'))
        *--p = '\0';

    a->cmds++;
    for (c = commands; c->name; c++)
    {
        if (strcmp(c->name, word) == 0)
            break;
    }

    if (c->name)
    {
        arda_resp_reset(&a->resp);
        a->replied = FALSE;
        c->fn(a, args);
        if (a->replied)
        {
            /* the handler sent its own bytes (GET) */
            a->replied = FALSE;
            return;
        }
        if (a->resp.multi)
            arda_resp_end(&a->resp);
        if (a->resp.len == 0)
            arda_resp_err(&a->resp, 500, "command produced no response");
    }
    else
    {
        arda_resp_err(&a->resp, 404, "unknown command, try HELP");
    }

    if (strncmp(a->resp.buf, "ERR ", 4) == 0)
        a->errors++;

    arda_net_reply(a);
}
