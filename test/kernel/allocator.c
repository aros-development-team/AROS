/*
 * WARNING!!!
 * This test relies on some particular internal functioning of kernel memory allocator!
 * You'll have to change the code if you change something in kernel!
 *
 * Note also that this test requires working exec trap handling.
 */

/*
 * We will not perform access tests because when we exit from trap handler,
 * execution continues from the same location (re-triggering the trap again).
 * Fixing this requires adjusting PC register in the context, which is not
 * portable accross various CPUs and additionally will not currently work
 * on all ports. We don't want to overcomplicate things so much.
 */
#define NO_ACCESS_TESTS

#include <aros/kernel.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <proto/kernel.h>

#include <stdio.h>

/* Include private kernel.resource stuff, for DumpState() */
struct KernelBase;
#include "../../rom/kernel/mm_linear.h"

#define PAGES_NUM 14

volatile static ULONG trap;

static void TrapHandler(ULONG code, void *ctx)
{
    trap = code;
}

static void TestRead(UBYTE *location)
{
#ifdef NO_ACCESS_TESTS
    printf("Access tests disabled\n");
#else

    volatile UBYTE val;

    /* Reset trap indication */
    trap = -1;

    printf("Test read from 0x%p... ", location);
    val = *location;

    /*
     * We can't printf() from trap handler. Instead we just
     * remember trap code and check it later here.
     */
    if (trap == -1)
	printf("Success, value is 0x%02X\n", val);
    else
	printf("Hit trap 0x%08X\n", trap);

#endif
}

/*
 * Print status of all pages in the specified MemHeader.
 * '#' means allocated page, '.' means free page
 */
static void DumpState(struct MemHeader *mh)
{
    struct BlockHeader *head = (struct BlockHeader *)mh->mh_First;
    IPTR p;

    printf("Page map (%u total):\n", (unsigned)head->size);

    for (p = 0; p < head->size; p++)
        printf(P_STATUS(head->map[p]) ? "#" : ".");

    printf("\n");
}

