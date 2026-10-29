/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

typedef void *APTR;
typedef uint8_t UBYTE;
typedef uint32_t ULONG;
typedef uintptr_t IPTR;
#define TRUE 1
#define FALSE 0
#define TS_WAIT 4
#define MEM_TRY_AGAIN 1
#define MEM_DID_NOTHING 0
#define MEMF_PUBLIC 1u
#define MEMF_CHIP 2u
#define MEMF_NO_EXPUNGE (1u << 31)
#define SCSI_BUFSIZE 2048
#define AROS_INTH1(name,type,arg) int name(type arg)
#define AROS_INTFUNC_INIT
#define AROS_INTFUNC_EXIT
#define D(x) do {} while (0)
struct FakeProcess { struct { int tc_State; } pr_Task; };
struct FakeVolume { uint64_t read_end; };
struct CDVDBase { struct FakeProcess *DosProc; struct FakeVolume *g_volume; };
typedef struct SectorCacheSlot {
    long current_sector;
    uint32_t last_used;
    unsigned short buffer_sector, sector_count, valid_sectors;
} SectorCacheSlot;
typedef struct CDROM {
    struct CDVDBase *global;
    UBYTE *buffer;
    uint32_t buffer_bytes;
    UBYTE *buffer_data, *buffer_file2, *buffer_metadata, *buffer_io;
    long io_sector;
    unsigned short cache_io_busy, split_cache, cache_reclaimed;
    short buffers_cnt;
    unsigned short buffer_sectors, file_slots;
    SectorCacheSlot *cache_slots;
    ULONG cache_memory_type;
} CDROM;
static unsigned allocations_left, io_count, live_allocations, bad_flags;
static int (*allocation_interrupt)(CDROM *);
static CDROM *allocation_cd;
static unsigned recovery_reclaimed;
static APTR AllocVec(size_t n, ULONG flags)
{
    if (flags != (MEMF_PUBLIC | MEMF_CHIP | MEMF_NO_EXPUNGE)) bad_flags++;
    if (allocation_interrupt && allocation_interrupt(allocation_cd) != MEM_DID_NOTHING)
        recovery_reclaimed++;
    if (!allocations_left) return NULL;
    void *p = malloc(n);
    if (p) { allocations_left--; live_allocations++; }
    return p;
}
static void FreeVec(APTR p)
{
    if (p) { free(p); live_allocations--; }
}
static UBYTE expected(long sector, unsigned offset)
{
    return (UBYTE)(sector * 17 + offset * 3);
}
static int Read_From_Drive(CDROM *cd, UBYTE *buf, long n, long sector, int count)
{
    (void)cd; (void)n;
    io_count++;
    for (int i = 0; i < count * SCSI_BUFSIZE; i++)
        buf[i] = expected(sector + i / SCSI_BUFSIZE, i % SCSI_BUFSIZE);
    return 1;
}
void Clear_Sector_Buffers(CDROM *cd);
