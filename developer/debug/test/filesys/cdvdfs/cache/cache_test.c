/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */
static unsigned checks, failures;
static void check(int okay, const char *message)
{
    checks++;
    if (!okay) { failures++; fprintf(stderr, "FAIL: %s\n", message); }
}
struct Fixture {
    CDROM cd;
    struct FakeProcess process;
    struct FakeVolume volume;
    struct CDVDBase global;
    SectorCacheSlot slots[14];
    UBYTE fallback[SCSI_BUFSIZE];
};
static void setup(struct Fixture *f, unsigned budget)
{
    memset(f, 0, sizeof(*f));
    f->process.pr_Task.tc_State = TS_WAIT;
    f->volume.read_end = 4096;
    f->global.DosProc = &f->process;
    f->global.g_volume = &f->volume;
    f->cd.global = &f->global;
    f->cd.cache_slots = f->slots;
    f->cd.buffer_io = f->fallback;
    f->cd.io_sector = -1;
    f->cd.buffers_cnt = 14;
    f->cd.buffer_sectors = 1;
    f->cd.split_cache = 1;
    f->cd.cache_reclaimed = 2;
    f->cd.cache_memory_type = MEMF_PUBLIC | MEMF_CHIP;
    for (int i = 0; i < 14; i++) {
        f->slots[i].current_sector = -1;
        f->slots[i].sector_count = i < 2 ? 2 : 1;
        f->slots[i].buffer_sector = i < 2 ? 0 : i - 2;
    }
    allocations_left = budget;
    io_count = bad_flags = 0;
}
static void cleanup(struct Fixture *f)
{
    FreeVec(f->cd.buffer_data);
    FreeVec(f->cd.buffer_file2);
    check(!live_allocations, "all optional cache allocations are released");
}
static int read_sector(struct Fixture *f, long sector, int file)
{
    if (!Read_Chunk_Internal(&f->cd, sector, file)) return 0;
    if (f->cd.buffer_bytes < SCSI_BUFSIZE) return 0;
    for (unsigned i = 0; i < SCSI_BUFSIZE; i++)
        if (f->cd.buffer[i] != expected(sector, i)) return 0;
    return 1;
}
int main(void)
{
    struct Fixture f;
    int okay = 1;
    setup(&f, 2);
    for (unsigned i = 0; i < 1000; i++)
        okay &= read_sector(&f, i & 1 ? 2119 : 2040, 1);
    check(okay, "alternating UFO-pattern reads return correct bytes");
    check(io_count == 2, "recovered cache avoids repeated physical seeks");
    check(f.cd.file_slots == 2, "both small file slots become usable again");
    check(!bad_flags, "recovery preserves DMA flags and prohibits expunging");
    check(read_sector(&f, 22, 0), "uncached metadata still reads correctly");
    unsigned old = io_count;
    check(read_sector(&f, 2040, 1) && io_count == old,
          "metadata reads do not evict the recovered file windows");
    f.cd.cache_io_busy = 1;
    check(CDCacheLowMemHandler(&f.cd) == MEM_DID_NOTHING,
          "in-flight I/O protects its buffers from reclamation");
    f.cd.cache_io_busy = 0;
    check(CDCacheLowMemHandler(&f.cd) == MEM_TRY_AGAIN &&
          !f.cd.buffer_file2 && f.cd.buffer_data,
          "recovered secondary cache remains reclaimable");
    check(CDCacheLowMemHandler(&f.cd) == MEM_TRY_AGAIN &&
          !f.cd.buffer_data, "recovered primary cache remains reclaimable");
    check(CDCacheLowMemHandler(&f.cd) == MEM_DID_NOTHING,
          "fully reclaimed cache stops retrying memory handlers");
    allocations_left = 2;
    old = io_count;
    check(read_sector(&f, 2119, 1) && read_sector(&f, 2040, 1) &&
          read_sector(&f, 2119, 1) && io_count == old + 2,
          "cache recovers after a second round of memory pressure");
    cleanup(&f);

    setup(&f, 0);
    check(read_sector(&f, 2040, 1), "allocation failure preserves fallback reads");
    old = io_count;
    check(read_sector(&f, 2040, 1) && io_count == old,
          "repeated fallback sector remains cached without free memory");
    check(!f.cd.buffer_data && !f.cd.buffer_file2 && !bad_flags,
          "failed recovery retains the no-expunge fallback");
    cleanup(&f);

    setup(&f, 1);
    check(read_sector(&f, 2040, 1) && f.cd.file_slots == 1,
          "partial recovery provides a usable primary slot");
    old = io_count;
    allocations_left = 1;
    check(read_sector(&f, 2119, 1) && f.cd.file_slots == 2,
          "secondary slot can recover later");
    check(read_sector(&f, 2040, 1) && io_count == old + 1,
          "secondary recovery preserves valid primary data");
    IO_Buffer(&f.cd);
    check(f.cd.io_sector == -1, "SCSI control use invalidates the fallback tag");
    cleanup(&f);

    setup(&f, 1);
    check(read_sector(&f, 2040, 1), "prepare primary cache before allocator wait");
    UBYTE *primary = f.cd.buffer_data;
    allocation_interrupt = CDCacheLowMemHandler;
    allocation_cd = &f.cd;
    allocations_left = 1;
    recovery_reclaimed = 0;
    check(read_sector(&f, 2119, 1) && !recovery_reclaimed &&
          primary && f.cd.buffer_data == primary,
          "concurrent memory pressure cannot reclaim a cache during recovery");
    check(!f.cd.cache_io_busy, "recovery releases its reclamation guard");
    allocation_interrupt = NULL;
    cleanup(&f);

    setup(&f, 2);
    f.cd.split_cache = 0;
    check(read_sector(&f, 2040, 1) && !f.cd.buffer_data && !live_allocations,
          "non-split configurations keep their original fallback path");
    cleanup(&f);
    printf("CDVDFS CACHE: %u/%u pass, %u failures (pointer size %zu)\n",
           checks - failures, checks, failures, sizeof(IPTR));
    return failures ? 20 : 0;
}
