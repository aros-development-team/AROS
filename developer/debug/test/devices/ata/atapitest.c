/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: ata.device ATAPI transport test.

          Exercises an ATAPI optical unit through the normal device
          interfaces: HD_SCSICMD (INQUIRY, TEST UNIT READY, REQUEST SENSE,
          READ CAPACITY, READ(10), illegal commands), CMD_READ/TD_READ64,
          TD_GETGEOMETRY, TD_CHANGENUM/TD_CHANGESTATE, TD_ADDCHANGEINT and
          asynchronous SendIO/AbortIO/WaitIO usage. Results are checked for
          SCSI status, sense data, transfer counts and buffer overruns.

          Silent unless a check fails, or VERBOSE is given. All output is
          also sent to the debug console, so a serial log of a boot can be
          used to evaluate the run.

          Usage:
            atapitest [DEVICE=ata.device] [UNIT=n] [VERBOSE] [NOMEDIA]
                      [WAITMEDIA=secs] [VERIFYFILE=path]

          Without UNIT the first unit reporting DG_CDROM is used. NOMEDIA
          checks the no-media error paths instead of reading. WAITMEDIA
          waits for media to appear (insert a disc) and then runs the media
          tests; VERIFYFILE reads a file of the pattern
          byte[i] = (i * 7 + 3) & 0xff through the filesystem afterwards.
*/

#include <aros/debug.h>
#include <aros/asmcall.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/io.h>
#include <exec/memory.h>
#include <exec/interrupts.h>
#include <exec/errors.h>
#include <devices/trackdisk.h>
#include <devices/scsidisk.h>
#include <devices/newstyle.h>
#include <dos/dos.h>

#include <stdio.h>
#include <string.h>

#define TEMPLATE "DEVICE/K,UNIT/N/K,VERBOSE/S,NOMEDIA/S,WAITMEDIA/N/K,VERIFYFILE/K"

struct Args
{
    STRPTR device;
    IPTR  *unit;
    IPTR   verbose;
    IPTR   nomedia;
    IPTR  *waitmedia;
    STRPTR verifyfile;
};

#define SECTOR          2048
#define BIGREAD         (80 * 1024)     /* > 64 KiB: exercises the ATAPI byte count limit */
#define GUARD           64
#define GUARDBYTE       0xA5

#define SCSI_TESTUNITREADY  0x00
#define SCSI_REQUESTSENSE   0x03
#define SCSI_INQUIRY        0x12
#define SCSI_READCAPACITY   0x25
#define SCSI_READ10         0x28

#define SCSI_STATUS_CHECK   0x02
#define SCSI_STATUS_BUSY    0x08
#define SK_NOT_READY        0x02
#define SK_ILLEGAL_REQUEST  0x05
#define SK_UNIT_ATTENTION   0x06

static BOOL verbose = FALSE;
static LONG failures = 0;
static LONG warnings = 0;
static LONG checks = 0;

static struct MsgPort *port;
static struct IOStdReq *io;
static struct SCSICmd sc;
static UBYTE sense[32];
static UBYTE cdb[16];
static volatile ULONG changeints = 0;

#define LOG(...)                            \
    do {                                    \
        bug("[atapitest] " __VA_ARGS__);    \
        if (verbose)                        \
            printf(__VA_ARGS__);            \
    } while (0)

#define FAIL(...)                           \
    do {                                    \
        failures++;                         \
        bug("[atapitest] FAIL: " __VA_ARGS__); \
        printf("FAIL: " __VA_ARGS__);       \
    } while (0)

#define CHECK(cond, ...)                    \
    do {                                    \
        checks++;                           \
        if (!(cond))                        \
            FAIL(__VA_ARGS__);              \
    } while (0)

/* Media change soft interrupt (TD_ADDCHANGEINT) */
AROS_INTH1(ChangeInt, volatile ULONG *, counter)
{
    AROS_INTFUNC_INIT

    (*counter)++;
    return FALSE;

    AROS_INTFUNC_EXIT
}

static void fillguard(UBYTE *buf, ULONG len)
{
    memset(buf, GUARDBYTE, len);
}

static BOOL guardok(UBYTE *buf, ULONG from, ULONG to)
{
    ULONG i;

    for (i = from; i < to; i++)
        if (buf[i] != GUARDBYTE)
            return FALSE;
    return TRUE;
}

static LONG tdcmd(UWORD command, APTR data, ULONG length, ULONG offset)
{
    io->io_Command = command;
    io->io_Data    = data;
    io->io_Length  = length;
    io->io_Offset  = offset;
    io->io_Actual  = 0;
    return DoIO((struct IORequest *)io);
}

