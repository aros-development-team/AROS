/*
 * FAT BIOS Parameter Block parsing link library
 *
 * Copyright (C) 2026 The AROS Development Team
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 */

#ifndef LINKLIBS_FATBPB_H
#define LINKLIBS_FATBPB_H

#include <exec/types.h>

struct FATBPBInfo
{
    ULONG sector_size;
    ULONG sectors_per_cluster;
    ULONG cluster_size;
    ULONG reserved_sectors;
    ULONG fat_count;
    ULONG fat_size;
    ULONG total_sectors;
    ULONG root_dir_sectors;
    ULONG data_sectors;
    ULONG cluster_count;
    ULONG first_root_dir_sector;
    ULONG first_data_sector;
    ULONG dos_type;
};

/*
 * Perform the deliberately small plausibility check used to distinguish a
 * FAT superfloppy from an MBR. This is not sufficient to mount the volume.
 */
BOOL FAT_IsBPBPlausible(const UBYTE *data, ULONG size);

/*
 * Validate and decode a FAT12/16/32 BIOS Parameter Block. media_size is the
 * containing partition or device size in bytes.
 */
BOOL FAT_ParseBPB(const UBYTE *data, ULONG size, UQUAD media_size,
    struct FATBPBInfo *info);

#endif /* LINKLIBS_FATBPB_H */
