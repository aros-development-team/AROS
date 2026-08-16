/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dostags.h>
#include <dos/var.h>

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/err.h>

/* the C library formatters do the per-conversion work here */
#undef snprintf
#undef vsnprintf
#undef sprintf

/*
 * The kernel format language extends printf with a few %p forms that the
 * C library cannot know about (%pV nested formats, %pe errno pointers,
 * %ps symbols, %*ph hex dumps). Each conversion is therefore parsed and
 * emitted on its own, so that those forms can be handled here and
 * everything else handed to snprintf one argument at a time.
 */

struct outbuf {
    char *buf;
    size_t size;
    size_t len;
};

static void out_char(struct outbuf *o, char c)
{
    if (o->buf && o->len + 1 < o->size)
        o->buf[o->len] = c;
    o->len++;
}

static void out_str(struct outbuf *o, const char *s, size_t n)
{
    while (n--)
        out_char(o, *s++);
}

static void out_hex(struct outbuf *o, const u8 *data, int len, char sep)
{
    static const char hex[] = "0123456789abcdef";
    int i;
    for (i = 0; i < len; i++) {
        if (i && sep)
            out_char(o, sep);
        out_char(o, hex[data[i] >> 4]);
        out_char(o, hex[data[i] & 15]);
    }
}

static int format_into(struct outbuf *o, const char *fmt, va_list ap);

static void format_pointer(struct outbuf *o, const char **pfmt, va_list *ap)
{
    const char *fmt = *pfmt;
    char tmp[64];
    int n;

    switch (*fmt) {
    case 'V': {
        struct va_format *vaf = va_arg(*ap, struct va_format *);
        va_list aq;
        fmt++;
        if (vaf && vaf->fmt) {
            va_copy(aq, *vaf->va);
            format_into(o, vaf->fmt, aq);
            va_end(aq);
        }
        break;
    }
    case 'e': {
        const void *p = va_arg(*ap, const void *);
        fmt++;
        if (IS_ERR(p))
            n = snprintf(tmp, sizeof(tmp), "%ld", PTR_ERR(p));
        else
            n = snprintf(tmp, sizeof(tmp), "%p", p);
        out_str(o, tmp, n);
        break;
    }
    case 's': case 'S': case 'f': case 'F': case 'B': {
        const void *p = va_arg(*ap, const void *);
        fmt++;
        if (*fmt == 'R') fmt++;
        n = snprintf(tmp, sizeof(tmp), "%p", p);
        out_str(o, tmp, n);
        break;
    }
    case 'M': case 'm': case 'I': case 'i': case 'U': case 'O': case 'C': case 'd': case 'D': case 'g': case 'a': case 'E': case 'K': case 'x': case 'k': case 'r': case 'R': case 'b': case 'G': case 't': {
        /* other kernel extensions: print the raw pointer, skip the qualifier letters */
        const void *p = va_arg(*ap, const void *);
        while (*fmt >= 'a' && *fmt <= 'z')
            fmt++;
        while (*fmt >= 'A' && *fmt <= 'Z')
            fmt++;
        n = snprintf(tmp, sizeof(tmp), "%p", p);
        out_str(o, tmp, n);
        break;
    }
    default: {
        const void *p = va_arg(*ap, const void *);
        n = snprintf(tmp, sizeof(tmp), "%p", p);
        out_str(o, tmp, n);
        break;
    }
    }
    *pfmt = fmt;
}