/* Synchronous HD_SCSICMD with autosense into sense[] */
static LONG scsi(const UBYTE *cmd, UWORD cmdlen, APTR data, ULONG len, UBYTE flags)
{
    LONG err;

    memset(&sc, 0, sizeof(sc));
    memset(sense, 0, sizeof(sense));
    sc.scsi_Data        = data;
    sc.scsi_Length      = len;
    sc.scsi_Command     = (UBYTE *)cmd;
    sc.scsi_CmdLength   = cmdlen;
    sc.scsi_Flags       = flags | SCSIF_AUTOSENSE;
    sc.scsi_SenseData   = sense;
    sc.scsi_SenseLength = sizeof(sense);

    io->io_Command = HD_SCSICMD;
    io->io_Data    = &sc;
    io->io_Length  = sizeof(sc);
    err = DoIO((struct IORequest *)io);

    LOG("  cdb %02x: io_Error %ld, status %d, actual %lu/%lu, sense %u bytes (%02x/%02x/%02x)\n",
        cmd[0], (long)err, sc.scsi_Status, (unsigned long)sc.scsi_Actual, (unsigned long)len,
        sc.scsi_SenseActual, sense[2] & 0x0f, sense[12], sense[13]);
    return err;
}

static void expect_check(const char *what, LONG err, UBYTE key, UBYTE asc)
{
    CHECK(err != 0, "%s: no io_Error reported\n", what);
    CHECK(sc.scsi_Status == SCSI_STATUS_CHECK, "%s: scsi_Status %d, expected CHECK CONDITION\n", what, sc.scsi_Status);
    CHECK(sc.scsi_SenseActual >= 14, "%s: scsi_SenseActual %d, expected sense data\n", what, sc.scsi_SenseActual);
    CHECK((sense[2] & 0x0f) == key, "%s: sense key %x, expected %x\n", what, sense[2] & 0x0f, key);
    if (asc)
        CHECK(sense[12] == asc, "%s: ASC %02x, expected %02x\n", what, sense[12], asc);
}

static BOOL media_present(void)
{
    tdcmd(TD_CHANGESTATE, NULL, 0, 0);
    return io->io_Actual == 0;
}

static void test_inquiry(void)
{
    UBYTE *buf = AllocMem(256 + GUARD, MEMF_PUBLIC);
    LONG err;

    if (!buf)
        return;

    LOG("INQUIRY (36 bytes)\n");
    memset(cdb, 0, 6);
    cdb[0] = SCSI_INQUIRY; cdb[4] = 36;
    fillguard(buf, 256 + GUARD);
    err = scsi(cdb, 6, buf, 36, SCSIF_READ);
    CHECK(err == 0 && sc.scsi_Status == 0, "INQUIRY: error %ld status %d\n", (long)err, sc.scsi_Status);
    CHECK(sc.scsi_Actual == 36, "INQUIRY: actual %lu, expected 36\n", (unsigned long)sc.scsi_Actual);
    CHECK(sc.scsi_CmdActual == 6, "INQUIRY: scsi_CmdActual %d, expected 6\n", sc.scsi_CmdActual);
    CHECK((buf[0] & 0x1f) == 5, "INQUIRY: peripheral device type %x, expected 5 (CD-ROM)\n", buf[0] & 0x1f);
    CHECK(guardok(buf, 36, 256 + GUARD), "INQUIRY: wrote beyond the 36 requested bytes\n");
    LOG("  vendor '%.8s' product '%.16s' rev '%.4s'\n", buf + 8, buf + 16, buf + 32);

    /* Short transfer: ask for more than the device has */
    LOG("INQUIRY (96 bytes requested, short transfer)\n");
    cdb[4] = 96;
    fillguard(buf, 256 + GUARD);
    err = scsi(cdb, 6, buf, 96, SCSIF_READ);
    CHECK(err == 0 && sc.scsi_Status == 0, "INQUIRY(96): error %ld status %d\n", (long)err, sc.scsi_Status);
    CHECK(sc.scsi_Actual >= 36 && sc.scsi_Actual <= 96, "INQUIRY(96): actual %lu\n", (unsigned long)sc.scsi_Actual);
    CHECK(guardok(buf, sc.scsi_Actual, 256 + GUARD), "INQUIRY(96): wrote beyond scsi_Actual\n");
    CHECK(sc.scsi_Actual == (ULONG)buf[4] + 5 || sc.scsi_Actual == 96,
        "INQUIRY(96): actual %lu does not match additional length %d\n", (unsigned long)sc.scsi_Actual, buf[4]);

    /*
     * Odd length transfer: 35 bytes into a 35 byte window. Real devices
     * pad the last byte and complete; QEMU's IDE emulation never finishes
     * an odd-sized PIO transfer (ide_data_readw() refuses the final word),
     * so the driver's timeout and device reset path is exercised instead.
     * That case is reported as a warning, not a failure.
     */
    LOG("INQUIRY (35 bytes, odd length)\n");
    cdb[4] = 35;
    fillguard(buf, 256 + GUARD);
    err = scsi(cdb, 6, buf, 35, SCSIF_READ);
    if ((err == IOERR_UNITBUSY) && (sc.scsi_Status == SCSI_STATUS_BUSY))
    {
        warnings++;
        LOG("  WARNING: device did not complete the odd-length PIO transfer (expected on QEMU)\n");
    }
    else
    {
        CHECK(err == 0 && sc.scsi_Status == 0, "INQUIRY(35): error %ld status %d\n", (long)err, sc.scsi_Status);
        CHECK(sc.scsi_Actual == 35, "INQUIRY(35): actual %lu, expected 35\n", (unsigned long)sc.scsi_Actual);
        CHECK((buf[0] & 0x1f) == 5, "INQUIRY(35): bad data\n");
    }
    CHECK(guardok(buf, 35, 256 + GUARD), "INQUIRY(35): wrote beyond the 35 requested bytes\n");

    /* Larger buffer than allocation length: the device decides */
    LOG("INQUIRY (36 bytes into 254 byte buffer)\n");
    cdb[4] = 36;
    fillguard(buf, 256 + GUARD);
    err = scsi(cdb, 6, buf, 254, SCSIF_READ);
    CHECK(err == 0 && sc.scsi_Status == 0, "INQUIRY(36/254): error %ld status %d\n", (long)err, sc.scsi_Status);
    CHECK(sc.scsi_Actual == 36, "INQUIRY(36/254): actual %lu, expected 36\n", (unsigned long)sc.scsi_Actual);
    CHECK(guardok(buf, 36, 256 + GUARD), "INQUIRY(36/254): wrote beyond 36 bytes\n");

    FreeMem(buf, 256 + GUARD);
}

