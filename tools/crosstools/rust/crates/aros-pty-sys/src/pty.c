/* A pseudo console for Rust on AROS. Contract: pty.h.

   The handler follows con-handler's packet handling
   (rom/filesys/console_handler/con_handler.c). It only uses exec and dos,
   no C library: it runs as a process of its own. */
#include <stdint.h>
#include <exec/memory.h>
#include <exec/semaphores.h>
#include <exec/lists.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <devices/timer.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <aros/asmcall.h>
#include <aros/macros.h>

#include "pty.h"

/* 1 for serial debug output. */
#define DEBUG 0
#include <aros/debug.h>

#define CSI 0x9b
#define ESC 0x1b

/* SetMode() argument for xterm programs: bytes pass both ways untouched,
   UTF-8 included, and no key is a break. Disk info then reports XTRM.
   On other consoles it is plain raw mode. */
#define MODE_TRANSPARENT 2

#define MAX_NAME    256
#define MAX_MATCHES 1024
#define MAX_HISTORY 32      /* as con-handler's CMD_HISTORY_SIZE */

/* A byte queue: data[start..end] is pending. */
struct buffer {
    UBYTE *data;
    ULONG start, end, cap;
};

static BOOL buffer_append(struct buffer *b, const UBYTE *src, ULONG len)
{
    if (b->end + len > b->cap) {
        ULONG used = b->end - b->start;
        if (used + len <= b->cap) {
            /* Room once the consumed part is dropped. */
            ULONG i;
            for (i = 0; i < used; i++)
                b->data[i] = b->data[b->start + i];
        } else {
            ULONG cap = b->cap ? b->cap : 256;
            UBYTE *data;
            while (cap < used + len)
                cap *= 2;
            data = AllocVec(cap, MEMF_ANY);
            if (!data)
                return FALSE;
            if (used)
                CopyMem(b->data + b->start, data, used);
            FreeVec(b->data);
            b->data = data;
            b->cap = cap;
        }
        b->start = 0;
        b->end = used;
    }
    CopyMem((APTR)src, b->data + b->end, len);
    b->end += len;
    return TRUE;
}

static ULONG buffer_size(const struct buffer *b)
{
    return b->end - b->start;
}

/* Moves up to `len` bytes out of the queue. */
static ULONG buffer_take(struct buffer *b, UBYTE *dst, ULONG len)
{
    ULONG n = buffer_size(b);
    if (n > len)
        n = len;
    if (n)
        CopyMem(b->data + b->start, dst, n);
    b->start += n;
    if (b->start == b->end)
        b->start = b->end = 0;
    return n;
}

static void buffer_free(struct buffer *b)
{
    FreeVec(b->data);
    b->data = NULL;
    b->start = b->end = b->cap = 0;
}

struct aros_pty {
    /* Shared with the terminal side, under `lock`. */
    struct SignalSemaphore lock;
    struct buffer output;       /* for the terminal, translated */
    struct buffer input;        /* from the terminal, ISO-8859-1 */
    struct Task *reader;        /* waiting in aros_pty_read */
    ULONG reader_mask;
    struct Task *closer;        /* waiting in aros_pty_close */
    ULONG closer_mask;
    LONG cols, rows;
    BOOL transparent;           /* MODE_TRANSPARENT */
    BOOL closing;
    BOOL gone;                  /* the handler has finished */

    /* Set up by the handler before it reports in. */
    struct Process *handler;
    struct MsgPort *port;
    ULONG input_mask;
    struct Task *starter;
    ULONG starter_mask;

    /* The handler's own. */
    LONG usecount;
    BOOL raw;
    BOOL xterm_seen;            /* the program wrote a DEC private sequence */
    BOOL eof_pending;           /* Ctrl-\ typed on an empty line */
    struct Task *breaktask;
    struct buffer line;         /* cooked mode: the line being edited */
    ULONG line_pos;             /* the cursor in it */
    struct buffer ready;        /* what reads are given */
    struct MinList reads;       /* pending ACTION_READ packets */
    struct DosPacket *waiting;  /* pending ACTION_WAIT_CHAR */
    struct MsgPort *timer_port;
    struct timerequest *timer;
    struct buffer out_line;     /* program output since the last newline */
    /* Output escape parser. */
    UBYTE out_state;
    UBYTE out_params[32];
    UBYTE out_len;
    BOOL bounds_pending;        /* a window bounds request to answer */
    /* Tab cycling through the matches of the last listing. */
    struct MinList cycle;
    ULONG cycle_count;
    LONG cycle_index;
    ULONG cycle_start;          /* where the completed word starts */
    UBYTE cycle_dir[MAX_NAME];  /* its directory, as typed */
    ULONG cycle_dirlen;
    BOOL cycle_quoted;
    BOOL cycling;
    /* Entered lines, oldest first; history_view == history_count on a new line. */
    struct buffer history[MAX_HISTORY];
    ULONG history_count;
    ULONG history_view;
};


/* As con-handler: reply through our process port. */
static void replypkt2(struct DosPacket *dp, SIPTR res1, SIPTR res2)
{
    struct MsgPort *mp = dp->dp_Port;
    struct Message *mn = dp->dp_Link;

    mn->mn_Node.ln_Name = (char *)dp;
    dp->dp_Port = &((struct Process *)FindTask(NULL))->pr_MsgPort;
    dp->dp_Res1 = res1;
    dp->dp_Res2 = res2;
    PutMsg(mp, mn);
}

static void replypkt(struct DosPacket *dp, SIPTR res1)
{
    replypkt2(dp, res1, 0);
}

/* --- Output: AROS programs to the terminal ---------------------------- */

static void emit(struct buffer *out, const char *text)
{
    ULONG len = 0;
    while (text[len])
        len++;
    buffer_append(out, (const UBYTE *)text, len);
}

/* Decimal for the window bounds report. */
static void emit_number(struct buffer *out, LONG n)
{
    UBYTE digits[12];
    int i = sizeof(digits);
    if (n < 0)
        n = 0;
    do {
        digits[--i] = '0' + n % 10;
        n /= 10;
    } while (n);
    buffer_append(out, digits + i, sizeof(digits) - i);
}

static void serve_reads(struct aros_pty *pty);

/* An xterm program in raw mode: it gets xterm keys and moves the cursor
   itself, as on a Unix tty with OPOST and ISIG off. The Amiga console never
   uses DEC private modes (CSI ?), so writing one gives such a program away. */
