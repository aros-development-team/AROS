/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Test mounted RAM-drive geometry, disk formatting and transfer bounds.
    Uses an unused unit and a temporary DOS device entry; no disk is mounted.
*/
#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/filehandler.h>
#include <devices/trackdisk.h>
#include <exec/memory.h>
#include <string.h>

#define TEST_UNIT 7

static ULONG failures;

static void check(BOOL okay, CONST_STRPTR what)
{
    if (!okay)
    {
        bug("RAMDRIVE FAIL: %s\n", what);
        failures++;
    }
}

static LONG transfer(struct IOExtTD *io, UWORD command, ULONG offset,
                     APTR data, ULONG length)
{
    io->iotd_Req.io_Command = command;
    io->iotd_Req.io_Offset = offset;
    io->iotd_Req.io_Data = data;
    io->iotd_Req.io_Length = length;
    return DoIO((struct IORequest *)io);
}

static void diskcase(struct IOExtTD *io, struct DosList *entry,
                     struct DosEnvec *env, ULONG low, ULONG high, ULONG type)
{
    ULONG sectors = (high + 1) * 22;
    ULONG first = low * 22;
    ULONG root = (sectors - first) / 2;
    ULONG data[128], copy[128], i, sum;
    ULONG available;
    LONG error;

    env->de_LowCyl = low;
    env->de_HighCyl = high;
    env->de_DosType = type;
    check(AddDosEntry(entry), "add mounted-unit geometry");
    available = AvailMem(MEMF_PUBLIC);
    error = OpenDevice("ramdrive.device", TEST_UNIT, (struct IORequest *)io, 0);
    check(error == 0, "open mounted unit");
    if (error == 0)
    {
        /* Distinguish the requested small disk from the old 880 KiB image. */
        check(available - AvailMem(MEMF_PUBLIC) < sectors * 512 + 65536,
              "allocation follows mounted geometry");
        check(transfer(io, TD_GETNUMTRACKS, 0, NULL, 0) == 0 &&
              io->iotd_Req.io_Actual == (high + 1) * 2, "track count");
        check(transfer(io, CMD_READ, first * 512, data, sizeof(data)) == 0 &&
              AROS_BE2LONG(data[0]) == type, "boot block filesystem type");
        check(transfer(io, CMD_READ, (first + root) * 512,
                       data, sizeof(data)) == 0, "read root block");
        sum = 0;
        for (i = 0; i < 128; i++)
            sum += AROS_BE2LONG(data[i]);
        check(sum == 0 && AROS_BE2LONG(data[0]) == 2 &&
              AROS_BE2LONG(data[79]) == root + 1, "root location and checksum");
        check(transfer(io, CMD_READ, (first + root + 1) * 512,
                       data, sizeof(data)) == 0, "read allocation bitmap");
        sum = 0;
        for (i = 0; i < 128; i++)
            sum += AROS_BE2LONG(data[i]);
        check(sum == 0, "bitmap checksum");
        for (i = 0; i < 128; i++)
            data[i] = i * 0x01020304UL;
        check(transfer(io, CMD_WRITE, (sectors - 1) * 512,
                       data, sizeof(data)) == 0, "write last sector");
        check(transfer(io, CMD_READ, (sectors - 1) * 512,
                       copy, sizeof(copy)) == 0 &&
              memcmp(data, copy, sizeof(data)) == 0, "read back last sector");
        check(transfer(io, CMD_READ, sectors * 512, copy, sizeof(copy)) ==
              TDERR_SeekError, "reject read beyond disk");
        check(transfer(io, CMD_WRITE, 0xfffffff0UL, data, sizeof(data)) ==
              TDERR_SeekError, "reject overflowing write offset");
        CloseDevice((struct IORequest *)io);
    }
    check(RemDosEntry(entry), "remove temporary geometry");
}

int main(void)
{
    struct MsgPort *port = CreateMsgPort();
    struct IOExtTD *io = port ? (struct IOExtTD *)CreateIORequest(port, sizeof(*io)) : NULL;
    struct DosList *entry = MakeDosEntry("RamDriveTest", DLT_DEVICE);
    struct FileSysStartupMsg *startup = AllocVec(sizeof(*startup), MEMF_PUBLIC | MEMF_CLEAR);
    struct DosEnvec *env = AllocVec(sizeof(*env), MEMF_PUBLIC | MEMF_CLEAR);
    UBYTE *name = AllocVec(AROS_BSTR_MEMSIZE4LEN(sizeof("ramdrive.device")), MEMF_PUBLIC | MEMF_CLEAR);
    ULONG i;

    if (!io || !entry || !startup || !env || !name)
    {
        check(FALSE, "allocate test resources");
        goto end;
    }
    startup->fssm_Device = MKBADDR(name);
    AROS_BSTR_setstrlen(startup->fssm_Device, sizeof("ramdrive.device"));
    for (i = 0; i < sizeof("ramdrive.device"); i++)
        AROS_BSTR_putchar(startup->fssm_Device, i, "ramdrive.device"[i]);
    startup->fssm_Unit = TEST_UNIT;
    startup->fssm_Environ = MKBADDR(env);
    entry->dol_misc.dol_handler.dol_Startup = MKBADDR(startup);
    env->de_TableSize = DE_DOSTYPE;
    env->de_SizeBlock = 128;
    env->de_Surfaces = 2;
    env->de_SectorPerBlock = 1;
    env->de_BlocksPerTrack = 11;
    env->de_Reserved = 2;

    diskcase(io, entry, env, 0, 11, ID_DOS_DISK);
    diskcase(io, entry, env, 1, 12, ID_FFS_DISK);
    diskcase(io, entry, env, 0, 79, ID_DOS_DISK);
    env->de_HighCyl = 0xffffffffUL;
    check(AddDosEntry(entry), "add invalid geometry");
    if (OpenDevice("ramdrive.device", TEST_UNIT, (struct IORequest *)io, 0) == 0)
    {
        check(FALSE, "reject overflowing geometry");
        CloseDevice((struct IORequest *)io);
    }
    check(RemDosEntry(entry), "remove invalid geometry");
    if (OpenDevice("ramdrive.device", TEST_UNIT, (struct IORequest *)io, 0) == 0)
    {
        check(transfer(io, TD_GETNUMTRACKS, 0, NULL, 0) == 0 &&
              io->iotd_Req.io_Actual == 160, "unmounted-unit default");
        CloseDevice((struct IORequest *)io);
    }
    else
        check(FALSE, "open unmounted unit");
end:
    if (name) FreeVec(name);
    if (env) FreeVec(env);
    if (startup) FreeVec(startup);
    if (entry) FreeDosEntry(entry);
    if (io) DeleteIORequest((struct IORequest *)io);
    if (port) DeleteMsgPort(port);
    bug("RAMDRIVE TEST: %lu failures\n", (unsigned long)failures);
    return failures ? 20 : 0;
}