static void test_requestsense(void)
{
    UBYTE buf[32];
    LONG err;

    LOG("REQUEST SENSE (explicit)\n");
    memset(cdb, 0, 6);
    cdb[0] = SCSI_REQUESTSENSE; cdb[4] = 18;
    fillguard(buf, sizeof(buf));
    err = scsi(cdb, 6, buf, 18, SCSIF_READ);
    CHECK(err == 0 && sc.scsi_Status == 0, "REQUEST SENSE: error %ld status %d\n", (long)err, sc.scsi_Status);
    CHECK(sc.scsi_Actual >= 8 && sc.scsi_Actual <= 18, "REQUEST SENSE: actual %lu\n", (unsigned long)sc.scsi_Actual);
    CHECK((buf[0] & 0x7f) == 0x70 || (buf[0] & 0x7f) == 0x71, "REQUEST SENSE: response code %02x\n", buf[0]);
    CHECK(guardok(buf, 18, sizeof(buf)), "REQUEST SENSE: wrote beyond 18 bytes\n");
}

static void test_illegal(void)
{
    LONG err;
    UBYTE buf[64];

    LOG("Illegal opcode 0xFF\n");
    memset(cdb, 0, 12);
    cdb[0] = 0xff;
    err = scsi(cdb, 12, NULL, 0, SCSIF_READ);
    expect_check("Illegal opcode", err, SK_ILLEGAL_REQUEST, 0x20);
    CHECK(sc.scsi_Actual == 0, "Illegal opcode: actual %lu\n", (unsigned long)sc.scsi_Actual);

    LOG("Invalid CDB lengths\n");
    memset(cdb, 0, 16);
    cdb[0] = SCSI_TESTUNITREADY;
    err = scsi(cdb, 0, NULL, 0, SCSIF_READ);
    CHECK(err == IOERR_NOCMD, "CDB length 0: io_Error %ld, expected IOERR_NOCMD\n", (long)err);
    err = scsi(cdb, 17, NULL, 0, SCSIF_READ);
    CHECK(err == IOERR_NOCMD, "CDB length 17: io_Error %ld, expected IOERR_NOCMD\n", (long)err);

    LOG("Data length without buffer\n");
    err = scsi(cdb, 6, NULL, 64, SCSIF_READ);
    CHECK(err == IOERR_BADADDRESS, "NULL data: io_Error %ld, expected IOERR_BADADDRESS\n", (long)err);

    LOG("HD_SCSICMD without io_Data\n");
    io->io_Command = HD_SCSICMD;
    io->io_Data    = NULL;
    io->io_Length  = sizeof(sc);
    err = DoIO((struct IORequest *)io);
    CHECK(err == IOERR_BADADDRESS, "NULL io_Data: io_Error %ld, expected IOERR_BADADDRESS\n", (long)err);

    /* Non data command carrying a data length: the device must not get stuck */
    LOG("TEST UNIT READY with a data buffer attached\n");
    err = scsi(cdb, 6, buf, sizeof(buf), SCSIF_READ);
    CHECK(sc.scsi_Actual == 0, "TUR with buffer: actual %lu, expected 0\n", (unsigned long)sc.scsi_Actual);
    memset(cdb, 0, 6);
    cdb[0] = SCSI_INQUIRY; cdb[4] = 36;
    err = scsi(cdb, 6, buf, 36, SCSIF_READ);
    CHECK(err == 0 && sc.scsi_Actual == 36, "INQUIRY after TUR with buffer failed: %ld\n", (long)err);
}

