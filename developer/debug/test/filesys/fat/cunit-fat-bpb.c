#include <dos/filesystemids.h>
#include <exec/types.h>

#include <CUnit/Automated.h>
#include <CUnit/Basic.h>

#include <string.h>

#include "bpb.h"

#define SECTOR_SIZE 512
#define BPB_BYTES_PER_SECTOR       11
#define BPB_SECTORS_PER_CLUSTER    13
#define BPB_RESERVED_SECTORS       14
#define BPB_FAT_COUNT              16
#define BPB_ROOT_ENTRY_COUNT       17
#define BPB_TOTAL_SECTORS_16       19
#define BPB_MEDIA                  21
#define BPB_FAT_SIZE_16            22
#define BPB_TOTAL_SECTORS_32       32
#define BPB_FAT_SIZE_32            36
#define BPB_FILESYSTEM_16          54
#define BPB_FILESYSTEM_32          82
#define BPB_SIGNATURE              510

static void WriteLE16(UBYTE *value, UWORD number)
{
    value[0] = number;
    value[1] = number >> 8;
}

static void WriteLE32(UBYTE *value, ULONG number)
{
    value[0] = number;
    value[1] = number >> 8;
    value[2] = number >> 16;
    value[3] = number >> 24;
}

static ULONG InitBPB(UBYTE *data, ULONG clusters)
{
    ULONG total_sectors = clusters + 3;

    memset(data, 0, SECTOR_SIZE);
    WriteLE16(data + BPB_BYTES_PER_SECTOR, SECTOR_SIZE);
    data[BPB_SECTORS_PER_CLUSTER] = 1;
    WriteLE16(data + BPB_RESERVED_SECTORS, 1);
    data[BPB_FAT_COUNT] = 1;
    WriteLE16(data + BPB_ROOT_ENTRY_COUNT, 16);
    if (total_sectors <= 0xffff)
        WriteLE16(data + BPB_TOTAL_SECTORS_16, total_sectors);
    else
        WriteLE32(data + BPB_TOTAL_SECTORS_32, total_sectors);
    data[BPB_MEDIA] = 0xf8;
    WriteLE16(data + BPB_FAT_SIZE_16, 1);
    data[BPB_SIGNATURE] = 0x55;
    data[BPB_SIGNATURE + 1] = 0xaa;

    return total_sectors;
}

static void AssertType(ULONG clusters, ULONG expected)
{
    UBYTE data[SECTOR_SIZE];
    struct FATBPBInfo info;
    ULONG total_sectors = InitBPB(data, clusters);

    CU_ASSERT_TRUE_FATAL(FAT_ParseBPB(data, sizeof(data),
        (UQUAD)total_sectors * SECTOR_SIZE,
        &info));
    CU_ASSERT_EQUAL(info.cluster_count, clusters);
    CU_ASSERT_EQUAL(info.dos_type, expected);
}

static void TestTypesAndBoundaries(void)
{
    AssertType(4084, ID_FAT12_DISK);
    AssertType(4085, ID_FAT16_DISK);
    AssertType(65524, ID_FAT16_DISK);
    AssertType(65525, ID_FAT32_DISK);
}

static void TestLabelsAreNotSignatures(void)
{
    UBYTE data[SECTOR_SIZE];
    struct FATBPBInfo info;
    ULONG total_sectors = InitBPB(data, 70000);

    memcpy(data + 3, "NOTFAT!!", 8);
    memcpy(data + BPB_FILESYSTEM_16, "NTFS    ", 8);
    memcpy(data + BPB_FILESYSTEM_32, "NOTFAT  ", 8);

    CU_ASSERT_TRUE(FAT_ParseBPB(data, sizeof(data),
        (UQUAD)total_sectors * SECTOR_SIZE, &info));
    CU_ASSERT_EQUAL(info.dos_type, ID_FAT32_DISK);

    memset(data, 0, sizeof(data));
    memcpy(data + 3, "EXFAT   ", 8);
    memcpy(data + BPB_FILESYSTEM_32, "FAT32   ", 8);
    CU_ASSERT_FALSE(FAT_ParseBPB(data, sizeof(data),
        (UQUAD)total_sectors * SECTOR_SIZE, &info));
}