int main(void)
{
#if defined(KrnStatMemory)

    APTR KernelBase;
    struct Task *me;
    ULONG page;
    struct MemHeader *TestArea;
    ULONG TestLength;
    struct MemChunk *mc;
    APTR region1, region2, region3, region4;

    KernelBase = OpenResource("kernel.resource");
    if (!KernelBase)
    {
	printf("Failed to open kernel.resource!\n");
	return 1;
    }

    if (!KrnStatMemory(0, KMS_PageSize, &page, TAG_DONE))
    {
	printf("MMU support is not implemented for this system!\n"
	       "kernel.resource memory allocator will not work!\n");
	return 1;
    }
    printf("System page size: %u (0x%08X)\n", (unsigned)page, (unsigned)page);

    TestLength = PAGES_NUM * page;
    TestArea = AllocMem(TestLength, MEMF_ANY);
    printf("Allocated test region (%u bytes) at 0x%p\n", (unsigned)TestLength, TestArea);
    if (!TestArea)
    {
	printf("Failed to allocate test region!\n");
	return 1;
    }

    /* Install trap handler */
    me = FindTask(NULL);
    me->tc_TrapCode = TrapHandler;

    /* Compose a MemHeader */
    TestArea->mh_Node.ln_Succ = NULL;
    TestArea->mh_Node.ln_Type = NT_MEMORY;
    TestArea->mh_Node.ln_Name = "Kernel allocator test area";
    TestArea->mh_Node.ln_Pri  = 127;			/* This MemHeader must be the first in the list, otherwise KrnFreePages() will find a wrong one */
    TestArea->mh_Attributes   = MEMF_FAST;
    TestArea->mh_Lower        = TestArea;
    TestArea->mh_Upper        = TestArea->mh_Lower + TestLength - 1;
    TestArea->mh_First        = TestArea->mh_Lower + MEMHEADER_TOTAL;
    TestArea->mh_Free         = TestLength - MEMHEADER_TOTAL;

    mc = TestArea->mh_First;
    mc->mc_Next  = NULL;
    mc->mc_Bytes = TestArea->mh_Free;

    /* Give up the area to kernel allocator */
    KrnInitMemory(TestArea);
    if (mc->mc_Next || mc->mc_Bytes)
    {
	printf("KrnInitMemory() failed:\n"
	       "  mc_Next  is 0x%p\n"
	       "  mc_Bytes is %lu\n",
	       mc->mc_Next, mc->mc_Bytes);
	goto exit;
    }

    printf("Testing initial no-access protection...\n");
    TestRead((UBYTE *)TestArea + page);

    /*
     * Insert the area into system list.
     * We do it manually because in future AddMemList() will call KrnInitMemory() itself.
     */
    Forbid();
    Enqueue(&SysBase->MemList, &TestArea->mh_Node);
    Permit();

    printf("Allocating region1 (two read-write pages)...\n");
    region1 = KrnAllocPages(NULL, 2 * page, MEMF_FAST);
    printf("region1 at 0x%p\n", region1);
    DumpState(TestArea);

    printf("Freeing region1...\n");
    KrnFreePages(region1, 2 * page);
    printf("Done!\n");
    DumpState(TestArea);

    printf("Allocating region1 (3 read-only pages)...\n");
    region1 = KrnAllocPages(NULL, 3 * page, MEMF_FAST);
    printf("region1 at 0x%p\n", region1);
    DumpState(TestArea);

    printf("Allocating region2 (4 write-only ;-) pages)...\n");
    region2 = KrnAllocPages(NULL, 4 * page, MEMF_FAST);
    printf("region2 at 0x%p\n", region2);
    DumpState(TestArea);

    printf("Attempting to allocate page with wrong flags...\n");
    region3 = KrnAllocPages(NULL, page, MEMF_CHIP|MEMF_FAST);
    printf("Region at 0x%p\n", region3);
    if (region3)
    {
	printf("WARNING!!! This should have been NULL!\n");
	KrnFreePages(region3, page);
    }

    printf("Freeing region1...\n");
    KrnFreePages(region1, 3 * page);
    printf("Done!\n");
    DumpState(TestArea);

    printf("Freeing region2...\n");
    KrnFreePages(region2, 4 * page);
    printf("Done!\n");
    DumpState(TestArea);

    printf("Allocating region1 (one read-write page)...\n");
    region1 = KrnAllocPages(NULL, page, MEMF_FAST);
    printf("region1 at 0x%p\n", region1);
    DumpState(TestArea);

    printf("Freeing region1...\n");
    KrnFreePages(region1, page);
    printf("Done!\n");
    DumpState(TestArea);

    printf("Now we'll try to fragment the memory\n");

    printf("Allocating region1 (2 pages)...\n");
    region1 = KrnAllocPages(NULL, 2 * page, MEMF_FAST);
    printf("region1 at 0x%p\n", region1);
    DumpState(TestArea);

    printf("Allocating region2 (3 pages)...\n");
    region2 = KrnAllocPages(NULL, 3 * page, MEMF_FAST);
    printf("region2 at 0x%p\n", region2);
    DumpState(TestArea);

    printf("Allocating region 3 (1 page)...\n");
    region3 = KrnAllocPages(NULL, page, MEMF_FAST);
    printf("Region at 0x%p\n", region3);
    DumpState(TestArea);
    
    printf("Allocating region 4 (2 pages)...\n");
    region4 = KrnAllocPages(NULL, 2 * page, MEMF_FAST);
    printf("region4 at 0x%p\n", region1);
    DumpState(TestArea);
    
    printf("Freeing region1...\n");
    KrnFreePages(region1, 2 * page);
    printf("Done!\n");
    DumpState(TestArea);

    printf("Freeing region3...\n");
    KrnFreePages(region3, page);
    printf("Done!\n");
    DumpState(TestArea);

    printf("Allocating region 3 (1 page) again...\n");
    region3 = KrnAllocPages(NULL, page, MEMF_FAST);
    printf("Region at 0x%p\n", region3);
    DumpState(TestArea);

    printf("Freeing region2...\n");
    KrnFreePages(region2, 3 * page);
    printf("Done!\n");
    DumpState(TestArea);
    
    printf("Freeing region3...\n");
    KrnFreePages(region3, page);
    printf("Done!\n");
    DumpState(TestArea);

    printf("Freeing region4...\n");
    KrnFreePages(region4, 2 * page);
    printf("Done!\n");
    DumpState(TestArea);

exit:
    if (TestArea->mh_Node.ln_Succ)
    {
	Forbid();
	Remove(&TestArea->mh_Node);
	Permit();
    }
    FreeMem(TestArea, TestLength);

#else
    printf("The test can't be built for this kernel.resource implementation\n");
#endif

    return 0;
}