static ULONG test_capacity(void)
{
    UBYTE buf[8];
    LONG err;
    ULONG lastlba, blocklen;

    LOG("READ CAPACITY\n");
    memset(cdb, 0, 10);
    cdb[0] = SCSI_READCAPACITY;
    err = scsi(cdb, 10, buf, 8, SCSIF_READ);
    CHECK(err == 0 && sc.scsi_Status == 0, "READ CAPACITY: error %ld status %d\n", (long)err, sc.scsi_Status);
    CHECK(sc.scsi_Actual == 8, "READ CAPACITY: actual %lu\n", (unsigned long)sc.scsi_Actual);
    lastlba  = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
    blocklen = (buf[4] << 24) | (buf[5] << 16) | (buf[6] << 8) | buf[7];
    LOG("  last LBA %lu, block length %lu\n", (unsigned long)lastlba, (unsigned long)blocklen);
    CHECK(blocklen == SECTOR, "READ CAPACITY: block length %lu\n", (unsigned long)blocklen);
    CHECK(lastlba > 16, "READ CAPACITY: last LBA %lu\n", (unsigned long)lastlba);
    return lastlba;
}

static void read10(UBYTE *cmd, ULONG lba, UWORD count)
{
    memset(cmd, 0, 10);
    cmd[0] = SCSI_READ10;
    cmd[2] = lba >> 24; cmd[3] = lba >> 16; cmd[4] = lba >> 8; cmd[5] = lba;
    cmd[7] = count >> 8; cmd[8] = count;
}

