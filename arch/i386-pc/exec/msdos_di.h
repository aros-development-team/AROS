/*
 * FAT partition boot sector information, taken from the Linux
 * kernel sources.
 * ts=8
 */


struct fat_boot_sector {
	char	ignored[3];	/* Boot strap short or near jump */
	char	system_id[8];	/* Name - can be used to special case
				   partition manager volumes */
	UBYTE	sector_size[2];	/* bytes per logical sector */
	UBYTE	cluster_size;	/* sectors/cluster */
	UWORD	reserved;	/* reserved sectors */
	UBYTE	fats;		/* number of FATs */
	UBYTE	dir_entries[2];	/* root directory entries */
	UBYTE	sectors[2];	/* number of sectors */
	UBYTE	media;		/* media code (unused) */
	UWORD	fat_length;	/* sectors/FAT */
	UWORD	secs_track;	/* sectors per track */
	UWORD	heads;		/* number of heads */
	ULONG	hidden;		/* hidden sectors (unused) */
	ULONG	total_sect;	/* number of sectors (if sectors == 0) */

	/* The following fields are only used by FAT32 */
	ULONG	fat32_length;	/* sectors/FAT */
	UWORD	flags;		/* bit 8: fat mirroring, low 4: active fat */
	UBYTE	version[2];	/* major, minor filesystem version */
	ULONG	root_cluster;	/* first cluster in root directory */
	UWORD	info_sector;	/* filesystem info sector */
	UWORD	backup_boot;	/* backup boot sector */
	UWORD	reserved2[6];	/* Unused */
};
