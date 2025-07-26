/*
    Copyright © 2003-2025, The AROS Development Team. All rights reserved.
    $Id$
*/

#define INTUITION_NO_INLINE_STDARG
#include   <aros/debug.h>

#include   <proto/uuid.h>

#include   <libraries/partition.h>
#include   <libraries/uuid.h>

#include "QP_Intern.h"
#include "QP_PartionColors.h"

const char *part_AFFS = "AFFS";
const char *part_MuFS = "MuFS";
const char *part_SFS = "SFS";
const char *part_PFS = "PFS";
const char *part_NTFS = "NTFS";
const char *part_FAT32 = "FAT32";
const char *part_EXT2 = "Ext2";

const uuid_t EFISYS_UUID	= {0xC12A7328, 0xF81F, 0x11D2, 0xBA, 0x4B, {0x00, 0xA0, 0xC9, 0x3E, 0xC9, 0x3B}};
const uuid_t BOOT_UUID		= {0x21686148, 0x6449, 0x6E6F, 0x74, 0x4E, {0x65, 0x65, 0x64, 0x45, 0x46, 0x49}};
const uuid_t MSR_UUID		= {0xE3C9E316, 0x0B5C, 0x4DB8, 0x81, 0x7D, {0xF9, 0x2D, 0xF0, 0x02, 0x15, 0xAE}};
const uuid_t MSDATA_UUID 	= {0xEBD0A0A2, 0xB9E5, 0x4433, 0x87, 0xC0, {0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7}};

void GetPartitionDrawAttribs(struct PartitionType *PartType, IPTR *BGPtr, char **LabelPtr)
{
	*BGPtr = (IPTR)DEF_PART_UNUSED;
	if (PartType->id_len == 1)
	{
		switch (PartType->id[0])
		{
		case 5:
		case 15:
			/* EBR */
			*BGPtr = (IPTR)DEF_PART_MBREXT;
			break;

		case 48:
			/* MBR AROS Extended Partition */
			*BGPtr = (IPTR)DEF_PART_AROSEXT;
			break;

		case 7:
			*LabelPtr = (char *)part_NTFS;
			*BGPtr = (IPTR)DEF_PART_NTFS;
			break;

		case 12:
			*LabelPtr = (char *)part_FAT32;
			*BGPtr = (IPTR)DEF_PART_FAT;
			break;

		case 131:
			*LabelPtr = (char *)part_EXT2;
			*BGPtr = (IPTR)DEF_PART_EXT;
			break;

		default :
			break;
		}
		D(bug("[QuickPart] MBR Style Partition ID %02x - %s\n", PartType->id[0], *LabelPtr);)
	}
	else if (PartType->id_len == 4)
	{
		ULONG fsid = AROS_BE2LONG(*(ULONG *)&PartType->id[0]);

		D(bug("[QuickPart] RDB Style Partition ID - %08x\n", fsid));
		switch (fsid)
		{
			case ID_DOS_DISK:
			case ID_FFS_DISK:
			case ID_INTER_DOS_DISK:
			case ID_INTER_FFS_DISK:
			case ID_FASTDIR_DOS_DISK:
			case ID_FASTDIR_FFS_DISK:
			case ID_LNFS_DOS_DISK:
			case ID_LNFS_FFS_DISK:
				*LabelPtr = (char *)part_AFFS;
				*BGPtr = (IPTR)DEF_PART_AFFS;
				break;

			case ID_SFS_BE_DISK:
			case ID_SFS_LE_DISK:
				*LabelPtr = (char *)part_SFS;
				*BGPtr = (IPTR)DEF_PART_SFS;
				break;

			case ID_PFS_DISK:
			case ID_PFS2_DISK:
			case ID_PFS3_DISK:
			case ID_PFS2_SCSI_DISK:
			case ID_PFS3_SCSI_DISK:
				*LabelPtr = (char *)part_PFS;
				*BGPtr = (IPTR)DEF_PART_PFS;
				break;

			case ID_FAT12_DISK:
			case ID_FAT16_DISK:

			case ID_FAT32_DISK:
				*LabelPtr = (char *)part_FAT32;
				*BGPtr = (IPTR)DEF_PART_FAT;
				break;

			case ID_NTFS_DISK:
				*LabelPtr = (char *)part_NTFS;
				*BGPtr = (IPTR)DEF_PART_NTFS;
				break;

			case ID_EXT2_DISK:
				*LabelPtr = (char *)part_EXT2;
				*BGPtr = (IPTR)DEF_PART_EXT;
				break;

			default:
				break;
		}
	}
	else if ((PartType->id_len == 16) && (UUIDBase))
	{
		D(bug("[QuickPart] GUID Partition ID Style (ID Len = %u)\n", PartType->id_len);)
		if (UUID_Compare(&EFISYS_UUID, (const uuid_t *)&PartType->id[0]) == 0)
		{
			D(bug("[QuickPart] EFI System GUID\n");)
			*LabelPtr = (char *)"EFI Sys";
		}
		else if (UUID_Compare(&BOOT_UUID, (const uuid_t *)&PartType->id[0]) == 0)
		{
			D(bug("[QuickPart] Boot Partition GUID\n");)
			*LabelPtr = (char *)"Boot";
		}
		else if (UUID_Compare(&MSR_UUID, (const uuid_t *)&PartType->id[0]) == 0)
		{
			D(bug("[QuickPart] MSR GUID\n");)
			*LabelPtr = (char *)"MSR";
		}
		else if (UUID_Compare(&MSDATA_UUID, (const uuid_t *)&PartType->id[0]) == 0)
		{
			D(bug("[QuickPart] Ms. Data Partition GUID\n");)
			*LabelPtr = (char *)"Ms. Data";
		}
	}
	else
	{
		D(bug("[QuickPart] Unknown Partition ID Style (ID Len = %u)\n", PartType->id_len);)
	}
}


void GetPartitionContainerDrawAttribs(IPTR *ContainerType, IPTR *BGPtr, char **LabelPtr)
{
	switch(*ContainerType)
	{
	case PHPTT_RDB:
		D(bug("[QuickPart]   # RDB Container\n"));
		*BGPtr = (IPTR)DEF_PART_AROSEXT;
		break;

	case PHPTT_MBR:
		D(bug("[QuickPart]   # MBR Container\n"));
		*BGPtr = (IPTR)DEF_PART_MBREXT;
		break;

	case PHPTT_GPT:
		D(bug("[QuickPart]   # GPT Container\n"));
		*BGPtr = (IPTR)DEF_PART_GPTEXT;
		break;

	default:
		D(bug("[QuickPart]   # UNKNOWN Container!\n"));
		*BGPtr = (IPTR)DEF_PART_UNUSED;
		break;
	}
}

void InitDefaultMappings()
{
#if (0)
STATIC const struct MUI_Palette_Entry initialpens[8] =
{
    {DETAILPEN,                 0, 0, 0, part_AFFS},
    {BLOCKPEN,                  0, 0, 0, part_SFS},
    {TEXTPEN,                   0, 0, 0, part_PFS},
    {SHINEPEN,                  0, 0, 0, part_MuFS},
    {SHADOWPEN,                 0, 0, 0, part_FAT32},
    {FILLPEN,                   0, 0, 0, part_NTFS},
    {FILLTEXTPEN,               0, 0, 0, part_EXT2},
    {MUIV_Palette_Entry_End,    0, 0, 0, 0}
};
#endif
}