static int format_into(struct outbuf *o, const char *fmt, va_list ap)
{
    va_list ap2;
    char spec[32];
    char tmp[512];

    va_copy(ap2, ap);

    while (*fmt) {
        const char *start;
        size_t sl;
        int have_star_width = 0, have_star_prec = 0;
        int star_width = 0, star_prec = 0;
        int lenmod = 0;      /* 0 int, 1 long, 2 long long, 3 size_t, 4 short, 5 char, 6 ptrdiff */
        char conv;
        int n;

        if (*fmt != '%') {
            out_char(o, *fmt++);
            continue;
        }
        start = fmt++;
        if (*fmt == '%') {
            out_char(o, '%');
            fmt++;
            continue;
        }
        /* flags */
        while (*fmt == '-' || *fmt == '+' || *fmt == ' ' || *fmt == '#' || *fmt == '0')
            fmt++;
        /* width */
        if (*fmt == '*') {
            have_star_width = 1;
            star_width = va_arg(ap2, int);
            fmt++;
        } else {
            while (*fmt >= '0' && *fmt <= '9')
                fmt++;
        }
        /* precision */
        if (*fmt == '.') {
            fmt++;
            if (*fmt == '*') {
                have_star_prec = 1;
                star_prec = va_arg(ap2, int);
                fmt++;
            } else {
                while (*fmt >= '0' && *fmt <= '9')
                    fmt++;
            }
        }
        /* length modifiers */
        for (;;) {
            if (*fmt == 'l') { lenmod = (lenmod == 1) ? 2 : 1; fmt++; continue; }
            if (*fmt == 'h') { lenmod = (lenmod == 4) ? 5 : 4; fmt++; continue; }
            if (*fmt == 'z' || *fmt == 'Z') { lenmod = 3; fmt++; continue; }
            if (*fmt == 't') { lenmod = 6; fmt++; continue; }
            if (*fmt == 'L' || *fmt == 'q' || *fmt == 'j') { lenmod = 2; fmt++; continue; }
            break;
        }
        conv = *fmt;
        if (!conv)
            break;
        fmt++;

        sl = fmt - start;
        if (sl >= sizeof(spec))
            sl = sizeof(spec) - 1;
        memcpy(spec, start, sl);
        spec[sl] = 0;

        switch (conv) {
        case 'p':
            if (*fmt == 'h' && have_star_width) {
                /* %*ph: hex dump of star_width bytes */
                const u8 *data = va_arg(ap2, const u8 *);
                char sep = ' ';
                fmt++;
                if (*fmt == 'C') { sep = ':'; fmt++; }
                else if (*fmt == 'D') { sep = '-'; fmt++; }
                else if (*fmt == 'N') { sep = 0; fmt++; }
                if (star_width > 64) star_width = 64;
                out_hex(o, data, star_width, sep);
            } else if (*fmt == 'V' || *fmt == 'e' || (*fmt >= 'a' && *fmt <= 'z') || (*fmt >= 'A' && *fmt <= 'Z')) {
                format_pointer(o, &fmt, &ap2);
            } else {
                const void *p = va_arg(ap2, const void *);
                n = snprintf(tmp, sizeof(tmp), "%p", p);
                out_str(o, tmp, n);
            }
            break;
        case 'd': case 'i': case 'u': case 'x': case 'X': case 'o': case 'c': {
            /* rebuild the spec with explicit width/precision */
            char sp2[48];
            char *w = sp2;
            const char *q = start + 1;
            *w++ = '%';
            while (*q == '-' || *q == '+' || *q == ' ' || *q == '#' || *q == '0')
                *w++ = *q++;
            if (have_star_width) w += sprintf(w, "%d", star_width);
            else while (*q >= '0' && *q <= '9') *w++ = *q++;
            if (*q == '.') {
                q++;
                *w++ = '.';
                if (*q == '*') { w += sprintf(w, "%d", star_prec); q++; }
                else while (*q >= '0' && *q <= '9') *w++ = *q++;
            }
            switch (lenmod) {
            case 1: *w++ = 'l'; break;
            case 2: *w++ = 'l'; *w++ = 'l'; break;
            case 3: *w++ = 'z'; break;
            case 4: *w++ = 'h'; break;
            case 5: *w++ = 'h'; *w++ = 'h'; break;
            case 6: *w++ = 't'; break;
            }
            *w++ = conv;
            *w = 0;
            switch (lenmod) {
            case 1: n = snprintf(tmp, sizeof(tmp), sp2, va_arg(ap2, long)); break;
            case 2: n = snprintf(tmp, sizeof(tmp), sp2, va_arg(ap2, long long)); break;
            case 3: n = snprintf(tmp, sizeof(tmp), sp2, va_arg(ap2, size_t)); break;
            case 6: n = snprintf(tmp, sizeof(tmp), sp2, va_arg(ap2, ptrdiff_t)); break;
            default: n = snprintf(tmp, sizeof(tmp), sp2, va_arg(ap2, int)); break;
            }
            out_str(o, tmp, n < (int)sizeof(tmp) ? n : (int)sizeof(tmp) - 1);
            break;
        }
        case 's': {
            const char *s = va_arg(ap2, const char *);
            char sp2[48];
            char *w = sp2;
            const char *q = start + 1;
            if (!s)
                s = "(null)";
            *w++ = '%';
            while (*q == '-' || *q == '+' || *q == ' ' || *q == '#' || *q == '0')
                *w++ = *q++;
            if (have_star_width) w += sprintf(w, "%d", star_width);
            else while (*q >= '0' && *q <= '9') *w++ = *q++;
            if (*q == '.') {
                q++;
                *w++ = '.';
                if (*q == '*') { w += sprintf(w, "%d", star_prec); q++; }
                else while (*q >= '0' && *q <= '9') *w++ = *q++;
            }
            *w++ = 's';
            *w = 0;
            n = snprintf(tmp, sizeof(tmp), sp2, s);
            if (n >= (int)sizeof(tmp))
                n = sizeof(tmp) - 1;
            out_str(o, tmp, n);
            break;
        }
        case 'f': case 'g': case 'e': case 'E': case 'G': case 'F': {
            double d = va_arg(ap2, double);
            n = snprintf(tmp, sizeof(tmp), spec, d);
            out_str(o, tmp, n);
            break;
        }
        case 'n':
            (void)va_arg(ap2, int *);
            break;
        default:
            out_str(o, spec, sl);
            break;
        }
    }

    va_end(ap2);
    if (o->buf && o->size)
        o->buf[o->len < o->size ? o->len : o->size - 1] = 0;
    return (int)o->len;
}

