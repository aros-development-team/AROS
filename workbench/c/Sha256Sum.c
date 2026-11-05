/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Sha256Sum CLI command.
*/

/******************************************************************************


    NAME

        Sha256Sum

    SYNOPSIS

        FILE/M,TEXT/K,QUIET/S

    LOCATION

        C:

    FUNCTION

        Prints the SHA-256 digest (FIPS 180-4) of each file, of standard input
        when no file is given, or of the text given with TEXT. A line is the
        digest in lowercase hex, two spaces and the name, as GNU sha256sum
        prints it ("-" for standard input), so the output can be compared
        with that of other systems.

    INPUTS

        FILE  -- The files to read. "-" is standard input.
        TEXT  -- A string to take as the data instead: its characters as they
                 are, with no newline added (for example a password, see
                 below). The name printed is "-".
        QUIET -- Print only the digest, with no name.

    RESULT

        RETURN_OK, or RETURN_ERROR when a file could not be read (its line is
        left out and the others are printed).

    NOTES

        TEXT puts the string on the command line, where the shell may keep
        it in its history.

    EXAMPLE

        Sha256Sum SYS:C/List
            b0e1...4f2a  SYS:C/List

        Sha256Sum TEXT "mypassword" QUIET >ENVARC:SSH/password
            sets the password of sshd, which keeps the SHA-256 digest of the
            password in that file.

    BUGS

    SEE ALSO

    INTERNALS

        The digest is computed here, so the command needs nothing but
        dos.library.

    HISTORY

******************************************************************************/

#include <exec/types.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <string.h>

const TEXT version[] = "$VER: Sha256Sum 1.0 (5.11.2026)";

int __nocommandline;

#define BUFSIZE 16384

/* ------------------------------------------------------------------ SHA-256 */

struct Sha256 {
    ULONG h[8];
    UBYTE block[64];
    ULONG used;                         /* bytes in block */
    UQUAD length;                       /* bytes hashed so far */
};