static void test_read(ULONG lastlba)
{
    UBYTE *a = AllocMem(BIGREAD + GUARD, MEMF_PUBLIC);
    UBYTE *b = AllocMem(BIGREAD + GUARD, MEMF_PUBLIC);
    LONG err;
    ULONG i;

    if (!a || !b)
    {
        FAIL("out of memory\n");
        goto out;
    }

    /* READ(10) of the primary volume descriptor */
    LOG("READ(10) LBA 16, 1 sector\n");
    read10(cdb, 16, 1);
    fillguard(a, BIGREAD + GUARD);
    err = scsi(cdb, 10, a, SECTOR, SCSIF_READ);
    CHECK(err == 0 && sc.scsi_Status == 0, "READ(10): error %ld status %d\n", (long)err, sc.scsi_Status);
    CHECK(sc.scsi_Actual == SECTOR, "READ(10): actual %lu\n", (unsigned long)sc.scsi_Actual);
    CHECK(a[0] == 1 && memcmp(a + 1, "CD001", 5) == 0, "READ(10): sector 16 is not an ISO9660 PVD\n");
    CHECK(guardok(a, SECTOR, BIGREAD + GUARD), "READ(10): wrote beyond one sector\n");

    /* CMD_READ of the same sector */
    LOG("CMD_READ LBA 16, 1 sector\n");
    fillguard(b, BIGREAD + GUARD);
    err = tdcmd(CMD_READ, b, SECTOR, 16 * SECTOR);
    CHECK(err == 0, "CMD_READ: error %ld\n", (long)err);
    CHECK(io->io_Actual == SECTOR, "CMD_READ: io_Actual %lu, expected %d\n", (unsigned long)io->io_Actual, SECTOR);
    CHECK(memcmp(a, b, SECTOR) == 0, "CMD_READ: data differs from READ(10)\n");
    CHECK(guardok(b, SECTOR, BIGREAD + GUARD), "CMD_READ: wrote beyond one sector\n");

    /* 16 sectors, both ways */
    LOG("READ(10) / CMD_READ LBA 16, 16 sectors\n");
    read10(cdb, 16, 16);
    err = scsi(cdb, 10, a, 16 * SECTOR, SCSIF_READ);
    CHECK(err == 0 && sc.scsi_Actual == 16 * SECTOR, "READ(10) x16: error %ld actual %lu\n", (long)err, (unsigned long)sc.scsi_Actual);
    err = tdcmd(CMD_READ, b, 16 * SECTOR, 16 * SECTOR);
    CHECK(err == 0 && io->io_Actual == 16 * SECTOR, "CMD_READ x16: error %ld actual %lu\n", (long)err, (unsigned long)io->io_Actual);
    CHECK(memcmp(a, b, 16 * SECTOR) == 0, "16 sector read: data differs\n");

    /* TD_READ64 / NSCMD_TD_READ64 */
    LOG("NSCMD_TD_READ64 LBA 17, 2 sectors\n");
    fillguard(b, BIGREAD + GUARD);
    io->io_Command = NSCMD_TD_READ64;
    io->io_Data    = b;
    io->io_Length  = 2 * SECTOR;
    io->io_Offset  = 17 * SECTOR;
    io->io_Actual  = 0;
    err = DoIO((struct IORequest *)io);
    CHECK(err == 0 && io->io_Actual == 2 * SECTOR, "NSCMD_TD_READ64: error %ld actual %lu\n", (long)err, (unsigned long)io->io_Actual);
    CHECK(memcmp(a + SECTOR, b, 2 * SECTOR) == 0, "NSCMD_TD_READ64: data differs\n");

    /* Large read, larger than the 16 bit ATAPI byte count */
    LOG("CMD_READ %d bytes in one request\n", BIGREAD);
    fillguard(a, BIGREAD + GUARD);
    err = tdcmd(CMD_READ, a, BIGREAD, 16 * SECTOR);
    CHECK(err == 0, "CMD_READ big: error %ld\n", (long)err);
    CHECK(io->io_Actual == BIGREAD, "CMD_READ big: io_Actual %lu\n", (unsigned long)io->io_Actual);
    CHECK(guardok(a, BIGREAD, BIGREAD + GUARD), "CMD_READ big: wrote beyond the buffer\n");
    for (i = 0; i < BIGREAD; i += 16 * SECTOR)
    {
        ULONG chunk = (BIGREAD - i) < 16 * SECTOR ? (BIGREAD - i) : 16 * SECTOR;

        err = tdcmd(CMD_READ, b + i, chunk, 16 * SECTOR + i);
        CHECK(err == 0, "CMD_READ chunk at %lu: error %ld\n", (unsigned long)i, (long)err);
    }
    CHECK(memcmp(a, b, BIGREAD) == 0, "CMD_READ big: data differs from chunked reads\n");

    /* Same through READ(10) directly */
    LOG("READ(10) %d bytes in one command\n", BIGREAD);
    read10(cdb, 16, BIGREAD / SECTOR);
    fillguard(a, BIGREAD + GUARD);
    err = scsi(cdb, 10, a, BIGREAD, SCSIF_READ);
    CHECK(err == 0 && sc.scsi_Actual == BIGREAD, "READ(10) big: error %ld actual %lu\n", (long)err, (unsigned long)sc.scsi_Actual);
    CHECK(memcmp(a, b, BIGREAD) == 0, "READ(10) big: data differs\n");

    /* Beyond the end of the medium */
    LOG("READ(10) beyond capacity\n");
    read10(cdb, lastlba + 1000, 1);
    err = scsi(cdb, 10, a, SECTOR, SCSIF_READ);
    expect_check("READ(10) beyond capacity", err, SK_ILLEGAL_REQUEST, 0x21);

    LOG("CMD_READ beyond capacity\n");
    err = tdcmd(CMD_READ, a, SECTOR, (lastlba + 1000) * SECTOR);
    CHECK(err != 0, "CMD_READ beyond capacity: no error\n");

    /* Unaligned CMD_READ is refused */
    LOG("CMD_READ unaligned\n");
    err = tdcmd(CMD_READ, a, 512, 16 * SECTOR);
    CHECK(err == IOERR_NOCMD, "CMD_READ unaligned: io_Error %ld\n", (long)err);

    /* Reads still work after the error paths */
    read10(cdb, 16, 1);
    err = scsi(cdb, 10, a, SECTOR, SCSIF_READ);
    CHECK(err == 0 && a[0] == 1 && memcmp(a + 1, "CD001", 5) == 0, "READ(10) after errors failed: %ld\n", (long)err);

out:
    if (a) FreeMem(a, BIGREAD + GUARD);
    if (b) FreeMem(b, BIGREAD + GUARD);
}