static BOOL xterm_active(struct aros_pty *pty)
{
    return pty->raw && pty->xterm_seen;
}

/* Plain output: newlines, form feed and ISO-8859-1. */
static void out_char(struct aros_pty *pty, struct buffer *out, UBYTE c)
{
    UBYTE utf8[2];

    if (c == '\n' && !xterm_active(pty))
        /* The Amiga console starts a new line; a terminal only moves down. */
        emit(out, "\r\n");
    else if (c == '\f' && !xterm_active(pty))
        emit(out, "\x1b[H\x1b[2J");
    else if (c < 0x80)
        buffer_append(out, &c, 1);
    else {
        utf8[0] = 0xc0 | (c >> 6);
        utf8[1] = 0x80 | (c & 0x3f);
        buffer_append(out, utf8, 2);
    }
}

static void out_csi(struct aros_pty *pty, struct buffer *out, UBYTE final)
{
    const UBYTE *params = pty->out_params;
    ULONG len = pty->out_len;

    if (len && params[0] == '?' && !pty->xterm_seen) {
        D(bug("[aros-pty] xterm program\n"));
        pty->xterm_seen = TRUE;
    }
    /* CSI > n m sets the window background pen: nothing to map it to.
       Colours otherwise pass as they are: the terminal's palette is the
       Workbench's, colour n being pen n. */
    if (len && params[0] == '>' && !xterm_active(pty))
        return;
    /* CSI 0 SPACE q asks for the console size, answered as a window bounds
       report read like input. In xterm it sets the cursor style. */
    if (final == 'q' && !xterm_active(pty) && len >= 1 && params[len - 1] == ' '
            && (len == 1 || (len == 2 && params[0] == '0'))) {
        pty->bounds_pending = TRUE;
        return;
    }
    emit(out, "\x1b[");
    buffer_append(out, params, len);
    buffer_append(out, &final, 1);
}

/* Translates console output for the terminal, which expects UTF-8 and
   7-bit escapes, and hands it to the reader. */
static void write_output(struct aros_pty *pty, const UBYTE *src, ULONG len)
{
    enum { NORMAL, ESCAPE, PARAMS };
    struct buffer *out = &pty->output;
    BOOL bounds;
    LONG cols, rows;
    ULONG i;

    ObtainSemaphore(&pty->lock);
    if (pty->transparent) {
        buffer_append(out, src, len);
        if (pty->reader)
            Signal(pty->reader, pty->reader_mask);
        ReleaseSemaphore(&pty->lock);
        return;
    }
    for (i = 0; i < len; i++) {
        UBYTE c = src[i];

        switch (pty->out_state) {
        case ESCAPE:
            pty->out_state = NORMAL;
            if (c == '[') {
                pty->out_state = PARAMS;
                pty->out_len = 0;
            } else {
                emit(out, "\x1b");
                out_char(pty, out, c);
            }
            break;
        case PARAMS:
            if (c >= 0x20 && c <= 0x3f) {
                if (pty->out_len < sizeof(pty->out_params))
                    pty->out_params[pty->out_len++] = c;
                break;
            }
            pty->out_state = NORMAL;
            if (c >= 0x40 && c <= 0x7e) {
                out_csi(pty, out, c);
            } else {
                emit(out, "\x1b[");
                buffer_append(out, pty->out_params, pty->out_len);
                out_char(pty, out, c);
            }
            break;
        default:
            if (c == CSI) {
                pty->out_state = PARAMS;
                pty->out_len = 0;
            } else if (c == ESC) {
                pty->out_state = ESCAPE;
            } else {
                out_char(pty, out, c);
            }
            break;
        }
    }
    bounds = pty->bounds_pending;
    pty->bounds_pending = FALSE;
    cols = pty->cols;
    rows = pty->rows;
    if (pty->reader)
        Signal(pty->reader, pty->reader_mask);
    ReleaseSemaphore(&pty->lock);

    if (bounds) {
        buffer_append(&pty->ready, (const UBYTE *)"\x9b" "1;1;", 5);
        emit_number(&pty->ready, rows);
        buffer_append(&pty->ready, (const UBYTE *)";", 1);
        emit_number(&pty->ready, cols);
        buffer_append(&pty->ready, (const UBYTE *)" r", 2);
        serve_reads(pty);
    }
}

static void echo(struct aros_pty *pty, const char *text)
{
    ULONG len = 0;
    while (text[len])
        len++;
    write_output(pty, (const UBYTE *)text, len);
}

/* --- Input: the terminal to AROS programs ----------------------------- */

static void serve_reads(struct aros_pty *pty)
{
    struct Node *node;

    while ((node = (struct Node *)pty->reads.mlh_Head)->ln_Succ) {
        struct DosPacket *dp = (struct DosPacket *)node->ln_Name;

        if (buffer_size(&pty->ready)) {
            Remove(node);
            replypkt(dp, buffer_take(&pty->ready, (UBYTE *)dp->dp_Arg2, dp->dp_Arg3));
        } else if (pty->closing || pty->eof_pending) {
            Remove(node);
            pty->eof_pending = FALSE;
            replypkt(dp, 0);
        } else {
            break;
        }
    }

    if (pty->waiting && (buffer_size(&pty->ready) || pty->closing)) {
        AbortIO((struct IORequest *)pty->timer);
        WaitIO((struct IORequest *)pty->timer);
        replypkt(pty->waiting, buffer_size(&pty->ready) ? DOSTRUE : DOSFALSE);
        pty->waiting = NULL;
    }
}

static void send_break(struct aros_pty *pty, UBYTE c)
{
    /* Ctrl-C to Ctrl-F are the four break signals. */
    D(bug("[aros-pty] break 0x%02x to '%s'\n", (unsigned int)c,
        pty->breaktask ? pty->breaktask->tc_Node.ln_Name : "(none)"));
    if (pty->breaktask)
        Signal(pty->breaktask, SIGBREAKF_CTRL_C << (c - 0x03));
}

/* Length of the escape sequence at src[0] (ESC), or 0 if it is cut off. */
static ULONG escape_length(const UBYTE *src, ULONG len)
{
    ULONG i;

    if (len < 2)
        return 0;
    if (src[1] == 'O')
        return len >= 3 ? 3 : 0;
    if (src[1] != '[')
        return 2;
    for (i = 2; i < len; i++)
        if (src[i] >= 0x40 && src[i] <= 0x7e)
            return i + 1;
    return 0;
}

