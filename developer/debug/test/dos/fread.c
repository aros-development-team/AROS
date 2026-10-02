/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#include <proto/dos.h>
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <dos/stdio.h>
#include <exec/memory.h>
#include <stdio.h>
#include <string.h>
#include "../../../../rom/dos/dos_fhflags.h"
#include "test.h"


BPTR fh = BNULL;
static UBYTE *data = NULL;
static UBYTE *readbuf = NULL;
#define DATA_SIZE 65536
#define LARGE_READ 8192

static void closehandles()
{
    if (fh != BNULL) Close(fh);
    fh = BNULL;
}
int main()
{
    LONG result = 0;
    LONG ioerr = 0;
    TEXT buffer[16];
    ULONG i;

    fh = Open("T:a", MODE_NEWFILE);

    /* Invalid parameters */
    SetIoErr(0);
    result = FRead(fh, buffer, 0, 0);
    ioerr = IoErr();
    TEST((result == 0));
    TEST((ioerr == 0));

    closehandles();
    data = AllocVec(DATA_SIZE, MEMF_ANY);
    readbuf = AllocVec(DATA_SIZE, MEMF_ANY);
    TEST(data != NULL && readbuf != NULL);
    for (i = 0; i < DATA_SIZE; i++)
        data[i] = i & 0xff;
    fh = Open("T:a", MODE_NEWFILE);
    TEST(fh != BNULL);
    TEST(Write(fh, data, DATA_SIZE) == DATA_SIZE);
    closehandles();

    /* Direct reads must preserve the last byte for either UnGetC form. */
    fh = Open("T:a", MODE_OLDFILE);
    TEST(fh != BNULL);
    TEST(FRead(fh, readbuf, 1, LARGE_READ) == LARGE_READ);
    TEST(memcmp(readbuf, data, LARGE_READ) == 0);
    TEST(UnGetC(fh, -1) != 0);
    TEST(FGetC(fh) == data[LARGE_READ - 1]);
    TEST(FGetC(fh) == data[LARGE_READ]);
    closehandles();

    fh = Open("T:a", MODE_OLDFILE);
    TEST(fh != BNULL);
    TEST(FRead(fh, readbuf, 1, LARGE_READ) == LARGE_READ);
    TEST(UnGetC(fh, 'X') != 0);
    TEST(FGetC(fh) == 'X');
    TEST(FGetC(fh) == data[LARGE_READ]);
    closehandles();

    /* Unread buffered data and direct reads must agree on file position. */
    fh = Open("T:a", MODE_OLDFILE);
    TEST(fh != BNULL);
    TEST(FRead(fh, readbuf, 1, 17) == 17);
    TEST(FRead(fh, readbuf + 17, 1, LARGE_READ) == LARGE_READ);
    TEST(memcmp(readbuf, data, LARGE_READ + 17) == 0);
    TEST(Seek(fh, 0, OFFSET_CURRENT) == LARGE_READ + 17);
    TEST(FRead(fh, readbuf, 1, 23) == 23);
    TEST(memcmp(readbuf, data + LARGE_READ + 17, 23) == 0);
    TEST(Seek(fh, 0, OFFSET_CURRENT) == LARGE_READ + 40);
    TEST(FRead(fh, readbuf, 1, LARGE_READ) == LARGE_READ);
    TEST(memcmp(readbuf, data + LARGE_READ + 40, LARGE_READ) == 0);
    TEST(Seek(fh, 0, OFFSET_CURRENT) == 2 * LARGE_READ + 40);
    closehandles();

    /* Append pending buffered bytes before a large byte-block write. */
    fh = Open("T:a", MODE_NEWFILE);
    TEST(fh != BNULL);
    TEST(Write(fh, "start", 5) == 5);
    closehandles();
    fh = Open("T:a", MODE_READWRITE);
    TEST(fh != BNULL);
    TEST(SetVBuf(fh, NULL, BUF_FULL, 256) == 0);
    ((struct FileHandle *)BADDR(fh))->fh_Flags |= FHF_APPEND;
    TEST(FWrite(fh, "pending", 1, 7) == 7);
    TEST(((struct FileHandle *)BADDR(fh))->fh_Pos == 7);
    TEST(FWrite(fh, data, 1, LARGE_READ) == LARGE_READ);
    TEST(Flush(fh) != 0);
    closehandles();
    fh = Open("T:a", MODE_OLDFILE);
    TEST(fh != BNULL);
    TEST(Read(fh, readbuf, 12 + LARGE_READ) == 12 + LARGE_READ);
    TEST(memcmp(readbuf, "startpending", 12) == 0);
    TEST(memcmp(readbuf + 12, data, LARGE_READ) == 0);
    TEST(Read(fh, buffer, 1) == 0);

    /* EOF */
    SetIoErr(0);
    result = FRead(fh, buffer, 1, 1);
    ioerr = IoErr();
    TEST((result == 0));
    TEST((ioerr == 0));

    /* BNULL file handle */
    SetIoErr(0);
    result = FRead(BNULL, buffer, 1, 1);
    ioerr = IoErr();
    TEST((result == 0));
    TEST((ioerr == 0));

    cleanup();

    return OK;
}

void cleanup()
{
    closehandles();
    if (data) FreeVec(data);
    if (readbuf) FreeVec(readbuf);
    data = readbuf = NULL;
    DeleteFile("T:a");
}