static void TestUnalignedInput(void)
{
    UBYTE storage[SECTOR_SIZE + 1];
    UBYTE *data = storage + 1;
    struct FATBPBInfo info;
    ULONG total_sectors = InitBPB(data, 4085);

    CU_ASSERT_TRUE(FAT_ParseBPB(data, SECTOR_SIZE,
        (UQUAD)total_sectors * SECTOR_SIZE, &info));
    CU_ASSERT_EQUAL(info.dos_type, ID_FAT16_DISK);
}

static void TestBPBSectorSize(void)
{
    UBYTE data[SECTOR_SIZE];
    struct FATBPBInfo info;
    ULONG total_sectors = InitBPB(data, 5000);

    WriteLE16(data + BPB_BYTES_PER_SECTOR, 4096);
    CU_ASSERT_TRUE(FAT_ParseBPB(data, sizeof(data),
        (UQUAD)total_sectors * 4096, &info));
    CU_ASSERT_EQUAL(info.sector_size, 4096);
    CU_ASSERT_EQUAL(info.cluster_count, 5000);
}

static void TestMBRPlausibility(void)
{
    UBYTE data[SECTOR_SIZE];

    InitBPB(data, 4084);
    data[BPB_SIGNATURE] = 0;
    data[BPB_SIGNATURE + 1] = 0;
    WriteLE16(data + BPB_RESERVED_SECTORS, 0);
    data[BPB_FAT_COUNT] = 0;
    WriteLE16(data + BPB_FAT_SIZE_16, 0);

    CU_ASSERT_TRUE(FAT_IsBPBPlausible(data, sizeof(data)));
    CU_ASSERT_FALSE(FAT_IsBPBPlausible(data, 21));

    WriteLE16(data + BPB_BYTES_PER_SECTOR, 256);
    CU_ASSERT_FALSE(FAT_IsBPBPlausible(data, sizeof(data)));
    WriteLE16(data + BPB_BYTES_PER_SECTOR, SECTOR_SIZE);

    data[BPB_SECTORS_PER_CLUSTER] = 3;
    CU_ASSERT_FALSE(FAT_IsBPBPlausible(data, sizeof(data)));
    data[BPB_SECTORS_PER_CLUSTER] = 1;

    data[BPB_SECTORS_PER_CLUSTER] = 128;
    WriteLE16(data + BPB_BYTES_PER_SECTOR, 1024);
    CU_ASSERT_FALSE(FAT_IsBPBPlausible(data, sizeof(data)));
    WriteLE16(data + BPB_BYTES_PER_SECTOR, SECTOR_SIZE);
    data[BPB_SECTORS_PER_CLUSTER] = 1;

    data[BPB_MEDIA] = 0xef;
    CU_ASSERT_FALSE(FAT_IsBPBPlausible(data, sizeof(data)));
}

static void TestCapacityTolerance(void)
{
    UBYTE data[SECTOR_SIZE];
    struct FATBPBInfo info;
    ULONG total_sectors = InitBPB(data, 5000);

    CU_ASSERT_TRUE(FAT_ParseBPB(data, sizeof(data),
        (UQUAD)total_sectors * SECTOR_SIZE, &info));
    CU_ASSERT_TRUE(FAT_ParseBPB(data, sizeof(data),
        (UQUAD)(total_sectors + 64) * SECTOR_SIZE,
        &info));
    CU_ASSERT_FALSE(FAT_ParseBPB(data, sizeof(data),
        (UQUAD)(total_sectors + 65) * SECTOR_SIZE,
        &info));
    CU_ASSERT_FALSE(FAT_ParseBPB(data, sizeof(data),
        (UQUAD)(total_sectors - 1) * SECTOR_SIZE,
        &info));
    CU_ASSERT_FALSE(FAT_ParseBPB(data, sizeof(data),
        0x100000000ULL * SECTOR_SIZE,
        &info));
    CU_ASSERT_FALSE(FAT_ParseBPB(data, sizeof(data),
        (UQUAD)total_sectors * SECTOR_SIZE + 1, &info));
}