static const ULONG k[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

#define ROR(x, n)   ((((x) >> (n)) | ((x) << (32 - (n)))) & 0xffffffffUL)

static void sha256_init(struct Sha256 *s)
{
    static const ULONG h0[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };

    memcpy(s->h, h0, sizeof(s->h));
    s->used = 0;
    s->length = 0;
}

static void sha256_block(struct Sha256 *s, const UBYTE *p)
{
    ULONG w[64], a, b, c, d, e, f, g, h, t1, t2;
    int i;

    for (i = 0; i < 16; i++)
        w[i] = ((ULONG)p[4 * i] << 24) | ((ULONG)p[4 * i + 1] << 16) |
               ((ULONG)p[4 * i + 2] << 8) | p[4 * i + 3];
    for (i = 16; i < 64; i++)
    {
        ULONG s0 = ROR(w[i - 15], 7) ^ ROR(w[i - 15], 18) ^ (w[i - 15] >> 3);
        ULONG s1 = ROR(w[i - 2], 17) ^ ROR(w[i - 2], 19) ^ (w[i - 2] >> 10);

        w[i] = (w[i - 16] + s0 + w[i - 7] + s1) & 0xffffffffUL;
    }

    a = s->h[0]; b = s->h[1]; c = s->h[2]; d = s->h[3];
    e = s->h[4]; f = s->h[5]; g = s->h[6]; h = s->h[7];
    for (i = 0; i < 64; i++)
    {
        t1 = (h + (ROR(e, 6) ^ ROR(e, 11) ^ ROR(e, 25)) + ((e & f) ^ (~e & g)) + k[i] + w[i])
             & 0xffffffffUL;
        t2 = ((ROR(a, 2) ^ ROR(a, 13) ^ ROR(a, 22)) + ((a & b) ^ (a & c) ^ (b & c)))
             & 0xffffffffUL;
        h = g; g = f; f = e;
        e = (d + t1) & 0xffffffffUL;
        d = c; c = b; b = a;
        a = (t1 + t2) & 0xffffffffUL;
    }
    s->h[0] = (s->h[0] + a) & 0xffffffffUL; s->h[1] = (s->h[1] + b) & 0xffffffffUL;
    s->h[2] = (s->h[2] + c) & 0xffffffffUL; s->h[3] = (s->h[3] + d) & 0xffffffffUL;
    s->h[4] = (s->h[4] + e) & 0xffffffffUL; s->h[5] = (s->h[5] + f) & 0xffffffffUL;
    s->h[6] = (s->h[6] + g) & 0xffffffffUL; s->h[7] = (s->h[7] + h) & 0xffffffffUL;
}

static void sha256_update(struct Sha256 *s, const UBYTE *p, ULONG n)
{
    s->length += n;
    while (n > 0)
    {
        ULONG take = 64 - s->used;

        if (take > n)
            take = n;
        memcpy(s->block + s->used, p, take);
        s->used += take;
        p += take;
        n -= take;
        if (s->used == 64)
        {
            sha256_block(s, s->block);
            s->used = 0;
        }
    }
}

/* The digest, as 64 lowercase hex digits and a terminator. */
static void sha256_final(struct Sha256 *s, char *hex)
{
    static const char digits[] = "0123456789abcdef";
    UQUAD bits = s->length * 8;
    int i;

    s->block[s->used++] = 0x80;
    if (s->used > 56)
    {
        memset(s->block + s->used, 0, 64 - s->used);
        sha256_block(s, s->block);
        s->used = 0;
    }
    memset(s->block + s->used, 0, 56 - s->used);
    for (i = 0; i < 8; i++)
        s->block[56 + i] = (UBYTE)(bits >> (56 - 8 * i));
    sha256_block(s, s->block);

    for (i = 0; i < 32; i++)
    {
        UBYTE byte = (UBYTE)(s->h[i / 4] >> (24 - 8 * (i % 4)));

        hex[2 * i] = digits[byte >> 4];
        hex[2 * i + 1] = digits[byte & 15];
    }
    hex[64] = 0;
}

/* ------------------------------------------------------------------ the command */

static void print_line(const char *hex, CONST_STRPTR name, BOOL quiet)
{
    PutStr(hex);
    if (!quiet)
    {
        PutStr("  ");
        PutStr(name);
    }
    PutStr("\n");
}

/* The digest of what fh holds, to its end. FALSE on a read error or ^C. */
static BOOL sum_handle(BPTR fh, UBYTE *buf, char *hex)
{
    struct Sha256 s;
    LONG n;

    sha256_init(&s);
    while ((n = Read(fh, buf, BUFSIZE)) > 0)
    {
        sha256_update(&s, buf, (ULONG)n);
        if (SetSignal(0, SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C)
        {
            SetIoErr(ERROR_BREAK);
            return FALSE;
        }
    }
    if (n < 0)
        return FALSE;
    sha256_final(&s, hex);
    return TRUE;
}

int main(void)
{
    enum { ARG_FILE, ARG_TEXT, ARG_QUIET, ARG_COUNT };
    IPTR args[ARG_COUNT] = { 0 };
    struct RDArgs *rda;
    UBYTE *buf;
    char hex[65];
    LONG rc = RETURN_OK;

    if ((rda = ReadArgs("FILE/M,TEXT/K,QUIET/S", args, NULL)) == NULL)
    {
        PrintFault(IoErr(), "Sha256Sum");
        return RETURN_FAIL;
    }

    if (args[ARG_TEXT])
    {
        CONST_STRPTR text = (CONST_STRPTR)args[ARG_TEXT];
        struct Sha256 s;

        sha256_init(&s);
        sha256_update(&s, (const UBYTE *)text, strlen(text));
        sha256_final(&s, hex);
        print_line(hex, "-", args[ARG_QUIET] != 0);
        FreeArgs(rda);
        return RETURN_OK;
    }

    if ((buf = AllocMem(BUFSIZE, MEMF_ANY)) == NULL)
    {
        PrintFault(ERROR_NO_FREE_STORE, "Sha256Sum");
        FreeArgs(rda);
        return RETURN_FAIL;
    }

    if (args[ARG_FILE] == 0 || ((STRPTR *)args[ARG_FILE])[0] == NULL)
    {
        if (sum_handle(Input(), buf, hex))
            print_line(hex, "-", args[ARG_QUIET] != 0);
        else
        {
            PrintFault(IoErr(), "Sha256Sum: standard input");
            rc = RETURN_ERROR;
        }
    }
    else
    {
        STRPTR *name;

        for (name = (STRPTR *)args[ARG_FILE]; *name; name++)
        {
            BOOL stdin_ = strcmp((const char *)*name, "-") == 0;
            BPTR fh = stdin_ ? Input() : Open(*name, MODE_OLDFILE);
            BOOL ok = fh != BNULL && sum_handle(fh, buf, hex);
            LONG err = IoErr();

            if (fh != BNULL && !stdin_)
                Close(fh);
            if (ok)
                print_line(hex, *name, args[ARG_QUIET] != 0);
            else
            {
                PrintFault(err, *name);
                rc = RETURN_ERROR;
                if (err == ERROR_BREAK)
                    break;
            }
        }
    }

    FreeMem(buf, BUFSIZE);
    FreeArgs(rda);
    return rc;
}
