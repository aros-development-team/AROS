/* Copyright (C) 2026, The AROS Development Team. All rights reserved.
   Actual production handlers/accessors over a recording, standalone C
   fixture. No AROS SDK, application, CPU core or C++ runtime is required. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef uint8_t UBYTE;
typedef uint32_t ULONG;
typedef uintptr_t IPTR;
typedef int BOOL;
#define TRUE 1
#define FALSE 0
struct M68KEmuContext { UBYTE *mem; ULONG mem_size; BOOL low_guard; };
struct TestCPU { ULONG d[8], a[8]; };
#define THUNK_D(n) (((struct TestCPU *)cpu)->d[n])
#define THUNK_A(n) (((struct TestCPU *)cpu)->a[n])
static unsigned dropped;
static void m68k_low_write_trap(struct M68KEmuContext *ctx, ULONG addr, const char *who)
{ (void)ctx; (void)addr; (void)who; ++dropped; }
#include "constants.inc"
#define m68k_read32 production_read32
#include "accessors.inc"
#undef m68k_read32
static unsigned read32_calls;
static ULONG m68k_read32(struct M68KEmuContext *ctx, ULONG addr)
{ ++read32_calls; return production_read32(ctx, addr); }
#include "memheader.inc"

#define ARENA 16384u
#define MH 2048u
#define POOL 4096u
#define SLOTS 256u
static UBYTE arena[ARENA], saved[ARENA], free_slots[SLOTS];
static struct M68KEmuContext context;
static unsigned checks, failures;
static void check(int ok, const char *label)
{
    ++checks;
    if (!ok) { if (failures < 24) printf("FAIL: %s\n", label); ++failures; }
}
static void put(ULONG at, ULONG value)
{
    assert((uint64_t)at + 4 <= ARENA);
    arena[at] = (UBYTE)(value >> 24); arena[at+1] = (UBYTE)(value >> 16);
    arena[at+2] = (UBYTE)(value >> 8); arena[at+3] = (UBYTE)value;
}
static ULONG get(ULONG at)
{
    assert((uint64_t)at + 4 <= ARENA);
    return (ULONG)arena[at] << 24 | (ULONG)arena[at+1] << 16 | (ULONG)arena[at+2] << 8 | arena[at+3];
}
static void chunk(ULONG at, ULONG next, ULONG bytes) { put(at, next); put(at+4, bytes); }
static void header(ULONG at, ULONG first, ULONG bytes)
{
    memset(arena+at, 0, 32); put(at+16, first); put(at+28, bytes);
    /* These fields are deliberately not a usable pool bound. */
    put(at+20, 0xDEADBEEFu); put(at+24, 3);
}
static void reset(void)
{
    memset(arena, 0xA5, sizeof(arena)); context.mem=arena; context.mem_size=ARENA; context.low_guard=FALSE;
    header(MH, POOL, 128); chunk(POOL, 0, 128); dropped=0;
}
static ULONG allocate(ULONG mh, ULONG size)
{
    struct TestCPU cpu = {{0},{0}}; cpu.a[0]=mh; cpu.d[0]=size;
    return (ULONG)thunk_exec_Allocate_m(&context, &cpu);
}
static void deallocate(ULONG mh, ULONG block, ULONG size)
{
    struct TestCPU cpu = {{0},{0}}; cpu.a[0]=mh; cpu.a[1]=block; cpu.d[0]=size;
    (void)thunk_exec_Deallocate_m(&context, &cpu);
}
static void refusal(ULONG mh, ULONG block, ULONG size, int freeing, const char *label)
{
    ULONG result=0; memcpy(saved, arena, ARENA); dropped=0;
    if (freeing) deallocate(mh,block,size); else result=allocate(mh,size);
    check(!result && !memcmp(saved,arena,ARENA) && !dropped,label);
}
static void reject_both(const char *label)
{
    refusal(MH,0,8,0,label); refusal(MH,POOL-32,16,1,label);
}
static void positives(void)
{
    const ULONG sizes[]={1,2,3,4,5,7,8,9,15,16,17,63,64,65,120,121,128,129};
    unsigned i;
    for(i=0;i<sizeof(sizes)/sizeof(sizes[0]);i++) {
        ULONG n=sizes[i],rounded=(ULONG)(((uint64_t)n+7)/8*8),p;
        reset(); memcpy(saved,arena,ARENA); p=allocate(MH,n);
        if(rounded>128) {check(!p&&!memcmp(saved,arena,ARENA),"ordinary OOM is unchanged");continue;}
        check(p==POOL&&get(MH+28)==128-rounded,"first-fit rounds bytes to eight");
        check(get(MH+16)==(rounded==128?0:POOL+rounded),"exact removal or split link");
        deallocate(MH,p,n);
        check(get(MH+16)==POOL&&get(MH+28)==128&&get(POOL)==0&&get(POOL+4)==128,"allocation/free restores the chunk");
        check(!memcmp(saved,arena,MH+16)&&!memcmp(saved+MH+32,arena+MH+32,POOL-MH-32)&&
              !memcmp(saved+POOL+128,arena+POOL+128,ARENA-POOL-128),"metadata leaves unrelated bytes unchanged");
    }
    reset(); refusal(MH,0,0,0,"Allocate zero is a no-op");refusal(MH,POOL,0,1,"Deallocate zero is a no-op");
    refusal(MH,0,8,1,"Deallocate NULL is a no-op");refusal(0,0,8,0,"Allocate NULL header is refused");
    for(i=0;i<8;i++) {
        reset();header(MH,0,0);deallocate(MH,POOL+i,16-i);
        check(get(MH+16)==POOL&&get(POOL+4)==16&&get(MH+28)==16,"unaligned free rounds outward without losing bytes");
    }
    reset();header(MH,POOL,32);chunk(POOL,POOL+32,16);chunk(POOL+32,0,16);
    deallocate(MH,POOL+16,16);
    check(get(POOL)==0&&get(POOL+4)==48&&get(MH+28)==48,"free coalesces both neighbors");
    reset();header(MH,POOL,32);chunk(POOL,POOL+16,16);chunk(POOL+16,0,16);
    check(allocate(MH,24)==0&&get(MH+28)==32,"adjacent input chunks remain valid without inventing a larger allocation");
    reset();header(MH,ARENA-8,8);chunk(ARENA-8,0,8);
    check(allocate(MH,8)==ARENA-8&&get(MH+16)==0,"last complete chunk at arena end is usable");
    deallocate(MH,ARENA-8,8);check(get(MH+28)==8&&get(MH+16)==ARENA-8,"last complete free span is usable");
    reset();header(ARENA-32,POOL,16);chunk(POOL,0,16);
    check(allocate(ARENA-32,8)==POOL,"last complete classic MemHeader is usable");
    reset();header(MH,POOL,16);chunk(POOL,0,16);put(MH+20,0);put(MH+24,0);
    check(allocate(MH,16)==POOL,"private header with unset lower/upper bounds works");
}
static void invalids(void)
{
    const ULONG bad_sizes[]={0xFFFFFFF9u,0xFFFFFFFAu,0xFFFFFFFBu,0xFFFFFFFCu,0xFFFFFFFDu,0xFFFFFFFEu,0xFFFFFFFFu};
    const ULONG bad_headers[]={ARENA-31,ARENA-20,ARENA-1,ARENA,0xFFFFFFE0u,0xFFFFFFF0u};
    unsigned i;
    for(i=0;i<sizeof(bad_sizes)/sizeof(bad_sizes[0]);i++) {
        reset();refusal(MH,0,bad_sizes[i],0,"allocation rounding overflow is atomic");
        reset();header(MH,0,0);refusal(MH,POOL+7,bad_sizes[i],1,"free rounding overflow is atomic");
    }
    for(i=0;i<sizeof(bad_headers)/sizeof(bad_headers[0]);i++) {
        reset();refusal(bad_headers[i],0,8,0,"partial/wrapped header is refused");
        reset();refusal(bad_headers[i],POOL+256,8,1,"bad free header cannot mutate a valid block");
    }
    reset();header(MH,ARENA-8,16);chunk(ARENA-8,0,16);reject_both("whole chunk must fit in the arena");
    reset();header(MH,0,0);refusal(MH,ARENA-8,16,1,"whole free span must fit");
    reset();header(MH,0,0);refusal(MH,0xFFFFFFF8u,16,1,"wrapped free span is refused");
    reset();header(MH,POOL+4,16);chunk(POOL+4,0,16);reject_both("unaligned chunk is refused");
    for(i=0;i<4;i++) {
        const ULONG bytes[]={0,4,9,0xFFFFFFF8u};reset();header(MH,POOL,bytes[i]);chunk(POOL,0,bytes[i]);reject_both("invalid chunk size is refused");
    }
    reset();header(MH,0xFFFFFFF8u,8);reject_both("out-of-arena chunk pointer is refused");
    reset();header(MH,POOL,32);chunk(POOL,POOL,32);reject_both("self-cycle is refused");
    reset();header(MH,POOL,64);chunk(POOL,POOL+64,32);chunk(POOL+64,POOL,32);reject_both("cycle after an early fit is refused");
    reset();header(MH,POOL+64,32);chunk(POOL+64,POOL,16);chunk(POOL,0,16);reject_both("descending list is refused");
    reset();header(MH,POOL,96);chunk(POOL,POOL+32,64);chunk(POOL+32,0,32);reject_both("overlapping chunks are refused");
    reset();header(MH,MH-16,64);chunk(MH-16,0,64);reject_both("free chunk cannot contain its MemHeader");
    reset();put(MH+28,120);reject_both("mh_Free smaller than chunk sum is refused");
    reset();header(MH,0,0xFFFFFFFFu);
    refusal(MH,POOL,8,1,"overstated mh_Free must not overflow on free");
    reset();refusal(MH,POOL,8,1,"double free is unchanged");
    reset();refusal(MH,POOL-8,16,1,"free overlapping following chunk is unchanged");
    reset();refusal(MH,POOL+120,16,1,"free overlapping preceding chunk is unchanged");
    reset();header(MH,0,0);refusal(MH,MH,32,1,"free cannot consume MemHeader storage");
    reset();header(MH,POOL,48);chunk(POOL,POOL+64,32);chunk(POOL+64,POOL+64,16);
    refusal(MH,0,8,0,"late corruption prevents an early fitting allocation");
    refusal(MH,POOL+256,8,1,"late corruption prevents an early insertion");
    reset();context.low_guard=TRUE;header(0x120,POOL,32);chunk(POOL,0,32);
    refusal(0x120,0,8,0,"protected header fields cannot be half updated");
    reset();context.low_guard=TRUE;header(0x140,0x180,32);chunk(0x180,0,32);
    refusal(0x140,0,16,0,"protected split tail is refused before any write");
    check(allocate(0x140,32)==0x180,"permitted low-vector metadata is not rejected wholesale");
    reset();context.low_guard=TRUE;header(0x140,0,0);
    refusal(0x140,0x190,8,1,"protected free header is refused before publication");
}
/* The oracle stores one free/allocated bit per eight-byte slot, without
   using production links, free-byte counters or rounding expressions. */