static void TestInvalidLayouts(void)
{
    UBYTE data[SECTOR_SIZE];
    struct FATBPBInfo info;
    struct FATBPBInfo original;
    ULONG total_sectors = InitBPB(data, 5000);

    memset(&info, 0xa5, sizeof(info));
    original = info;
    CU_ASSERT_FALSE(FAT_ParseBPB(data, SECTOR_SIZE - 1,
        (UQUAD)total_sectors * SECTOR_SIZE,
        &info));
    CU_ASSERT_EQUAL(memcmp(&info, &original, sizeof(info)), 0);
    CU_ASSERT_FALSE(FAT_ParseBPB(NULL, sizeof(data),
        (UQUAD)total_sectors * SECTOR_SIZE, &info));
    CU_ASSERT_FALSE(FAT_ParseBPB(data, sizeof(data),
        (UQUAD)total_sectors * SECTOR_SIZE, NULL));

    data[BPB_SIGNATURE] = 0;
    CU_ASSERT_FALSE(FAT_ParseBPB(data, sizeof(data),
        (UQUAD)total_sectors * SECTOR_SIZE, &info));
    data[BPB_SIGNATURE] = 0x55;

    WriteLE16(data + BPB_RESERVED_SECTORS, 0);
    CU_ASSERT_FALSE(FAT_ParseBPB(data, sizeof(data),
        (UQUAD)total_sectors * SECTOR_SIZE, &info));
    WriteLE16(data + BPB_RESERVED_SECTORS, 1);

    data[BPB_FAT_COUNT] = 0;
    CU_ASSERT_FALSE(FAT_ParseBPB(data, sizeof(data),
        (UQUAD)total_sectors * SECTOR_SIZE, &info));
    data[BPB_FAT_COUNT] = 1;

    WriteLE16(data + BPB_FAT_SIZE_16, 0);
    WriteLE32(data + BPB_FAT_SIZE_32, 0);
    CU_ASSERT_FALSE(FAT_ParseBPB(data, sizeof(data),
        (UQUAD)total_sectors * SECTOR_SIZE, &info));
    WriteLE16(data + BPB_FAT_SIZE_16, 1);

    WriteLE16(data + BPB_RESERVED_SECTORS, total_sectors);
    CU_ASSERT_FALSE(FAT_ParseBPB(data, sizeof(data),
        (UQUAD)total_sectors * SECTOR_SIZE, &info));
}

int main(void)
{
    CU_pSuite suite;

    if (CU_initialize_registry() != CUE_SUCCESS)
        return CU_get_error();

    suite = CU_add_suite("FAT_BPB", NULL, NULL);
    if (suite == NULL ||
        CU_add_test(suite, "types and boundaries",
            TestTypesAndBoundaries) == NULL ||
        CU_add_test(suite, "labels are not signatures",
            TestLabelsAreNotSignatures) == NULL ||
        CU_add_test(suite, "unaligned input", TestUnalignedInput) == NULL ||
        CU_add_test(suite, "BPB sector size", TestBPBSectorSize) == NULL ||
        CU_add_test(suite, "MBR plausibility", TestMBRPlausibility) == NULL ||
        CU_add_test(suite, "capacity tolerance",
            TestCapacityTolerance) == NULL ||
        CU_add_test(suite, "invalid layouts", TestInvalidLayouts) == NULL)
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("FATBPBUnitTests");
    CU_set_output_filename("FATBPB");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}
