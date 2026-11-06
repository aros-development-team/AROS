/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <exec/memory.h>
#include <exec/ports.h>
#include <dos/dos.h>
#include <libraries/iffparse.h>
#include <datatypes/textclass.h>
#include <proto/exec.h>
#include <proto/iffparse.h>

#include <stdio.h>
#include <string.h>

#define CODE_READ_TEXT  0x100
#define CODE_WRITE_TEXT 0x101

#define TEXT_VERSION     1
#define TEXT_UNSUPPORTED 0
#define TEXT_OK          1
#define TEXT_ERROR       2
#define TEXT_ENC_LEGACY  0
#define TEXT_ENC_UTF8    1

#define ID_UTF8          MAKE_ID('U','T','F','8')
#define ID_CSET          MAKE_ID('C','S','E','T')

struct SGWork;

struct ConClipTextMsg
{
    struct Message      msg;
    struct SGWork       *sgw;
    WORD                code;
    ULONG               version;
    ULONG               status;
    UBYTE               *data;
    ULONG               length;
    ULONG               capacity;
    ULONG               encoding;
};

static struct MsgPort *replyport;
static int failures;

static void check(BOOL ok, const char *what)
{
    printf("%s: %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok)
        failures++;
}

static BOOL sendmsg(struct ConClipTextMsg *msg)
{
    struct MsgPort *port;

    msg->msg.mn_Node.ln_Type = NT_MESSAGE;
    msg->msg.mn_ReplyPort = replyport;
    msg->msg.mn_Length = sizeof(*msg);

    Forbid();
    if ((port = FindPort("ConClip.rendezvous")))
        PutMsg(port, &msg->msg);
    Permit();

    if (!port)
        return FALSE;

    WaitPort(replyport);
    GetMsg(replyport);
    return TRUE;
}

static ULONG writetextenc(const char *text, ULONG length, ULONG encoding)
{
    struct ConClipTextMsg msg = { 0 };

    msg.code = CODE_WRITE_TEXT;
    msg.version = TEXT_VERSION;
    msg.data = (UBYTE *)text;
    msg.length = msg.capacity = length;
    msg.encoding = encoding;
    sendmsg(&msg);
    return msg.status;
}

static ULONG writetext(const char *text, ULONG length)
{
    return writetextenc(text, length, TEXT_ENC_LEGACY);
}

/* read with the encoding asked for; got: the encoding of the reply */
static BOOL readmatchesenc(const char *text, ULONG length, ULONG encoding, ULONG *got)
{
    struct ConClipTextMsg msg = { 0 };
    BOOL ok;

    msg.code = CODE_READ_TEXT;
    msg.version = TEXT_VERSION;
    msg.encoding = encoding;
    sendmsg(&msg);
    if (got)
        *got = msg.encoding;
    if (msg.status != TEXT_OK)
        return FALSE;

    ok = msg.length == length && msg.capacity >= length &&
        (length == 0 || memcmp(msg.data, text, length) == 0);
    if (msg.data)
        FreeMem(msg.data, msg.capacity);
    return ok;
}

static BOOL readmatches(const char *text, ULONG length)
{
    return readmatchesenc(text, length, TEXT_ENC_LEGACY, NULL);
}

/* a clip written the AmigaOS 4 way: CSET 106 says the CHRS bytes are UTF-8 */
static BOOL writecsetutf8(const char *text)
{
    struct IFFHandle *iff;
    ULONG cset[8] = { 106, 0, 0, 0, 0, 0, 0, 0 };
    BOOL ok = FALSE;

    if (!(iff = AllocIFF()))
        return FALSE;
    if ((iff->iff_Stream = (IPTR)OpenClipboard(0)))
    {
        InitIFFasClip(iff);
        if (!OpenIFF(iff, IFFF_WRITE))
        {
            ok = !PushChunk(iff, ID_FTXT, ID_FORM, IFFSIZE_UNKNOWN) &&
                !PushChunk(iff, ID_FTXT, ID_CSET, IFFSIZE_UNKNOWN) &&
                WriteChunkBytes(iff, cset, sizeof(cset)) == sizeof(cset) &&
                !PopChunk(iff) &&
                !PushChunk(iff, ID_FTXT, ID_CHRS, IFFSIZE_UNKNOWN) &&
                WriteChunkBytes(iff, (APTR)text, strlen(text)) == strlen(text) &&
                !PopChunk(iff) &&
                !PopChunk(iff);
            CloseIFF(iff);
        }
        CloseClipboard((struct ClipboardHandle *)iff->iff_Stream);
    }
    FreeIFF(iff);
    return ok;
}

static BOOL writetwochunks(const char *a, const char *b)
{
    struct IFFHandle *iff;
    BOOL ok = FALSE;

    if (!(iff = AllocIFF()))
        return FALSE;
    if ((iff->iff_Stream = (IPTR)OpenClipboard(0)))
    {
        InitIFFasClip(iff);
        if (!OpenIFF(iff, IFFF_WRITE))
        {
            ok = !PushChunk(iff, ID_FTXT, ID_FORM, IFFSIZE_UNKNOWN) &&
                !PushChunk(iff, ID_FTXT, ID_CHRS, IFFSIZE_UNKNOWN) &&
                WriteChunkBytes(iff, (APTR)a, strlen(a)) == strlen(a) &&
                !PopChunk(iff) &&
                !PushChunk(iff, ID_FTXT, ID_CHRS, IFFSIZE_UNKNOWN) &&
                WriteChunkBytes(iff, (APTR)b, strlen(b)) == strlen(b) &&
                !PopChunk(iff) &&
                !PopChunk(iff);
            CloseIFF(iff);
        }
        CloseClipboard((struct ClipboardHandle *)iff->iff_Stream);
    }
    FreeIFF(iff);
    return ok;
}

int main(void)
{
    static const char text[] = "first line\nsecond line\n";
    struct ConClipTextMsg msg = { 0 };
    char *big;
    ULONG i, bigsize = 100000;

    if (!FindPort("ConClip.rendezvous"))
    {
        printf("ConClip is not running\n");
        return RETURN_FAIL;
    }
    if (!(replyport = CreateMsgPort()))
        return RETURN_FAIL;

    check(writetext(text, sizeof(text) - 1) == TEXT_OK, "write text");
    check(readmatches(text, sizeof(text) - 1), "read back text");

    check(writetext("", 0) == TEXT_OK, "write empty text");
    check(readmatches("", 0), "read back empty text");

    if ((big = AllocMem(bigsize, MEMF_ANY)))
    {
        for (i = 0; i < bigsize; i++)
            big[i] = 'a' + (i % 26);
        check(writetext(big, bigsize) == TEXT_OK, "write 100000 bytes");
        check(readmatches(big, bigsize), "read back 100000 bytes");
        FreeMem(big, bigsize);
    }
    else
        check(FALSE, "allocate 100000 bytes");

    check(writetwochunks("two ", "chunks"), "write two CHRS chunks directly");
    check(readmatches("two chunks", 10), "read joins both chunks");

    /* encodings: e-acute, a smiling face and a CJK character */
    {
        static const char utf8[] = "caf\xc3\xa9 \xf0\x9f\x99\x82 \xe6\xbc\xa2";
        static const char latin1[] = "caf\xe9 ? ?";
        static const char plain[] = "caf\xe9";
        static const char plainutf8[] = "caf\xc3\xa9";
        ULONG got;

        check(writetextenc(utf8, sizeof(utf8) - 1, TEXT_ENC_UTF8) == TEXT_OK, "write UTF-8 text");
        check(readmatchesenc(utf8, sizeof(utf8) - 1, TEXT_ENC_UTF8, &got) && got == TEXT_ENC_UTF8, "read it back as UTF-8");
        check(readmatchesenc(latin1, sizeof(latin1) - 1, TEXT_ENC_LEGACY, &got) && got == TEXT_ENC_LEGACY,
            "a legacy reader gets the Latin-1 rendering from CHRS");
        check(readmatches(latin1, sizeof(latin1) - 1), "the old request form reads the same CHRS");

        check(writetext(plain, sizeof(plain) - 1) == TEXT_OK, "write Latin-1 text the legacy way");
        check(readmatchesenc(plainutf8, sizeof(plainutf8) - 1, TEXT_ENC_UTF8, &got) && got == TEXT_ENC_UTF8,
            "read Latin-1 CHRS as UTF-8");
        check(readmatchesenc(plain, sizeof(plain) - 1, TEXT_ENC_LEGACY, &got) && got == TEXT_ENC_LEGACY,
            "read Latin-1 CHRS as it is");

        check(writecsetutf8(utf8), "write CSET 106 + UTF-8 CHRS directly");
        check(readmatchesenc(utf8, sizeof(utf8) - 1, TEXT_ENC_UTF8, &got) && got == TEXT_ENC_UTF8,
            "CSET 106 marks the CHRS bytes as UTF-8");
        check(readmatchesenc(utf8, sizeof(utf8) - 1, TEXT_ENC_LEGACY, &got) && got == TEXT_ENC_UTF8,
            "a legacy read of it reports UTF-8");

        check(writetextenc("bad \xc3", 5, TEXT_ENC_UTF8) == TEXT_ERROR, "truncated UTF-8 is refused");
        check(writetextenc("ascii", 5, 7) == TEXT_ERROR, "an unknown encoding is refused");
        check(writetextenc("ascii", 5, TEXT_ENC_UTF8) == TEXT_OK && readmatches("ascii", 5), "ASCII is the same in both");
    }

    msg.code = CODE_WRITE_TEXT;
    msg.version = TEXT_VERSION;
    msg.data = (UBYTE *)text;
    msg.length = 10;
    msg.capacity = 5;
    sendmsg(&msg);
    check(msg.status == TEXT_ERROR, "write with length beyond capacity is refused");

    memset(&msg, 0, sizeof(msg));
    msg.code = CODE_READ_TEXT;
    msg.version = TEXT_VERSION + 1;
    sendmsg(&msg);
    check(msg.status == TEXT_UNSUPPORTED && msg.data == NULL, "unknown version is left unanswered");

    DeleteMsgPort(replyport);

    printf("%s\n", failures ? "CONCLIP-TEST FAIL" : "CONCLIP-TEST PASS");
    return failures ? RETURN_FAIL : RETURN_OK;
}