/* xterm function key numbers (CSI n ~) for F5 to F10. */
static const UBYTE fkeys[] = { 15, 17, 18, 19, 20, 21 };

/* Raw mode wants Amiga console sequences: CSI for ESC [, and function keys
   as CSI 0~ to CSI 9~. */
static void raw_escape(struct aros_pty *pty, const UBYTE *seq, ULONG len)
{
    UBYTE out[16];
    ULONG n = 0, i, k;

    if (len == 3 && seq[1] == 'O' && seq[2] >= 'P' && seq[2] <= 'S') {
        out[n++] = CSI;
        out[n++] = '0' + (seq[2] - 'P');
        out[n++] = '~';
    } else if (len >= 3 && seq[1] == '[') {
        ULONG num = 0;
        BOOL digits = len >= 4 && seq[len - 1] == '~';

        for (k = 2; digits && k < len - 1; k++) {
            if (seq[k] < '0' || seq[k] > '9')
                digits = FALSE;
            else
                num = num * 10 + (seq[k] - '0');
        }
        out[n++] = CSI;
        for (i = 0; digits && i < sizeof(fkeys); i++)
            if (fkeys[i] == num)
                break;
        if (digits && i < sizeof(fkeys)) {
            out[n++] = '4' + i;
            out[n++] = '~';
        } else {
            for (k = 2; k < len && n < sizeof(out); k++)
                out[n++] = seq[k];
        }
    } else {
        for (k = 0; k < len && n < sizeof(out); k++)
            out[n++] = seq[k];
    }
    buffer_append(&pty->ready, out, n);
}

/* --- Line editing ---------------------------------------------------- */

/* Moves the terminal cursor `n` columns, left when negative. */
static void cursor_move(struct aros_pty *pty, LONG n)
{
    UBYTE seq[16], digits[10];
    ULONG len = 0, count = n < 0 ? -n : n, d = 0;

    if (n == 0)
        return;
    seq[len++] = ESC;
    seq[len++] = '[';
    do {
        digits[d++] = '0' + count % 10;
        count /= 10;
    } while (count);
    while (d)
        seq[len++] = digits[--d];
    seq[len++] = n < 0 ? 'D' : 'C';
    write_output(pty, seq, len);
}

static void line_move(struct aros_pty *pty, ULONG pos)
{
    cursor_move(pty, (LONG)pos - (LONG)pty->line_pos);
    pty->line_pos = pos;
}

/* Inserts at the cursor and redraws the rest of the line. */
static void line_insert(struct aros_pty *pty, const UBYTE *text, ULONG len)
{
    ULONG size = buffer_size(&pty->line), i;
    UBYTE *line;

    if (!buffer_append(&pty->line, text, len))
        return;
    line = pty->line.data + pty->line.start;
    for (i = size; i-- > pty->line_pos; )
        line[i + len] = line[i];
    for (i = 0; i < len; i++)
        line[pty->line_pos + i] = text[i];
    size += len;
    write_output(pty, line + pty->line_pos, size - pty->line_pos);
    pty->line_pos += len;
    cursor_move(pty, -(LONG)(size - pty->line_pos));
}

/* Removes `n` characters from `at` on; the cursor ends up at `at`. */
static void line_delete(struct aros_pty *pty, ULONG at, ULONG n)
{
    UBYTE *line = pty->line.data + pty->line.start;
    ULONG size = buffer_size(&pty->line), i;

    if (n == 0)
        return;
    line_move(pty, at);
    for (i = at; i + n < size; i++)
        line[i] = line[i + n];
    pty->line.end -= n;
    size -= n;
    write_output(pty, line + at, size - at);
    echo(pty, "\x1b[K");
    cursor_move(pty, -(LONG)(size - at));
}

static void erase_line(struct aros_pty *pty)
{
    line_delete(pty, 0, buffer_size(&pty->line));
    pty->line.start = pty->line.end = 0;
}

/* The line went to the reader. */
static void line_done(struct aros_pty *pty)
{
    pty->line.start = pty->line.end = 0;
    pty->line_pos = 0;
}

/* --- Filename completion, as con-handler's (completion.c) -------------- */

struct match {
    struct MinNode node;
    UBYTE name[1];
};

static ULONG str_len(const UBYTE *s)
{
    ULONG n = 0;
    while (s[n])
        n++;
    return n;
}

static UBYTE to_lower(UBYTE c)
{
    if ((c >= 'A' && c <= 'Z') || (c >= 0xc0 && c <= 0xde && c != 0xd7))
        return c + 0x20;
    return c;
}

static LONG compare_nocase(const UBYTE *a, const UBYTE *b)
{
    while (*a && to_lower(*a) == to_lower(*b)) {
        a++;
        b++;
    }
    return (LONG)to_lower(*a) - (LONG)to_lower(*b);
}

/* Adds `name` and its suffix ('/' drawer, ':' volume) to the sorted list. */
static void add_match(struct MinList *list, ULONG *count, const UBYTE *name, UBYTE suffix)
{
    ULONG len = str_len(name);
    struct MinNode *pos, *prev = NULL;
    struct match *m;

    if (*count >= MAX_MATCHES || len >= MAX_NAME)
        return;
    m = AllocVec(sizeof(*m) + len + 1, MEMF_ANY);
    if (!m)
        return;
    CopyMem((APTR)name, m->name, len);
    m->name[len] = suffix;
    m->name[len + (suffix ? 1 : 0)] = 0;

    for (pos = list->mlh_Head; pos->mln_Succ; pos = pos->mln_Succ) {
        LONG cmp = compare_nocase(m->name, ((struct match *)pos)->name);
        if (cmp == 0) {
            FreeVec(m);
            return;
        }
        if (cmp < 0)
            break;
        prev = pos;
    }
    Insert((struct List *)list, (struct Node *)m, (struct Node *)prev);
    (*count)++;
}

static void scan_dir(const UBYTE *dir, const UBYTE *pattern, struct MinList *list, ULONG *count)
{
    struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);
    BPTR lock;

    if (!fib)
        return;
    if ((lock = Lock((CONST_STRPTR)dir, SHARED_LOCK))) {
        if (Examine(lock, fib))
            while (ExNext(lock, fib))
                if (MatchPatternNoCase((CONST_STRPTR)pattern, fib->fib_FileName))
                    add_match(list, count, (const UBYTE *)fib->fib_FileName,
                        fib->fib_DirEntryType > 0 ? '/' : 0);
        UnLock(lock);
    }
    FreeDosObject(DOS_FIB, fib);
}

