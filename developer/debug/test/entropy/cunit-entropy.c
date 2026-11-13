/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit stress/behaviour tests for entropy.resource.

    The resource is a CSPRNG, so its output is non-deterministic by design;
    these tests therefore check API contracts, statistical quality over large
    volumes of output, and robustness under concurrent load rather than
    fixed known-answer vectors.  (The ChaCha20 core is covered by a separate
    known-answer check.)
*/

#include <exec/types.h>
#include <exec/tasks.h>
#include <dos/dos.h>
#include <resources/entropy.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/entropy.h>
#include <clib/alib_protos.h>

#include <string.h>

#include <CUnit/Basic.h>
#include <CUnit/Automated.h>

struct Library *EntropyBase = NULL;

int init_suite(void)
{
    EntropyBase = OpenResource(ENTROPYNAME);
    return EntropyBase ? 0 : -1;    /* non-zero aborts the suite */
}

int clean_suite(void)
{
    return 0;                        /* resources are never closed */
}

/* --- helpers ------------------------------------------------------------ */

/* Count the set bits in a buffer. */
static ULONG count_bits(const UBYTE *p, ULONG len)
{
    static const UBYTE nib[16] = { 0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4 };
    ULONG bits = 0, i;

    for (i = 0; i < len; i++)
        bits += nib[p[i] & 0xf] + nib[p[i] >> 4];

    return bits;
}

/* Longest run of identical consecutive bytes. */
static ULONG longest_run(const UBYTE *p, ULONG len)
{
    ULONG best = 0, cur = 1, i;

    if (len == 0)
        return 0;
    for (i = 1; i < len; i++)
    {
        if (p[i] == p[i - 1])
            cur++;
        else
            cur = 1;
        if (cur > best)
            best = cur;
    }
    return best;
}

/* --- tests -------------------------------------------------------------- */

void testOPEN(void)
{
    CU_ASSERT_PTR_NOT_NULL_FATAL(EntropyBase);
}

void testINFO(void)
{
    ULONG flags = GetEntropyInfo();

    /* The software collector is always present.  Any hardware source is
       reported only through the neutral EIF_HARDWARE bit (the specific
       instruction, e.g. x86 RDRAND/RDSEED, is intentionally not exposed). */
    CU_ASSERT_TRUE(flags & EIF_SOFTWARE);
    CU_ASSERT_TRUE((flags & ~(EIF_SOFTWARE | EIF_HARDWARE)) == 0);
}

void testAPI(void)
{
    UBYTE buf[32];

    /* NULL buffer is rejected. */
    CU_ASSERT_EQUAL(GetEntropy(NULL, sizeof(buf)), -1);

    /* Zero length is a no-op that succeeds. */
    CU_ASSERT_EQUAL(GetEntropy(buf, 0), 0);

    /* A normal request returns exactly the number of bytes asked for. */
    CU_ASSERT_EQUAL(GetEntropy(buf, sizeof(buf)), (LONG)sizeof(buf));
}

/* GetEntropy() must fill exactly length bytes and not one byte more. */
void testBOUNDS(void)
{
    UBYTE buf[64];
    ULONG i;
    int   changed = 0;

    memset(buf, 0xA5, sizeof(buf));
    CU_ASSERT_EQUAL(GetEntropy(buf, 32), 32);

    for (i = 0; i < 32; i++)
        if (buf[i] != 0xA5)
            changed = 1;
    CU_ASSERT_TRUE(changed);                 /* the region was written */

    for (i = 32; i < sizeof(buf); i++)
        CU_ASSERT_EQUAL(buf[i], 0xA5);       /* nothing past length touched */
}

/* Two successive requests must not return identical blocks. */
void testDISTINCT(void)
{
    UBYTE a[64], b[64];

    CU_ASSERT_EQUAL(GetEntropy(a, sizeof(a)), (LONG)sizeof(a));
    CU_ASSERT_EQUAL(GetEntropy(b, sizeof(b)), (LONG)sizeof(b));
    CU_ASSERT_TRUE(memcmp(a, b, sizeof(a)) != 0);
}

/* A large block must not be a single repeated byte, nor contain an
   implausibly long run of identical bytes. */
void testNOTCONSTANT(void)
{
    UBYTE buf[256];
    ULONG i;
    int   allsame = 1;

    CU_ASSERT_EQUAL(GetEntropy(buf, sizeof(buf)), (LONG)sizeof(buf));

    for (i = 1; i < sizeof(buf); i++)
        if (buf[i] != buf[0])
            allsame = 0;
    CU_ASSERT_FALSE(allsame);

    /* P(run >= 7) across 256 bytes is ~ 256 * 256^-6 ~= 1e-12. */
    CU_ASSERT_TRUE(longest_run(buf, sizeof(buf)) < 7);
}

/*
 * Statistical stress: pull a large volume of output in many small,
 * variably-sized requests, accumulate a byte-value histogram, and check that
 * the distribution is close to uniform (chi-square) with a well-balanced bit
 * count.  The bounds are deliberately loose - they never false-fail a working
 * CSPRNG but catch gross defects (stuck values, bias, truncated output).
 */