#define NASYNC 4

static void test_async(void)
{
    struct IOStdReq *reqs[NASYNC];
    struct SCSICmd cmds[NASYNC];
    UBYTE cdbs[NASYNC][6];
    UBYTE *bufs[NASYNC];
    UBYTE *big = AllocMem(BIGREAD, MEMF_PUBLIC);
    LONG err, i;

    LOG("Asynchronous HD_SCSICMD x%d\n", NASYNC);
    for (i = 0; i < NASYNC; i++)
    {
        reqs[i] = (struct IOStdReq *)CreateIORequest(port, sizeof(struct IOStdReq));
        bufs[i] = AllocMem(64, MEMF_PUBLIC);
        if (!reqs[i] || !bufs[i])
        {
            FAIL("out of memory\n");
            return;
        }
        reqs[i]->io_Device = io->io_Device;
        reqs[i]->io_Unit   = io->io_Unit;
        memset(cdbs[i], 0, 6);
        cdbs[i][0] = SCSI_INQUIRY; cdbs[i][4] = 36;
        memset(&cmds[i], 0, sizeof(cmds[i]));
        cmds[i].scsi_Data      = (UWORD *)bufs[i];
        cmds[i].scsi_Length    = 36;
        cmds[i].scsi_Command   = cdbs[i];
        cmds[i].scsi_CmdLength = 6;
        cmds[i].scsi_Flags     = SCSIF_READ;
        reqs[i]->io_Command = HD_SCSICMD;
        reqs[i]->io_Data    = &cmds[i];
        reqs[i]->io_Length  = sizeof(cmds[i]);
        fillguard(bufs[i], 64);
        SendIO((struct IORequest *)reqs[i]);
    }
    for (i = 0; i < NASYNC; i++)
    {
        err = WaitIO((struct IORequest *)reqs[i]);
        CHECK(err == 0 && cmds[i].scsi_Status == 0 && cmds[i].scsi_Actual == 36,
            "async INQUIRY %ld: error %ld status %d actual %lu\n", (long)i, (long)err, cmds[i].scsi_Status, (unsigned long)cmds[i].scsi_Actual);
        CHECK((bufs[i][0] & 0x1f) == 5 && guardok(bufs[i], 36, 64), "async INQUIRY %ld: bad data\n", (long)i);
    }

    /* AbortIO on a queued request: either aborted while queued or completed normally */
    if (big)
    {
        LOG("SendIO + AbortIO\n");
        reqs[0]->io_Command = CMD_READ;
        reqs[0]->io_Data    = big;
        reqs[0]->io_Length  = BIGREAD;
        reqs[0]->io_Offset  = 16 * SECTOR;
        SendIO((struct IORequest *)reqs[0]);
        reqs[1]->io_Command = CMD_READ;
        reqs[1]->io_Data    = big;
        reqs[1]->io_Length  = BIGREAD;
        reqs[1]->io_Offset  = 16 * SECTOR;
        SendIO((struct IORequest *)reqs[1]);
        AbortIO((struct IORequest *)reqs[1]);
        err = WaitIO((struct IORequest *)reqs[1]);
        CHECK(err == 0 || err == IOERR_ABORTED, "aborted read: io_Error %ld\n", (long)err);
        LOG("  second request %s\n", err == IOERR_ABORTED ? "aborted" : "completed");
        err = WaitIO((struct IORequest *)reqs[0]);
        CHECK(err == 0 && reqs[0]->io_Actual == BIGREAD, "read before abort: error %ld actual %lu\n", (long)err, (unsigned long)reqs[0]->io_Actual);
        FreeMem(big, BIGREAD);
    }

    for (i = 0; i < NASYNC; i++)
    {
        FreeMem(bufs[i], 64);
        DeleteIORequest((struct IORequest *)reqs[i]);
    }
}

static void test_nomedia(void)
{
    UBYTE buf[SECTOR];
    LONG err;

    LOG("No media: TD_CHANGESTATE\n");
    CHECK(!media_present(), "TD_CHANGESTATE reports media present\n");

    LOG("No media: TEST UNIT READY\n");
    memset(cdb, 0, 6);
    err = scsi(cdb, 6, NULL, 0, SCSIF_READ);
    expect_check("TEST UNIT READY without media", err, SK_NOT_READY, 0x3a);

    LOG("No media: CMD_READ\n");
    err = tdcmd(CMD_READ, buf, SECTOR, 16 * SECTOR);
    CHECK(err == TDERR_DiskChanged, "CMD_READ without media: io_Error %ld, expected TDERR_DiskChanged\n", (long)err);

    LOG("No media: READ(10)\n");
    read10(cdb, 16, 1);
    err = scsi(cdb, 10, buf, SECTOR, SCSIF_READ);
    expect_check("READ(10) without media", err, SK_NOT_READY, 0);
    CHECK(sc.scsi_Actual == 0, "READ(10) without media: actual %lu\n", (unsigned long)sc.scsi_Actual);

    LOG("No media: TD_EJECT\n");
    err = tdcmd(TD_EJECT, NULL, 1, 0);
    LOG("  TD_EJECT returned %ld\n", (long)err);
}