static void scan_volumes(const UBYTE *pattern, struct MinList *list, ULONG *count)
{
    ULONG flags = LDF_READ | LDF_VOLUMES | LDF_DEVICES | LDF_ASSIGNS;
    struct DosList *dl = LockDosList(flags);

    while ((dl = NextDosEntry(dl, flags & ~LDF_READ))) {
        const UBYTE *name = (const UBYTE *)AROS_BSTR_ADDR(dl->dol_Name);
        if (MatchPatternNoCase((CONST_STRPTR)pattern, (CONST_STRPTR)name))
            add_match(list, count, name, ':');
    }
    UnLockDosList(flags);
}

static void cycle_reset(struct aros_pty *pty)
{
    struct MinNode *node;

    while ((node = (struct MinNode *)RemHead((struct List *)&pty->cycle)))
        FreeVec(node);
    pty->cycling = FALSE;
}

/* The completed word: the directory as typed and the name, quoted if there
   is a space, and a finished file name gets a space after it. */
static ULONG completion_text(UBYTE *text, const UBYTE *dir, ULONG dirlen,
    const UBYTE *name, ULONG namelen, BOOL quoted, BOOL finished)
{
    BOOL space = FALSE;
    ULONG i, n = 0;

    for (i = 0; i < namelen; i++)
        if (name[i] == ' ')
            space = TRUE;
    for (i = 0; i < dirlen; i++)
        if (dir[i] == ' ')
            space = TRUE;
    if (space && !quoted)
        text[n++] = '"';
    CopyMem((APTR)dir, text + n, dirlen);
    n += dirlen;
    CopyMem((APTR)name, text + n, namelen);
    n += namelen;
    if (finished) {
        if (space || quoted)
            text[n++] = '"';
        text[n++] = ' ';
    }
    return n;
}

static BOOL is_file(const UBYTE *name, ULONG len)
{
    return len && name[len - 1] != '/' && name[len - 1] != ':';
}

/* Replaces the line from `start` on with `text`, on screen too. */
static void replace_word(struct aros_pty *pty, ULONG start, const UBYTE *text, ULONG len)
{
    line_delete(pty, start, buffer_size(&pty->line) - start);
    line_insert(pty, text, len);
}

/* More than one match and nothing in common to add: list them, then put the
   prompt and the line back. */
static void list_matches(struct aros_pty *pty, struct MinList *list)
{
    struct MinNode *pos;
    ULONG width = 0, cols, column = 0;

    for (pos = list->mlh_Head; pos->mln_Succ; pos = pos->mln_Succ) {
        ULONG len = str_len(((struct match *)pos)->name);
        if (len > width)
            width = len;
    }
    width += 2;
    ObtainSemaphore(&pty->lock);
    cols = pty->cols > 0 ? (ULONG)pty->cols / width : 1;
    ReleaseSemaphore(&pty->lock);
    if (cols == 0)
        cols = 1;

    echo(pty, "\n");
    for (pos = list->mlh_Head; pos->mln_Succ; pos = pos->mln_Succ) {
        const UBYTE *name = ((struct match *)pos)->name;
        ULONG len = str_len(name);

        write_output(pty, name, len);
        if (++column == cols || !pos->mln_Succ->mln_Succ) {
            echo(pty, "\n");
            column = 0;
        } else {
            while (len++ < width)
                echo(pty, " ");
        }
    }
    write_output(pty, pty->out_line.data + pty->out_line.start, buffer_size(&pty->out_line));
    write_output(pty, pty->line.data + pty->line.start, buffer_size(&pty->line));
    pty->line_pos = buffer_size(&pty->line);
}

static void complete(struct aros_pty *pty, BOOL withinfo)
{
    struct Node *read = (struct Node *)pty->reads.mlh_Head;
    struct Task *task;
    const UBYTE *line = pty->line.data + pty->line.start;
    ULONG linelen = buffer_size(&pty->line), start = 0, len, typed, count = 0, i;
    UBYTE word[MAX_NAME], dirpart[MAX_NAME], filepart[MAX_NAME + 16];
    UBYTE pattern[2 * (MAX_NAME + 16) + 2], text[2 * MAX_NAME + 4];
    BOOL quoted = FALSE;
    struct MinList matches;
    struct MinNode *pos;
    BPTR lock, olddir;
    LONG parsed;

    /* Tab after a listing goes through the matches one by one. */
    if (pty->cycling) {
        struct MinNode *node = pty->cycle.mlh_Head;
        const UBYTE *name;
        LONG k;

        pty->cycle_index = (pty->cycle_index + 1) % (LONG)pty->cycle_count;
        for (k = 0; k < pty->cycle_index; k++)
            node = node->mln_Succ;
        name = ((struct match *)node)->name;
        len = str_len(name);
        replace_word(pty, pty->cycle_start, text,
            completion_text(text, pty->cycle_dir, pty->cycle_dirlen, name, len,
                pty->cycle_quoted, is_file(name, len)));
        return;
    }

    /* Names are relative to whoever waits for this line: usually the shell. */
    if (!read->ln_Succ)
        return;
    task = ((struct DosPacket *)read->ln_Name)->dp_Port->mp_SigTask;
    if (!task || task->tc_Node.ln_Type != NT_PROCESS)
        return;

    for (i = 0; i < linelen; i++) {
        switch (line[i]) {
        case '"':
            quoted = !quoted;
            if (quoted)
                start = i + 1;
            break;
        case ' ':
        case '<':
        case '>':
            if (!quoted)
                start = i + 1;
            break;
        }
    }
    len = linelen - start;
    if (len == 0 || len >= MAX_NAME)
        return;
    CopyMem((APTR)(line + start), word, len);
    word[len] = 0;
    CopyMem(word, dirpart, len + 1);
    *PathPart((STRPTR)dirpart) = 0;
    typed = str_len((const UBYTE *)FilePart((STRPTR)word));
    CopyMem(FilePart((STRPTR)word), filepart, typed + 1);

    /* A name without wildcards matches as a prefix; .info files only on
       Shift-Tab. */
    parsed = ParsePatternNoCase((STRPTR)filepart, (STRPTR)pattern, sizeof(pattern));
    if (parsed == 0) {
        const char *suffix = withinfo ? "#?" : "~(#?.info)";
        ULONG k = 0;
        while (suffix[k]) {
            filepart[typed + k] = suffix[k];
            k++;
        }
        filepart[typed + k] = 0;
        parsed = ParsePatternNoCase((STRPTR)filepart, (STRPTR)pattern, sizeof(pattern));
    }
    if (parsed != 1)
        return;

    NEWLIST((struct List *)&matches);
    lock = DupLock(((struct Process *)task)->pr_CurrentDir);
    olddir = CurrentDir(lock);
    if (!dirpart[0])
        scan_volumes(pattern, &matches, &count);
    scan_dir(dirpart, pattern, &matches, &count);
    CurrentDir(olddir);
    if (lock)
        UnLock(lock);

    if (count > 0) {
        const UBYTE *first = ((struct match *)matches.mlh_Head)->name;
        /* The directory as typed, separator included: PathPart() cut it. */
        ULONG common = str_len(first), dirlen = len - typed;

        for (pos = matches.mlh_Head->mln_Succ; pos->mln_Succ; pos = pos->mln_Succ) {
            const UBYTE *name = ((struct match *)pos)->name;
            ULONG k = 0;
            while (k < common && name[k] && to_lower(name[k]) == to_lower(first[k]))
                k++;
            common = k;
        }

        if (count == 1 || common > typed) {
            replace_word(pty, start, text,
                completion_text(text, word, dirlen, first, common, quoted,
                    count == 1 && is_file(first, common)));
        } else {
            /* Nothing in common to add: show them, and let Tab go through. */
            list_matches(pty, &matches);
            while ((pos = (struct MinNode *)RemHead((struct List *)&matches)))
                AddTail((struct List *)&pty->cycle, (struct Node *)pos);
            pty->cycle_count = count;
            pty->cycle_index = -1;
            pty->cycle_start = start;
            CopyMem(word, pty->cycle_dir, dirlen);
            pty->cycle_dirlen = dirlen;
            pty->cycle_quoted = quoted;
            pty->cycling = TRUE;
        }
    }

    while ((pos = (struct MinNode *)RemHead((struct List *)&matches)))
        FreeVec(pos);
}

