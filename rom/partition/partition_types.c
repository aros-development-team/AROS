/*
    Copyright © 1995-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <libraries/iffparse.h>
#include <libraries/uuid.h>

#include "partition_types.h"

/*
 * TODO: This should not be a hardcoded mapping, instead it should be loaded
 * from some file, like DEVS:partition-types.
 */

static const uuid_t GPT_Type_EFISystem    = MAKE_UUID(0xC12A7328, 0xF81F, 0x11D2, 0xBA4B, 0x00A0C93EC93BULL);
static const uuid_t GPT_Type_HFSPlus      = MAKE_UUID(0x48465300, 0x0000, 0x11AA, 0xAA11, 0x00306543ECACULL);
static const uuid_t GPT_Type_FreeBSD_Boot = MAKE_UUID(0x83BD6B9D, 0x7F41, 0x11DC, 0xBE0B, 0x001560B84F0FULL);
static const uuid_t GPT_Type_FreeBSD_Data = MAKE_UUID(0x516E7CB4, 0x6ECF, 0x11D6, 0x8FF8, 0x00022D09712BULL);
static const uuid_t GPT_Type_NetBSD_FFS   = MAKE_UUID(0x49F48D5A, 0xB10E, 0x11DC, 0xB99B, 0x0019D1879648ULL);

const struct TypeMapping PartTypes[] =
{
    { 0x01, MAKE_ID('F','A','T','\0'), NULL                  }, /* DOS 12-bit FAT */
    { 0x04, MAKE_ID('F','A','T','\1'), NULL                  }, /* DOS 16-bit FAT (up to 32M) */
    { 0x06, MAKE_ID('F','A','T','\1'), NULL                  }, /* DOS 16-bit FAT (over 32M) */
    { 0x07, MAKE_ID('N','T','F','S') , NULL                  }, /* Windows NT NTFS */
    { 0x0b, MAKE_ID('F','A','T','\2'), NULL                  }, /* W95 FAT32 */
    { 0x0c, MAKE_ID('F','A','T','\2'), &GPT_Type_EFISystem   }, /* W95 LBA FAT32 */
    { 0x0e, MAKE_ID('F','A','T','\1'), NULL                  }, /* W95 16-bit LBA FAT */
    { 0x2c, MAKE_ID('D','O','S','\0'), NULL                  }, /* AOS OFS */
    { 0x2d, MAKE_ID('D','O','S','\1'), NULL                  }, /* AOS FFS */
    { 0x2e, MAKE_ID('D','O','S','\3'), NULL                  }, /* AOS FFS-I */
    { 0x2f, MAKE_ID('S','F','S','\0'), NULL                  }, /* AOS SFS */
    { 0x80, MAKE_ID('M','N','X','\0'), NULL                  }, /* MINIX until 1.4a */
    { 0x81, MAKE_ID('M','N','X','\1'), NULL                  }, /* MINIX since 1.4b */
    { 0x83, MAKE_ID('E','X','T','\2'), NULL                  }, /* linux native partition */
    { 0x8e, MAKE_ID('L','V','M','\0'), NULL                  }, /* linux LVM partition */
    { 0x9f, MAKE_ID('B','S','D','\0'), NULL                  }, /* BSD/OS */
    { 0xa5, MAKE_ID('B','S','D','\1'), &GPT_Type_FreeBSD_Boot}, /* FreeBSD */
    { 0xa5, MAKE_ID('B','S','D','\1'), &GPT_Type_FreeBSD_Data},
    { 0xa5, MAKE_ID('B','S','D','\1'), &GPT_Type_NetBSD_FFS  }, /* NetBSD */
    { 0xa6, MAKE_ID('B','S','D','\2'), NULL                  }, /* OpenBSD */
    { 0xaf, MAKE_ID('H','F','S','+' ), &GPT_Type_HFSPlus     },
    { 0xdb, MAKE_ID('C','P','M','\2'), NULL                  }, /* CPM/M */
    { 0xeb, MAKE_ID('B','E','F','S' ), NULL                  }, /* BeOS FS */
    { 0xec, MAKE_ID('S','K','Y','\0'), NULL                  }, /* SkyOS FS */
    { 0xfd, MAKE_ID('R','A','I','D' ), NULL                  }, /* linux RAID with autodetect */
    { 0, 0, NULL }
};

ULONG MBR_FindDosType(UBYTE id)
{
    const struct TypeMapping *m;

    for (m = PartTypes; m->DOSType; m++)
    {
        if (m->MBRType == id)
            return m->DOSType;
    }

    return 0;
}