static void test_media(void)
{
    ULONG lastlba;

    LOG("Media present: TEST UNIT READY\n");
    memset(cdb, 0, 6);
    (void)scsi(cdb, 6, NULL, 0, SCSIF_READ);
    if (sc.scsi_Status != 0 && (sense[2] & 0x0f) == SK_UNIT_ATTENTION)
    {
        LOG("  unit attention, retrying\n");
        (void)scsi(cdb, 6, NULL, 0, SCSIF_READ);
    }
    CHECK(sc.scsi_Status == 0, "TEST UNIT READY: status %d sense %x/%02x\n", sc.scsi_Status, sense[2] & 0x0f, sense[12]);

    test_inquiry();
    test_requestsense();
    test_illegal();
    lastlba = test_capacity();
    if (lastlba > 16)
        test_read(lastlba);
    test_async();
}

static void test_verifyfile(STRPTR name)
{
    BPTR fh;
    UBYTE *buf = AllocMem(65536, MEMF_PUBLIC);
    ULONG pos = 0, bad = 0;
    LONG got;

    LOG("Verify file %s through the filesystem\n", name);
    if (!buf)
        return;
    fh = Open(name, MODE_OLDFILE);
    CHECK(fh != BNULL, "cannot open %s (error %ld)\n", name, (long)IoErr());
    if (fh)
    {
        while ((got = Read(fh, buf, 65536)) > 0)
        {
            LONG i;

            for (i = 0; i < got; i++, pos++)
                if (buf[i] != (UBYTE)((pos * 7 + 3) & 0xff))
                    bad++;
        }
        CHECK(got == 0, "read error %ld\n", (long)IoErr());
        Close(fh);
        LOG("  %lu bytes, %lu bad\n", (unsigned long)pos, (unsigned long)bad);
        CHECK(pos > 0 && bad == 0, "file content mismatch: %lu bad bytes of %lu\n", (unsigned long)bad, (unsigned long)pos);
    }
    FreeMem(buf, 65536);
}

static LONG find_unit(STRPTR device)
{
    LONG unit;

    for (unit = 0; unit < 8; unit++)
    {
        struct DriveGeometry dg;

        if (OpenDevice(device, unit, (struct IORequest *)io, 0) != 0)
            continue;
        if (tdcmd(TD_GETGEOMETRY, &dg, sizeof(dg), 0) == 0 && dg.dg_DeviceType == DG_CDROM)
        {
            CloseDevice((struct IORequest *)io);
            return unit;
        }
        CloseDevice((struct IORequest *)io);
    }
    return -1;
}