/* --- Line history, as con-handler's ------------------------------------ */

static void history_add(struct aros_pty *pty, const UBYTE *text, ULONG len)
{
    struct buffer *last = pty->history_count ? &pty->history[pty->history_count - 1] : NULL;
    ULONG i;

    if (last && buffer_size(last) == len) {
        for (i = 0; i < len && last->data[last->start + i] == text[i]; i++)
            ;
        if (i == len)
            len = 0;    /* the same as the last one */
    }
    if (len) {
        if (pty->history_count == MAX_HISTORY) {
            buffer_free(&pty->history[0]);
            for (i = 1; i < MAX_HISTORY; i++)
                pty->history[i - 1] = pty->history[i];
            pty->history[MAX_HISTORY - 1].data = NULL;
            pty->history[MAX_HISTORY - 1].start = 0;
            pty->history[MAX_HISTORY - 1].end = 0;
            pty->history[MAX_HISTORY - 1].cap = 0;
            pty->history_count--;
        }
        buffer_append(&pty->history[pty->history_count++], text, len);
    }
    pty->history_view = pty->history_count;
}

/* Up for an older line, down for a newer one; past the newest is empty. */
static void history_walk(struct aros_pty *pty, BOOL up)
{
    struct buffer *entry;

    if (up ? pty->history_view == 0 : pty->history_view == pty->history_count)
        return;
    pty->history_view += up ? -1 : 1;
    if (pty->history_view == pty->history_count) {
        erase_line(pty);
        return;
    }
    entry = &pty->history[pty->history_view];
    replace_word(pty, 0, entry->data + entry->start, buffer_size(entry));
}

/* Editing keys in cooked mode, as xterm sends them; con-handler's set. */
static void cooked_escape(struct aros_pty *pty, const UBYTE *seq, ULONG n)
{
    ULONG size = buffer_size(&pty->line), pos = pty->line_pos;
    UBYTE final = seq[n - 1];
    BOOL shift = n == 6 && seq[2] == '1' && seq[3] == ';' && seq[4] == '2';

    if (n < 3 || (seq[1] != '[' && seq[1] != 'O'))
        return;
    if (n == 4 && final == '~') {
        if (seq[2] == '3' && pos < size)        /* Delete */
            line_delete(pty, pos, 1);
        else if (seq[2] == '1' || seq[2] == '7')  /* Home */
            line_move(pty, 0);
        else if (seq[2] == '4' || seq[2] == '8')  /* End */
            line_move(pty, size);
        return;
    }
    if (n != 3 && !shift)
        return;
    switch (final) {
    case 'Z':
        complete(pty, TRUE);    /* Shift-Tab: with .info files */
        break;
    case 'A':
    case 'B':
        if (!shift)
            history_walk(pty, final == 'A');
        break;
    case 'C':
        line_move(pty, shift ? size : pos < size ? pos + 1 : pos);
        break;
    case 'D':
        line_move(pty, shift ? 0 : pos > 0 ? pos - 1 : pos);
        break;
    case 'H':
        line_move(pty, 0);
        break;
    case 'F':
        line_move(pty, size);
        break;
    }
}

