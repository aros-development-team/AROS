/* The AROS clipboard for Rust: text on clipboard unit 0 as IFF FTXT.
   Contract: glue.h.

   A separate object from glue.c, so only programs that use the clipboard
   pull in iffparse.library. */
#include <stdint.h>
#include <exec/memory.h>
#include <devices/clipboard.h>
#include <libraries/iffparse.h>
#include <proto/exec.h>
#include <proto/iffparse.h>

#include "glue.h"

#define ID_FTXT MAKE_ID('F', 'T', 'X', 'T')
#define ID_CHRS MAKE_ID('C', 'H', 'R', 'S')

static struct IFFHandle *open_clip(LONG mode)
{
    struct IFFHandle *iff = AllocIFF();

    if (!iff)
        return NULL;
    iff->iff_Stream = (IPTR)OpenClipboard(PRIMARY_CLIP);
    if (iff->iff_Stream) {
        InitIFFasClip(iff);
        if (OpenIFF(iff, mode) == 0)
            return iff;
        CloseClipboard((struct ClipboardHandle *)iff->iff_Stream);
    }
    FreeIFF(iff);
    return NULL;
}

static void close_clip(struct IFFHandle *iff)
{
    CloseIFF(iff);
    CloseClipboard((struct ClipboardHandle *)iff->iff_Stream);
    FreeIFF(iff);
}

int aros_gui_clip_write(const uint8_t *text, int32_t len)
{
    struct IFFHandle *iff = open_clip(IFFF_WRITE);
    int rc = -1;

    if (!iff)
        return -1;
    if (PushChunk(iff, ID_FTXT, ID_FORM, IFFSIZE_UNKNOWN) == 0) {
        if (PushChunk(iff, 0, ID_CHRS, IFFSIZE_UNKNOWN) == 0) {
            if (WriteChunkBytes(iff, (APTR)text, len) == len)
                rc = 0;
            PopChunk(iff);
        }
        PopChunk(iff);
    }
    close_clip(iff);
    return rc;
}

uint8_t *aros_gui_clip_read(int32_t *len)
{
    struct IFFHandle *iff = open_clip(IFFF_READ);
    UBYTE *text = NULL;
    ULONG size = 0;

    *len = 0;
    if (!iff)
        return NULL;

    /* Every CHRS chunk of every FTXT, in order. */
    if (StopChunk(iff, ID_FTXT, ID_CHRS) == 0) {
        while (ParseIFF(iff, IFFPARSE_SCAN) == 0) {
            struct ContextNode *cn = CurrentChunk(iff);
            UBYTE *more;
            LONG got;

            if (!cn || cn->cn_Size <= 0)
                continue;
            more = AllocVec(size + cn->cn_Size, MEMF_ANY);
            if (!more)
                break;
            if (text) {
                CopyMem(text, more, size);
                FreeVec(text);
            }
            text = more;
            got = ReadChunkBytes(iff, text + size, cn->cn_Size);
            if (got > 0)
                size += got;
        }
    }
    close_clip(iff);

    if (text && size == 0) {
        FreeVec(text);
        text = NULL;
    }
    *len = size;
    return text;
}

void aros_gui_clip_free(uint8_t *text)
{
    FreeVec(text);
}