int vsnprintk(char *buf, size_t size, const char *fmt, va_list ap)
{
    struct outbuf o = { buf, size, 0 };
    return format_into(&o, fmt, ap);
}

int snprintk(char *buf, size_t size, const char *fmt, ...)
{
    va_list ap;
    int n;
    va_start(ap, fmt);
    n = vsnprintk(buf, size, fmt, ap);
    va_end(ap);
    return n;
}

int vscnprintk(char *buf, size_t size, const char *fmt, va_list ap)
{
    int n = vsnprintk(buf, size, fmt, ap);
    if (n >= (int)size)
        return size ? (int)size - 1 : 0;
    return n;
}

int scnprintk(char *buf, size_t size, const char *fmt, ...)
{
    va_list ap;
    int n;
    va_start(ap, fmt);
    n = vscnprintk(buf, size, fmt, ap);
    va_end(ap);
    return n;
}

/*
 * Where a line goes. Everything is appended to a ring buffer that a
 * helper process writes to a log file (NOUVEAU_LOGFILE, default
 * SYS:nouveau.log, "off" to disable). The serial console only gets
 * lines up to KERN_NOTICE unless NOUVEAU_SERIAL is set to "all"; the
 * driver's debug chatter would otherwise overrun a serial capture.
 */
#define NLOG_SIZE   (2 * 1024 * 1024)

static struct {
    char *buf;
    volatile ULONG head, tail;      /* head = written by producers, tail = flushed */
    volatile ULONG lost;
    struct Task *flusher;
    ULONG sigbit;
    BOOL serial_all;
    BOOL started;
} nlog;

static void nlog_append(const char *s, size_t n)
{
    if (!nlog.buf)
        return;

    Disable();
    if (n >= NLOG_SIZE)
    {
        nlog.lost += n;
    }
    else
    {
        ULONG used = nlog.head - nlog.tail;

        if (used + n > NLOG_SIZE)
        {
            nlog.lost += n;
        }
        else
        {
            ULONG pos = nlog.head % NLOG_SIZE;
            ULONG first = NLOG_SIZE - pos;

            if (first > n)
                first = n;
            memcpy(nlog.buf + pos, s, first);
            if (n > first)
                memcpy(nlog.buf, s + first, n - first);
            nlog.head += n;
        }
    }
    Enable();

    if (nlog.flusher && nlog.sigbit != (ULONG)-1)
        Signal(nlog.flusher, 1UL << nlog.sigbit);
}

static void nlog_flusher(void)
{
    struct Task *self = FindTask(NULL);
    char name[128];
    char var[16];
    BOOL created = FALSE;
    ULONG idle = 0;

    nlog.sigbit = AllocSignal(-1);
    if (nlog.sigbit == (ULONG)-1)
        return;
    nlog.flusher = self;

    if (GetVar("NOUVEAU_LOGFILE", name, sizeof(name), GVF_GLOBAL_ONLY) <= 0)
        strcpy(name, "SYS:nouveau.log");
    if (GetVar("NOUVEAU_SERIAL", var, sizeof(var), GVF_GLOBAL_ONLY) > 0 && var[0] == 'a')
        nlog.serial_all = TRUE;

    if (!strcmp(name, "off"))
        return;

    /*
     * The file is opened, appended to and closed for every burst: a
     * machine that is reset or powered off mid-session must still have
     * the log on disk, and only a close makes the handler commit it.
     */
    for (;;)
    {
        BPTR fh;
        ULONG head, tail;

        Wait((1UL << nlog.sigbit) | SIGBREAKF_CTRL_C);
        /* let a burst of lines accumulate */
        Delay(2);

        if (nlog.head == nlog.tail)
            continue;

        fh = Open(name, created ? MODE_READWRITE : MODE_NEWFILE);
        if (!fh)
        {
            if (++idle == 1 || (idle % 50) == 0)
                bug("[nouveau] cannot open %s (%ld), retrying\n", name, (long)IoErr());
            Delay(25);
            Signal(self, 1UL << nlog.sigbit);
            continue;
        }
        if (!created)
        {
            char full[160];

            if (!NameFromFH(fh, full, sizeof(full)))
                strcpy(full, "?");
            bug("[nouveau] logging to %s (%s)\n", name, full);
            created = TRUE;
        }
        else
            Seek(fh, 0, OFFSET_END);

        do
        {
            head = nlog.head;
            tail = nlog.tail;
            if (head != tail)
            {
                ULONG pos = tail % NLOG_SIZE;
                ULONG n = head - tail;
                ULONG first = NLOG_SIZE - pos;

                if (first > n)
                    first = n;
                if (Write(fh, nlog.buf + pos, first) != (LONG)first ||
                    (n > first && Write(fh, nlog.buf, n - first) != (LONG)(n - first)))
                {
                    if (++idle < 4)
                        bug("[nouveau] log write failed (%ld)\n", (long)IoErr());
                }
                nlog.tail = head;
                if (nlog.lost)
                {
                    char msg[64];
                    int l = snprintf(msg, sizeof(msg), "[nouveau] log buffer overflow, %lu bytes lost\n", (unsigned long)nlog.lost);
                    nlog.lost = 0;
                    Write(fh, msg, l);
                }
            }
        } while (nlog.head != nlog.tail);
        Close(fh);
        /* one burst a second at most: the handler flushes to disk between them */
        Delay(50);
    }
}