/* Runs terminal input through the line discipline. */
static void process_input(struct aros_pty *pty, const UBYTE *src, ULONG len)
{
    ULONG i = 0;

    if (pty->transparent) {
        buffer_append(&pty->ready, src, len);
        serve_reads(pty);
        return;
    }
    while (i < len) {
        UBYTE c = src[i];

        if (c == ESC) {
            /* A lone Escape is the last byte of what the terminal sent. */
            ULONG n = escape_length(src + i, len - i);
            if (n == 0)
                n = (len - i == 1) ? 1 : len - i;
            if (!(n == 3 && src[i + 1] == '[' && src[i + 2] == 'Z'))
                cycle_reset(pty);
            if (xterm_active(pty))
                buffer_append(&pty->ready, src + i, n);
            else if (pty->raw)
                raw_escape(pty, src + i, n);
            else
                cooked_escape(pty, src + i, n);
            i += n;
            continue;
        }
        i++;

        if (c != '\t')
            cycle_reset(pty);
        if (c >= 0x03 && c <= 0x06 && !xterm_active(pty))
            send_break(pty, c);

        if (pty->raw) {
            buffer_append(&pty->ready, &c, 1);
            continue;
        }

        switch (c) {
        case '\r':
        case '\n':
            history_add(pty, pty->line.data + pty->line.start, buffer_size(&pty->line));
            buffer_append(&pty->line, (const UBYTE *)"\n", 1);
            buffer_append(&pty->ready, pty->line.data + pty->line.start,
                buffer_size(&pty->line));
            line_done(pty);
            /* A new command: whatever ran before is no hint for it. */
            pty->xterm_seen = FALSE;
            echo(pty, "\n");
            break;
        case 0x08:
        case 0x7f:
            if (pty->line_pos)
                line_delete(pty, pty->line_pos - 1, 1);
            break;
        case 0x01:  /* Ctrl-A: start of line */
            line_move(pty, 0);
            break;
        case 0x1a:  /* Ctrl-Z: end of line */
            line_move(pty, buffer_size(&pty->line));
            break;
        case 0x0b:  /* Ctrl-K: delete to the end */
            line_delete(pty, pty->line_pos, buffer_size(&pty->line) - pty->line_pos);
            break;
        case 0x15:  /* Ctrl-U: delete to the start */
            line_delete(pty, 0, pty->line_pos);
            break;
        case 0x18:  /* Ctrl-X */
            erase_line(pty);
            break;
        case '\t':
            complete(pty, FALSE);
            break;
        case 0x1c:  /* Ctrl-\ is end of file */
            if (buffer_size(&pty->line)) {
                buffer_append(&pty->ready, pty->line.data + pty->line.start,
                    buffer_size(&pty->line));
                line_done(pty);
            } else {
                pty->eof_pending = TRUE;
            }
            break;
        default:
            if (c >= 0x20)
                line_insert(pty, &c, 1);
            break;
        }
    }
    serve_reads(pty);
}

/* --- Packets ----------------------------------------------------------- */

static void handle_packet(struct aros_pty *pty, struct DosPacket *dp)
{
    struct FileHandle *dosfh;
    struct FileLock *fl;

    dp->dp_Res2 = 0;
    switch (dp->dp_Type) {
    case ACTION_FH_FROM_LOCK:
        fl = BADDR(dp->dp_Arg2);
        if (fl->fl_Task != pty->port || fl->fl_Key != (IPTR)pty) {
            replypkt2(dp, DOSFALSE, ERROR_OBJECT_NOT_FOUND);
            break;
        }
        pty->usecount--;
        FreeMem(fl, sizeof(*fl));
        /* Fallthrough */
    case ACTION_FINDINPUT:
    case ACTION_FINDOUTPUT:
    case ACTION_FINDUPDATE:
        dosfh = BADDR(dp->dp_Arg1);
        dosfh->fh_Interactive = DOSTRUE;
        dosfh->fh_Arg1 = (SIPTR)pty;
        dosfh->fh_Type = pty->port;
        pty->usecount++;
        pty->breaktask = dp->dp_Port->mp_SigTask;
        replypkt(dp, DOSTRUE);
        break;
    case ACTION_COPY_DIR_FH:
        fl = AllocMem(sizeof(*fl), MEMF_CLEAR | MEMF_PUBLIC);
        if (!fl) {
            replypkt2(dp, (SIPTR)BNULL, ERROR_NO_FREE_STORE);
            break;
        }
        pty->usecount++;
        fl->fl_Task = pty->port;
        fl->fl_Access = ACCESS_READ;
        fl->fl_Key = (IPTR)pty;
        replypkt(dp, (SIPTR)MKBADDR(fl));
        break;
    case ACTION_FREE_LOCK:
        fl = BADDR(dp->dp_Arg1);
        FreeMem(fl, sizeof(*fl));
        pty->usecount--;
        replypkt(dp, DOSTRUE);
        break;
    case ACTION_END:
        pty->usecount--;
        replypkt(dp, DOSTRUE);
        break;
    case ACTION_READ:
        pty->breaktask = dp->dp_Port->mp_SigTask;
        AddTail((struct List *)&pty->reads, (struct Node *)dp->dp_Link);
        serve_reads(pty);
        break;
    case ACTION_WRITE: {
        const UBYTE *data = (const UBYTE *)dp->dp_Arg2;
        LONG k;

        pty->breaktask = dp->dp_Port->mp_SigTask;
        for (k = 0; k < dp->dp_Arg3; k++) {
            if (data[k] == '\n')
                pty->out_line.start = pty->out_line.end = 0;
            else if (buffer_size(&pty->out_line) < 512)
                buffer_append(&pty->out_line, &data[k], 1);
        }
        write_output(pty, data, dp->dp_Arg3);
        replypkt(dp, dp->dp_Arg3);
        break;
    }
    case ACTION_SCREEN_MODE:
        if (dp->dp_Arg1 && !pty->raw && buffer_size(&pty->line)) {
            /* A half-typed line becomes raw input. */
            buffer_append(&pty->ready, pty->line.data + pty->line.start,
                buffer_size(&pty->line));
            line_done(pty);
        }
        pty->raw = dp->dp_Arg1 != 0;
        if (!pty->raw)
            pty->xterm_seen = FALSE;
        ObtainSemaphore(&pty->lock);
        pty->transparent = dp->dp_Arg1 == MODE_TRANSPARENT;
        ReleaseSemaphore(&pty->lock);
        D(bug("[aros-pty] %s mode\n",
            pty->transparent ? "transparent" : pty->raw ? "raw" : "cooked"));
        replypkt(dp, DOSTRUE);
        serve_reads(pty);
        break;
    case ACTION_CHANGE_SIGNAL: {
        struct Task *old = pty->breaktask;
        if (dp->dp_Arg2)
            pty->breaktask = (struct Task *)dp->dp_Arg2;
        replypkt2(dp, DOSTRUE, (SIPTR)old);
        break;
    }
    case ACTION_WAIT_CHAR:
        if (buffer_size(&pty->ready))
            replypkt(dp, DOSTRUE);
        else if (dp->dp_Arg1 == 0 || pty->closing || pty->waiting || !pty->timer)
            replypkt(dp, DOSFALSE);
        else {
            pty->timer->tr_node.io_Command = TR_ADDREQUEST;
            pty->timer->tr_time.tv_secs = dp->dp_Arg1 / 1000000;
            pty->timer->tr_time.tv_micro = dp->dp_Arg1 % 1000000;
            SendIO((struct IORequest *)pty->timer);
            pty->waiting = dp;
        }
        break;
    case ACTION_IS_FILESYSTEM:
        replypkt(dp, DOSFALSE);
        break;
    case ACTION_DISK_INFO: {
        struct InfoData *id = BADDR(dp->dp_Arg1);
        ULONG i;
        for (i = 0; i < sizeof(*id); i++)
            ((UBYTE *)id)[i] = 0;
        id->id_DiskType = pty->transparent ? AROS_MAKE_ID('X', 'T', 'R', 'M')
                        : pty->raw         ? AROS_MAKE_ID('R', 'A', 'W', 0)
                                           : AROS_MAKE_ID('C', 'O', 'N', 0);
        /* There is no window to hand out. */
        replypkt(dp, DOSTRUE);
        break;
    }
    case ACTION_DIE:
        if (pty->usecount > 0)
            replypkt2(dp, DOSFALSE, ERROR_OBJECT_IN_USE);
        else
            replypkt(dp, DOSTRUE);
        break;
    case ACTION_SEEK:
        /* DOSTRUE, as con-handler and the Guru Book have it. */
        replypkt2(dp, DOSTRUE, ERROR_ACTION_NOT_KNOWN);
        break;
    default:
        D(bug("[aros-pty] unknown action %d\n", (int)dp->dp_Type));
        replypkt2(dp, DOSFALSE, ERROR_ACTION_NOT_KNOWN);
        break;
    }
}