void testSTATS(void)
{
    ULONG  hist[256];
    ULONG  i, total = 0, setbits = 0;
    UBYTE  buf[256];
    ULONG  seed = 1;
    double expected, chi2 = 0.0, bitfrac;
    const ULONG ROUNDS = 4096;          /* up to ~1 MB of output */

    memset(hist, 0, sizeof(hist));

    for (i = 0; i < ROUNDS; i++)
    {
        /* Vary the request size in 1..256 to exercise the block boundary. */
        ULONG len = 1 + (seed % 256);
        LONG  got = GetEntropy(buf, len);
        ULONG j;

        CU_ASSERT_EQUAL_FATAL(got, (LONG)len);

        for (j = 0; j < len; j++)
            hist[buf[j]]++;
        total   += len;
        setbits += count_bits(buf, len);

        /* Cheap size PRNG derived from fresh output - keeps sizes irregular. */
        seed = seed * 1103515245u + 12345u + buf[0];
    }

    CU_ASSERT_TRUE(total > 0);

    /* Chi-square over the 256 byte values (df = 255). */
    expected = (double)total / 256.0;
    for (i = 0; i < 256; i++)
    {
        double d = (double)hist[i] - expected;
        chi2 += (d * d) / expected;
    }
    /* Mean 255, sigma ~= 22.6.  [150, 400] is roughly +/-6 sigma. */
    CU_ASSERT_TRUE(chi2 > 150.0);
    CU_ASSERT_TRUE(chi2 < 400.0);

    /* Overall bit balance should be very close to 50%. */
    bitfrac = (double)setbits / ((double)total * 8.0);
    CU_ASSERT_TRUE(bitfrac > 0.49);
    CU_ASSERT_TRUE(bitfrac < 0.51);
}

/* AddEntropy() must accept caller-supplied material (and edge cases) without
   disturbing the resource's ability to produce output. */
void testADDENTROPY(void)
{
    UBYTE before[64], after[64];
    UBYTE sample[32];
    ULONG i;

    for (i = 0; i < sizeof(sample); i++)
        sample[i] = (UBYTE)(i * 7 + 1);

    CU_ASSERT_EQUAL(GetEntropy(before, sizeof(before)), (LONG)sizeof(before));

    /* Robustness: NULL/zero must be harmless no-ops. */
    AddEntropy(NULL, 16, 8);
    AddEntropy(sample, 0, 0);

    /* A real contribution. */
    AddEntropy(sample, sizeof(sample), sizeof(sample) * 8);

    CU_ASSERT_EQUAL(GetEntropy(after, sizeof(after)), (LONG)sizeof(after));
    CU_ASSERT_TRUE(memcmp(before, after, sizeof(before)) != 0);
}

/* --- concurrency stress ------------------------------------------------- */

#define NWORKERS     4
#define WORKER_ITERS 1000

static volatile LONG workers_done;
static volatile LONG workers_err;

static void worker_entry(void)
{
    UBYTE buf[64];
    int   i, err = 0;

    for (i = 0; i < WORKER_ITERS; i++)
    {
        ULONG j;
        int   allsame = 1;

        if (GetEntropy(buf, sizeof(buf)) != (LONG)sizeof(buf))
        {
            err++;
            break;
        }
        for (j = 1; j < sizeof(buf); j++)
            if (buf[j] != buf[0])
                allsame = 0;
        if (allsame)
        {
            err++;
            break;
        }
    }

    Forbid();
    workers_err  += err;
    workers_done += 1;
    Permit();
}

/* Hammer GetEntropy() from several tasks at once to stress the internal
   semaphore and shared CSPRNG state. */
void testCONCURRENT(void)
{
    struct Task *t[NWORKERS];
    int          started = 0, i;

    workers_done = 0;
    workers_err  = 0;

    for (i = 0; i < NWORKERS; i++)
    {
        t[i] = CreateTask("entropy-stress", 0, worker_entry, 16384);
        if (t[i])
            started++;
    }
    CU_ASSERT_TRUE_FATAL(started > 0);

    /* Wait up to ~10s (500 * 20ms ticks) for the workers to finish. */
    for (i = 0; i < 500 && workers_done < started; i++)
        Delay(1);

    CU_ASSERT_EQUAL(workers_done, started);
    CU_ASSERT_EQUAL(workers_err, 0);
}

int main(void)
{
    CU_pSuite pSuite = NULL;

    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    pSuite = CU_add_suite("Entropy_Suite", init_suite, clean_suite);
    if (NULL == pSuite)
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if ((NULL == CU_add_test(pSuite, "OpenResource()", testOPEN)) ||
        (NULL == CU_add_test(pSuite, "GetEntropyInfo()", testINFO)) ||
        (NULL == CU_add_test(pSuite, "GetEntropy() API", testAPI)) ||
        (NULL == CU_add_test(pSuite, "GetEntropy() bounds", testBOUNDS)) ||
        (NULL == CU_add_test(pSuite, "successive blocks differ", testDISTINCT)) ||
        (NULL == CU_add_test(pSuite, "output not constant", testNOTCONSTANT)) ||
        (NULL == CU_add_test(pSuite, "statistical stress", testSTATS)) ||
        (NULL == CU_add_test(pSuite, "AddEntropy()", testADDENTROPY)) ||
        (NULL == CU_add_test(pSuite, "concurrent stress", testCONCURRENT)))
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("EntropyUnitTests");
    CU_set_output_filename("Entropy");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}