int main(void)
{
    struct Args args = { NULL, NULL, FALSE, FALSE, NULL, NULL };
    struct RDArgs *rda;
    STRPTR device = "ata.device";
    LONG unit = -1;
    struct DriveGeometry dg;
    struct Interrupt changeint;
    struct IOStdReq *changeio = NULL;
    LONG err;
    int rc = RETURN_OK;

    rda = ReadArgs(TEMPLATE, (IPTR *)&args, NULL);
    if (!rda)
    {
        PrintFault(IoErr(), "atapitest");
        return RETURN_FAIL;
    }
    if (args.device)
        device = args.device;
    if (args.unit)
        unit = *args.unit;
    verbose = args.verbose ? TRUE : FALSE;

    port = CreateMsgPort();
    io = (struct IOStdReq *)CreateIORequest(port, sizeof(struct IOStdReq));
    if (!port || !io)
    {
        printf("atapitest: out of memory\n");
        FreeArgs(rda);
        return RETURN_FAIL;
    }

    if (unit < 0)
        unit = find_unit(device);
    if (unit < 0)
    {
        printf("atapitest: no ATAPI CD-ROM unit found on %s\n", device);
        rc = RETURN_WARN;
        goto out;
    }

    if ((err = OpenDevice(device, unit, (struct IORequest *)io, 0)) != 0)
    {
        printf("atapitest: cannot open %s unit %ld (error %ld)\n", device, (long)unit, (long)err);
        rc = RETURN_FAIL;
        goto out;
    }
    LOG("%s unit %ld opened\n", device, (long)unit);

    /* Geometry and identity */
    err = tdcmd(TD_GETGEOMETRY, &dg, sizeof(dg), 0);
    CHECK(err == 0, "TD_GETGEOMETRY: error %ld\n", (long)err);
    LOG("TD_GETGEOMETRY: type %d, sector size %lu, sectors %lu, flags %lx\n", dg.dg_DeviceType,
        (unsigned long)dg.dg_SectorSize, (unsigned long)dg.dg_TotalSectors, (unsigned long)dg.dg_Flags);
    CHECK(dg.dg_DeviceType == DG_CDROM, "TD_GETGEOMETRY: device type %d, expected DG_CDROM\n", dg.dg_DeviceType);
    CHECK(dg.dg_SectorSize == SECTOR, "TD_GETGEOMETRY: sector size %lu\n", (unsigned long)dg.dg_SectorSize);
    CHECK(dg.dg_Flags & DGF_REMOVABLE, "TD_GETGEOMETRY: not flagged removable\n");

    err = tdcmd(TD_GETDRIVETYPE, NULL, 0, 0);
    CHECK(err == 0 && io->io_Actual == DRIVE_NEWSTYLE, "TD_GETDRIVETYPE: %ld / %lu\n", (long)err, (unsigned long)io->io_Actual);

    /* Media change interrupt */
    changeio = (struct IOStdReq *)CreateIORequest(port, sizeof(struct IOStdReq));
    if (changeio)
    {
        changeio->io_Device = io->io_Device;
        changeio->io_Unit   = io->io_Unit;
        changeint.is_Node.ln_Type = NT_INTERRUPT;
        changeint.is_Node.ln_Name = "atapitest changeint";
        changeint.is_Data = (APTR)&changeints;
        changeint.is_Code = (VOID_FUNC)ChangeInt;
        changeio->io_Command = TD_ADDCHANGEINT;
        changeio->io_Data    = &changeint;
        changeio->io_Length  = sizeof(changeint);
        SendIO((struct IORequest *)changeio);
    }

    tdcmd(TD_CHANGENUM, NULL, 0, 0);
    LOG("TD_CHANGENUM %lu, media %s\n", (unsigned long)io->io_Actual, media_present() ? "present" : "absent");

    if (args.waitmedia)
    {
        ULONG changenum, secs = *args.waitmedia;

        tdcmd(TD_CHANGENUM, NULL, 0, 0);
        changenum = io->io_Actual;
        LOG("Waiting up to %lu seconds for media\n", (unsigned long)secs);
        while (secs-- && !media_present())
            Delay(50);
        CHECK(media_present(), "no media appeared\n");
        tdcmd(TD_CHANGENUM, NULL, 0, 0);
        LOG("TD_CHANGENUM %lu -> %lu, change interrupts %lu\n", (unsigned long)changenum, (unsigned long)io->io_Actual, (unsigned long)changeints);
        CHECK(io->io_Actual != changenum, "TD_CHANGENUM did not change\n");
        CHECK(changeints != 0, "TD_ADDCHANGEINT interrupt was not raised\n");
        if (media_present())
        {
            test_media();
            if (args.verifyfile)
            {
                /* give the filesystem a moment to mount the volume */
                LONG tries = 30;
                BPTR lock;

                while (tries-- && !(lock = Lock(args.verifyfile, ACCESS_READ)))
                    Delay(50);
                if (lock)
                    UnLock(lock);
                test_verifyfile(args.verifyfile);
            }
        }
    }
    else if (args.nomedia)
    {
        test_nomedia();
    }
    else if (media_present())
    {
        test_media();
    }
    else
    {
        LOG("no media, running no-media checks\n");
        test_nomedia();
    }

    if (changeio)
    {
        changeio->io_Command = TD_REMCHANGEINT;
        changeio->io_Data    = &changeint;
        changeio->io_Length  = sizeof(changeint);
        DoIO((struct IORequest *)changeio);
        DeleteIORequest((struct IORequest *)changeio);
    }

    CloseDevice((struct IORequest *)io);

    LOG("%ld checks, %ld failures, %ld warnings\n", (long)checks, (long)failures, (long)warnings);
    if (failures)
    {
        printf("atapitest: %ld of %ld checks failed\n", (long)failures, (long)checks);
        rc = RETURN_ERROR;
    }

out:
    if (io)
        DeleteIORequest((struct IORequest *)io);
    if (port)
        DeleteMsgPort(port);
    FreeArgs(rda);
    return rc;
}