BOOL nouveau_log_init(void)
{
    if (nlog.started)
        return TRUE;
    nlog.started = TRUE;
    nlog.sigbit = (ULONG)-1;
    nlog.buf = AllocMem(NLOG_SIZE, MEMF_ANY);
    if (!nlog.buf)
        return FALSE;
    CreateNewProcTags(NP_Name, (IPTR)"Nouveau Log", NP_Priority, -1,
                      NP_Entry, (IPTR)nlog_flusher, NP_StackSize, 32 * 1024, TAG_DONE);
    return TRUE;
}

/* Every formatted line ends up here: strip the level marker, then route */
static void nlog_emit(char *buf, size_t n)
{
    int level = 6;      /* plain printk: treat as informational */

    if (n >= 2 && buf[0] == '\001')
    {
        if (buf[1] >= '0' && buf[1] <= '7')
            level = buf[1] - '0';
        buf += 2;
        n -= 2;
    }
    nlog_append(buf, n);
    if (level <= 5 || nlog.serial_all || !nlog.buf)
        bug("%s", buf);
}

/* the hidd's own messages: serial and log file alike */
void nouveau_compat_log(const char *fmt, ...)
{
    char buf[512];
    va_list ap;
    int n;

    va_start(ap, fmt);
    n = vsnprintk(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n >= (int)sizeof(buf))
        n = sizeof(buf) - 1;
    nlog_append(buf, n);
    bug("%s", buf);
}

int vprintk(const char *fmt, va_list ap)
{
    char buf[1024];
    int n = vsnprintk(buf, sizeof(buf), fmt, ap);
    if (n >= (int)sizeof(buf))
        n = sizeof(buf) - 1;
    nlog_emit(buf, n);
    return n;
}

int printk(const char *fmt, ...)
{
    va_list ap;
    int n;
    va_start(ap, fmt);
    n = vprintk(fmt, ap);
    va_end(ap);
    return n;
}

void print_hex_dump(const char *level, const char *prefix_str, int prefix_type,
    int rowsize, int groupsize, const void *buf, size_t len, bool ascii)
{
    const unsigned char *b = buf;
    char line[160];
    size_t off;

    if (rowsize != 16 && rowsize != 32)
        rowsize = 16;
    for (off = 0; off < len; off += rowsize) {
        size_t n = min_t(size_t, rowsize, len - off), i, pos = 0;

        for (i = 0; i < n && pos < sizeof(line) - 4; i++)
            pos += snprintf(line + pos, sizeof(line) - pos, "%02x ", b[off + i]);
        if (ascii) {
            for (; i < (size_t)rowsize && pos < sizeof(line) - 4; i++)
                pos += snprintf(line + pos, sizeof(line) - pos, "   ");
            pos += snprintf(line + pos, sizeof(line) - pos, " ");
            for (i = 0; i < n && pos < sizeof(line) - 2; i++)
                line[pos++] = (b[off + i] >= 0x20 && b[off + i] < 0x7f) ? b[off + i] : '.';
            line[pos] = 0;
        }
        {
            char out[256];
            int l;

            switch (prefix_type) {
            case DUMP_PREFIX_ADDRESS:
                l = snprintf(out, sizeof(out), "%s%s%p: %s\n", level ? level : "", prefix_str, b + off, line);
                break;
            case DUMP_PREFIX_OFFSET:
                l = snprintf(out, sizeof(out), "%s%s%08lx: %s\n", level ? level : "", prefix_str, (unsigned long)off, line);
                break;
            default:
                l = snprintf(out, sizeof(out), "%s%s%s\n", level ? level : "", prefix_str, line);
                break;
            }
            if (l >= (int)sizeof(out))
                l = sizeof(out) - 1;
            nlog_emit(out, l);
        }
    }
}