/* --- The handler process ----------------------------------------------- */

static void open_timer(struct aros_pty *pty)
{
    pty->timer_port = CreateMsgPort();
    if (!pty->timer_port)
        return;
    pty->timer = (struct timerequest *)CreateIORequest(pty->timer_port, sizeof(*pty->timer));
    if (pty->timer && OpenDevice((CONST_STRPTR)TIMERNAME, UNIT_MICROHZ,
            (struct IORequest *)pty->timer, 0) == 0)
        return;
    if (pty->timer)
        DeleteIORequest((struct IORequest *)pty->timer);
    DeleteMsgPort(pty->timer_port);
    pty->timer = NULL;
    pty->timer_port = NULL;
}

static void close_timer(struct aros_pty *pty)
{
    if (!pty->timer)
        return;
    CloseDevice((struct IORequest *)pty->timer);
    DeleteIORequest((struct IORequest *)pty->timer);
    DeleteMsgPort(pty->timer_port);
}

static void handler_run(struct aros_pty *pty)
{
    BYTE input_bit = AllocSignal(-1);
    ULONG port_mask, timer_mask, sigs;
    BOOL closing_seen = FALSE;
    struct Task *closer;
    ULONG closer_mask, i;

    pty->port = CreateMsgPort();
    if (input_bit >= 0)
        pty->input_mask = 1UL << input_bit;
    open_timer(pty);
    NEWLIST((struct List *)&pty->reads);
    NEWLIST((struct List *)&pty->cycle);

    /* Report in; the starter checks pty->port. */
    if (!pty->port || input_bit < 0) {
        if (pty->port)
            DeleteMsgPort(pty->port);
        pty->port = NULL;
        close_timer(pty);
        Forbid();
        Signal(pty->starter, pty->starter_mask);
        return;
    }
    Signal(pty->starter, pty->starter_mask);

    port_mask = 1UL << pty->port->mp_SigBit;
    timer_mask = pty->timer_port ? 1UL << pty->timer_port->mp_SigBit : 0;

    /* The starter's file handle counts as the first user. */
    while (pty->usecount > 0) {
        struct Message *mn;
        UBYTE chunk[256];
        ULONG n;
        BOOL closing;

        sigs = Wait(port_mask | timer_mask | pty->input_mask);

        if ((sigs & timer_mask) && GetMsg(pty->timer_port) && pty->waiting) {
            replypkt(pty->waiting, DOSFALSE);
            pty->waiting = NULL;
        }

        do {
            ObtainSemaphore(&pty->lock);
            n = buffer_take(&pty->input, chunk, sizeof(chunk));
            closing = pty->closing;
            ReleaseSemaphore(&pty->lock);
            if (n)
                process_input(pty, chunk, n);
        } while (n == sizeof(chunk));

        if (closing && !closing_seen) {
            /* EOF to every reader, and a break so the shell stops waiting. */
            closing_seen = TRUE;
            D(bug("[aros-pty] closing, %d users\n", (int)pty->usecount));
            if (pty->breaktask)
                Signal(pty->breaktask, SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D);
            serve_reads(pty);
        }

        while ((mn = GetMsg(pty->port)))
            handle_packet(pty, (struct DosPacket *)mn->mn_Node.ln_Name);
    }

    /* Nothing can reach the port any more: every handle is closed. */
    serve_reads(pty);
    if (pty->waiting) {
        AbortIO((struct IORequest *)pty->timer);
        WaitIO((struct IORequest *)pty->timer);
        replypkt(pty->waiting, DOSFALSE);
        pty->waiting = NULL;
    }
    close_timer(pty);
    DeleteMsgPort(pty->port);
    FreeSignal(input_bit);
    buffer_free(&pty->line);
    buffer_free(&pty->ready);
    buffer_free(&pty->out_line);
    for (i = 0; i < pty->history_count; i++)
        buffer_free(&pty->history[i]);
    cycle_reset(pty);

    ObtainSemaphore(&pty->lock);
    pty->gone = TRUE;
    closer = pty->closer;
    closer_mask = pty->closer_mask;
    if (pty->reader)
        Signal(pty->reader, pty->reader_mask);
    ReleaseSemaphore(&pty->lock);

    /* The closer may free the console as soon as it runs. */
    Forbid();
    if (closer)
        Signal(closer, closer_mask);
}

static AROS_PROCH(handler_entry, argstr, argsize, SysBase)
{
    AROS_PROCFUNC_INIT

    (void)argstr;
    (void)argsize;
    handler_run((struct aros_pty *)FindTask(NULL)->tc_UserData);
    return 0;

    AROS_PROCFUNC_EXIT
}

/* --- The terminal side ------------------------------------------------- */

struct aros_pty *aros_pty_spawn(const char *command, int32_t cols, int32_t rows)
{
    struct aros_pty *pty = AllocVec(sizeof(*pty), MEMF_ANY | MEMF_CLEAR);
    struct FileHandle *fh = AllocDosObject(DOS_FILEHANDLE, NULL);
    BPTR script = BNULL;
    BYTE bit;
    LONG rc;

