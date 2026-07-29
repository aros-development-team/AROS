/*
 * fat-handler - FAT12/16/32 filesystem handler
 *
 * Copyright (C) 2026 The AROS Development Team
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 */

#include <dos/filesystemids.h>
#include <exec/types.h>

#include "bpb.h"

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
#define BPB_SIGNATURE              510
#define BPB_MIN_PLAUSIBLE_SIZE     22
#define BPB_MIN_SIZE               512
#define BPB_MAX_CLUSTER_SIZE       (64 * 1024)
#define BPB_SIZE_TOLERANCE         64

static UWORD ReadLE16(const UBYTE *value)
{
    return (UWORD)value[0] | ((UWORD)value[1] << 8);
}

static ULONG ReadLE32(const UBYTE *value)
{
    return (ULONG)value[0] |
        ((ULONG)value[1] << 8) |
        ((ULONG)value[2] << 16) |
        ((ULONG)value[3] << 24);
}

static BOOL IsSectorSizeValid(ULONG size)
{
    return size == 512 || size == 1024 || size == 2048 || size == 4096;
}

static BOOL IsClusterSectorsValid(ULONG sectors)
{
    return sectors != 0 && sectors <= 128 &&
        (sectors & (sectors - 1)) == 0;
}

BOOL FAT_IsBPBPlausible(const UBYTE *data, ULONG size)
{
    ULONG sector_size;
    ULONG cluster_sectors;

    if (data == NULL || size < BPB_MIN_PLAUSIBLE_SIZE)
        return FALSE;

    sector_size = ReadLE16(data + BPB_BYTES_PER_SECTOR);
    cluster_sectors = data[BPB_SECTORS_PER_CLUSTER];

    return IsSectorSizeValid(sector_size) &&
        IsClusterSectorsValid(cluster_sectors) &&
        cluster_sectors * sector_size <= BPB_MAX_CLUSTER_SIZE &&
        data[BPB_MEDIA] >= 0xf0;
}

BOOL FAT_ParseBPB(const UBYTE *data, ULONG size, UQUAD media_size,
    struct FATBPBInfo *info)
{
    struct FATBPBInfo parsed;
    UQUAD media_sectors;
    UQUAD root_dir_bytes;
    UQUAD overhead;
    UQUAD data_sectors;
    ULONG root_entries;
    ULONG total_sectors_16;
    ULONG fat_size_16;

    if (data == NULL || info == NULL || size < BPB_MIN_SIZE ||
        media_size == 0)
        return FALSE;

    if (!FAT_IsBPBPlausible(data, size) ||
        data[BPB_SIGNATURE] != 0x55 ||
        data[BPB_SIGNATURE + 1] != 0xaa)
        return FALSE;

    parsed.sector_size = ReadLE16(data + BPB_BYTES_PER_SECTOR);
    media_sectors = media_size / parsed.sector_size;
    if (media_size % parsed.sector_size != 0 ||
        media_sectors == 0 || media_sectors > 0xffffffffULL)
        return FALSE;

    parsed.sectors_per_cluster = data[BPB_SECTORS_PER_CLUSTER];
    parsed.cluster_size = parsed.sector_size * parsed.sectors_per_cluster;
    parsed.reserved_sectors = ReadLE16(data + BPB_RESERVED_SECTORS);
    parsed.fat_count = data[BPB_FAT_COUNT];
    root_entries = ReadLE16(data + BPB_ROOT_ENTRY_COUNT);
    total_sectors_16 = ReadLE16(data + BPB_TOTAL_SECTORS_16);
    fat_size_16 = ReadLE16(data + BPB_FAT_SIZE_16);

    parsed.total_sectors = total_sectors_16 != 0 ? total_sectors_16 :
        ReadLE32(data + BPB_TOTAL_SECTORS_32);
    parsed.fat_size = fat_size_16 != 0 ? fat_size_16 :
        ReadLE32(data + BPB_FAT_SIZE_32);

    if (parsed.reserved_sectors == 0 || parsed.fat_count == 0 ||
        parsed.total_sectors == 0 || parsed.fat_size == 0)
        return FALSE;

    /*
     * Keep the handler's historical allowance for media whose geometry is
     * slightly larger than the filesystem recorded in the BPB.
     */
    if (parsed.total_sectors > media_sectors ||
        media_sectors - parsed.total_sectors > BPB_SIZE_TOLERANCE)
        return FALSE;

    root_dir_bytes = (UQUAD)root_entries * 32;
    parsed.root_dir_sectors =
        (root_dir_bytes + parsed.sector_size - 1) / parsed.sector_size;

    overhead = (UQUAD)parsed.reserved_sectors +
        (UQUAD)parsed.fat_count * parsed.fat_size +
        parsed.root_dir_sectors;
    if (overhead >= media_sectors)
        return FALSE;

    data_sectors = media_sectors - overhead;
    if (data_sectors > 0xffffffffULL)
        return FALSE;

    parsed.data_sectors = data_sectors;
    parsed.cluster_count =
        parsed.data_sectors / parsed.sectors_per_cluster;
    parsed.first_root_dir_sector = parsed.reserved_sectors +
        parsed.fat_count * parsed.fat_size;
    parsed.first_data_sector =
        parsed.first_root_dir_sector + parsed.root_dir_sectors;

    if (parsed.cluster_count < 4085)
        parsed.dos_type = ID_FAT12_DISK;
    else if (parsed.cluster_count < 65525)
        parsed.dos_type = ID_FAT16_DISK;
    else
        parsed.dos_type = ID_FAT32_DISK;

    *info = parsed;
    return TRUE;
}