static int oracle_matches(void)
{
    ULONG node=get(MH+16),sum=0;unsigned i=0;
    while(i<SLOTS) {
        unsigned first,count;if(!free_slots[i]){++i;continue;}first=i;
        while(i<SLOTS&&free_slots[i])++i;count=i-first;
        if(node!=POOL+first*8||get(node+4)!=count*8)return 0;
        sum+=count*8;node=get(node);
    }
    return !node&&get(MH+28)==sum;
}
static uint32_t rng=0xC04D68A9u;
static uint32_t random32(void){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
static void random_oracle(void)
{
    struct Live {unsigned first,count;} live[SLOTS];unsigned used=0,step;
    reset();header(MH,POOL,SLOTS*8);chunk(POOL,0,SLOTS*8);memset(free_slots,1,sizeof(free_slots));
    for(step=0;step<12000;step++) {
        if(!used||(used<SLOTS&&(random32()%100)<61)) {
            unsigned n=1+random32()%137,wanted=(unsigned)(((uint64_t)n+7)/8),first=SLOTS,i,j;ULONG got;
            for(i=0;i<SLOTS;i++) {
                for(j=0;j<wanted&&i+j<SLOTS&&free_slots[i+j];j++) {}
                if(j==wanted){first=i;break;}
            }
            got=allocate(MH,n);check(got==(first==SLOTS?0:POOL+first*8),"random allocation follows independent first-fit bitmap");
            if(got&&first!=SLOTS){for(i=0;i<wanted;i++)free_slots[first+i]=0;live[used].first=first;live[used++].count=wanted;}
        } else {
            unsigned at=random32()%used,first=live[at].first,count=live[at].count,off=random32()%8,i;
            deallocate(MH,POOL+first*8+off,count*8-off);
            for(i=0;i<count;i++)free_slots[first+i]=1;live[at]=live[--used];
        }
        check(oracle_matches(),"random operation preserves independent free intervals and exact count");
        if(!(step%97)) {
            ULONG bytes=get(MH+28);
            if(bytes < 8) continue;
            put(MH+28,bytes-8);
            refusal(MH,0,8,0,"random understated accounting rejects atomically");
            /* Keep later positive oracle operations independent even when
               this negative case exposes a mutation in the old handlers. */
            memcpy(arena,saved,ARENA);put(MH+28,bytes);
        }
    }
}
static void physical_list_bound(void)
{
    const ULONG count=1000001,first=4096,extent=first+count*16;ULONG i,p;
    struct M68KEmuContext ctx;struct TestCPU cpu={{0},{0}};UBYTE *memory=(UBYTE *)calloc(1,extent);
    check(memory!=NULL,"allocate a list larger than the old artificial node limit");if(!memory)return;
    ctx.mem=memory;ctx.mem_size=extent;ctx.low_guard=FALSE;
    for(i=0;i<count;i++){ULONG at=first+i*16;m68k_write32(&ctx,at,i+1<count?at+16:0);m68k_write32(&ctx,at+4,i+1<count?8:16);}
    m68k_write32(&ctx,128+16,first);m68k_write32(&ctx,128+28,count*8+8);cpu.a[0]=128;cpu.d[0]=16;
    p=(ULONG)thunk_exec_Allocate_m(&ctx,&cpu);
    check(p==first+(count-1)*16,"valid list length is limited by arena capacity rather than one million nodes");
    if(p){cpu.a[1]=p;(void)thunk_exec_Deallocate_m(&ctx,&cpu);check(m68k_read32(&ctx,128+28)==count*8+8,"large-list allocation/free restores accounting");}
    free(memory);
}
static void overstated_free(void)
{
    const ULONG counts[] = {136, 137, 0xFFFFFFFFu};
    unsigned i;

    for (i = 0; i < sizeof(counts) / sizeof(counts[0]); ++i)
    {
        ULONG block;
        reset();
        put(MH + 28, counts[i]);
        block = allocate(MH, 8);
        check(block == POOL && get(MH + 28) == counts[i] - 8,
              "overstated free count permits allocation and preserves surplus");
        if (block)
        {
            deallocate(MH, block, 8);
            check(get(MH + 28) == counts[i] && get(MH + 16) == POOL &&
                  get(POOL + 4) == 128,
                  "overstated free count survives allocation/free");
        }
    }
    reset();
    header(MH, 0, 32);
    refusal(MH, 0, 8, 0, "empty overstated header reports no fitting chunk");
    deallocate(MH, POOL, 16);
    check(get(MH + 28) == 48 && get(MH + 16) == POOL && get(POOL + 4) == 16,
          "empty overstated header accepts a free without losing surplus");
}

static void single_scan(void)
{
    const ULONG count = 1024, first = 4096, extent = first + count * 16 + 16;
    struct M68KEmuContext ctx;
    struct TestCPU cpu = {{0}, {0}};
    UBYTE *memory = (UBYTE *)calloc(1, extent);
    ULONG i, block;
    unsigned reads;

    check(memory != NULL, "allocate fragmented-pool scan fixture");
    if (!memory) return;
    ctx.mem = memory; ctx.mem_size = extent; ctx.low_guard = FALSE;
    for (i = 0; i < count; ++i)
    {
        ULONG at = first + i * 16;
        m68k_write32(&ctx, at, i + 1 < count ? at + 16 : 0);
        m68k_write32(&ctx, at + 4, i + 1 < count ? 8 : 16);
    }
    m68k_write32(&ctx, 128 + 16, first);
    m68k_write32(&ctx, 128 + 28, count * 8 + 8);
    cpu.a[0] = 128; cpu.d[0] = 16;
    read32_calls = 0;
    block = (ULONG)thunk_exec_Allocate_m(&ctx, &cpu);
    reads = read32_calls;
    check(block == first + (count - 1) * 16, "fragmented pool retains first fit");
    check(reads <= 2 * count + 8, "allocation uses a single list traversal");
    if (block)
    {
        cpu.a[1] = block;
        read32_calls = 0;
        (void)thunk_exec_Deallocate_m(&ctx, &cpu);
        reads = read32_calls;
        check(reads <= 2 * (count - 1) + 8, "free uses a single list traversal");
        check(m68k_read32(&ctx, 128 + 28) == count * 8 + 8,
              "fragmented pool restores free accounting");
    }
    free(memory);
}

int main(void)
{
    positives();overstated_free();single_scan();invalids();random_oracle();physical_list_bound();
    printf("classic MemHeader: %u/%u checks passed\n",checks-failures,checks);
    return failures?1:0;
}