    if (!pty || !fh) {
        if (fh)
            FreeDosObject(DOS_FILEHANDLE, fh);
        FreeVec(pty);
        return NULL;
    }
    InitSemaphore(&pty->lock);
    pty->cols = cols;
    pty->rows = rows;
    pty->usecount = 1;

    bit = AllocSignal(-1);
    if (bit < 0) {
        FreeDosObject(DOS_FILEHANDLE, fh);
        FreeVec(pty);
        return NULL;
    }
    pty->starter = FindTask(NULL);
    pty->starter_mask = 1UL << bit;
    SetSignal(0, pty->starter_mask);

    pty->handler = CreateNewProcTags(
        NP_Entry,    (IPTR)handler_entry,
        NP_Name,     (IPTR)"Alacritty console",
        NP_UserData, (IPTR)pty,
        TAG_DONE);
    if (pty->handler)
        Wait(pty->starter_mask);
    FreeSignal(bit);

    if (!pty->handler || !pty->port) {
        FreeDosObject(DOS_FILEHANDLE, fh);
        FreeVec(pty);
        return NULL;
    }

    /* The shell's input. Its output and error come from opening "*", which
       newcliproc does on the console task this handle sets. */
    fh->fh_Type = pty->port;
    fh->fh_Arg1 = (SIPTR)pty;
    fh->fh_Interactive = DOSTRUE;
    /* Without a command this is a shell for the user, started as NewShell
       does: not in the background, so it is interactive and prompts, and
       S:Shell-Startup sets it up. A command runs as a System() call, which
       ends with the command. */
    if (!(command && *command))
        script = Open((CONST_STRPTR)"S:Shell-Startup", MODE_OLDFILE);
    rc = SystemTags((CONST_STRPTR)(command ? command : ""),
        SYS_Input,      (IPTR)MKBADDR(fh),
        SYS_Output,     BNULL,
        SYS_Error,      BNULL,
        SYS_Asynch,     TRUE,
        SYS_Background, (command && *command) ? TRUE : FALSE,
        SYS_ScriptInput, (IPTR)script,
        SYS_UserShell,  TRUE,
        NP_Name,        (IPTR)"Alacritty Shell",
        TAG_DONE);
    if (rc == -1) {
        /* The shell owns the script only once it runs. */
        if (script)
            Close(script);
        /* Ends the handler: this was its only user. */
        Close(MKBADDR(fh));
        aros_pty_close(pty);
        aros_pty_free(pty);
        return NULL;
    }

    return pty;
}

int32_t aros_pty_read(struct aros_pty *pty, uint8_t *buf, int32_t len)
{
    BYTE bit = AllocSignal(-1);
    ULONG mask;
    int32_t n = 0;

    if (bit < 0)
        return 0;
    mask = 1UL << bit;

    for (;;) {
        /* Cleared before the check, so a signal sent after it is kept. */
        SetSignal(0, mask);
        ObtainSemaphore(&pty->lock);
        n = buffer_take(&pty->output, buf, len);
        if (n || pty->gone) {
            pty->reader = NULL;
            ReleaseSemaphore(&pty->lock);
            break;
        }
        pty->reader = FindTask(NULL);
        pty->reader_mask = mask;
        ReleaseSemaphore(&pty->lock);
        Wait(mask);
    }

    FreeSignal(bit);
    return n;
}

void aros_pty_write(struct aros_pty *pty, const uint8_t *buf, int32_t len)
{
    UBYTE latin1[256];
    int32_t i = 0;
    BOOL transparent;

    ObtainSemaphore(&pty->lock);
    transparent = pty->transparent;
    if (transparent && !pty->gone)
        buffer_append(&pty->input, buf, len);
    ReleaseSemaphore(&pty->lock);

    while (!transparent && i < len) {
        ULONG n = 0;

        /* UTF-8 to ISO-8859-1; anything outside it becomes '?'. */
        while (i < len && n < sizeof(latin1)) {
            uint8_t c = buf[i];
            ULONG cp, extra;

            if (c < 0x80) {
                cp = c;
                extra = 0;
            } else if ((c & 0xe0) == 0xc0) {
                cp = c & 0x1f;
                extra = 1;
            } else if ((c & 0xf0) == 0xe0) {
                cp = c & 0x0f;
                extra = 2;
            } else {
                cp = c & 0x07;
                extra = 3;
            }
            i++;
            while (extra-- && i < len && (buf[i] & 0xc0) == 0x80)
                cp = (cp << 6) | (buf[i++] & 0x3f);
            latin1[n++] = cp < 0x100 ? (UBYTE)cp : '?';
        }

        ObtainSemaphore(&pty->lock);
        if (!pty->gone)
            buffer_append(&pty->input, latin1, n);
        ReleaseSemaphore(&pty->lock);
    }

    ObtainSemaphore(&pty->lock);
    if (!pty->gone)
        Signal(&pty->handler->pr_Task, pty->input_mask);
    ReleaseSemaphore(&pty->lock);
}

void aros_pty_resize(struct aros_pty *pty, int32_t cols, int32_t rows)
{
    ObtainSemaphore(&pty->lock);
    pty->cols = cols;
    pty->rows = rows;
    ReleaseSemaphore(&pty->lock);
}

void aros_pty_close(struct aros_pty *pty)
{
    BYTE bit = AllocSignal(-1);

    ObtainSemaphore(&pty->lock);
    if (pty->gone) {
        ReleaseSemaphore(&pty->lock);
        if (bit >= 0)
            FreeSignal(bit);
        return;
    }
    if (bit < 0) {
        /* No signal to wait on: poll until the handler is gone. */
        BOOL gone;

        pty->closing = TRUE;
        Signal(&pty->handler->pr_Task, pty->input_mask);
        ReleaseSemaphore(&pty->lock);
        do {
            Delay(2);
            ObtainSemaphore(&pty->lock);
            gone = pty->gone;
            ReleaseSemaphore(&pty->lock);
        } while (!gone);
        return;
    }
    pty->closer = FindTask(NULL);
    pty->closer_mask = 1UL << bit;
    SetSignal(0, pty->closer_mask);
    pty->closing = TRUE;
    Signal(&pty->handler->pr_Task, pty->input_mask);
    ReleaseSemaphore(&pty->lock);

    Wait(1UL << bit);
    FreeSignal(bit);
}

void aros_pty_free(struct aros_pty *pty)
{
    buffer_free(&pty->output);
    buffer_free(&pty->input);
    FreeVec(pty);
}
