#ifndef DOS_FILEHANDLER_H
#define DOS_FILEHANDLER_H

#define DE_TABLESIZE	0
#define DE_SIZEBLOCK	1	/* LONGs per block. */
#define DE_BLOCKSIZE	2	/* Bytes per block. */
#define DE_NUMHEADS	3
#define DE_BLKSPERTRACK 5
#define DE_RESERVEDBLKS 6
#define DE_INTERLEAVE	8
#define DE_LOWCYL	9	/* First cylinder of partition. */
#define DE_HIGHCYL	10	/* Last cylinder of partition. */
#define DE_UPPERCYL	10	/* Alias. */
#define DE_NUMBUFFERS	11
#define DE_BUFMEMTYPE	12
#define DE_MAXTRANSFER	13
#define DE_MASK		14
#define DE_BOOTPRI	15
#define DE_DOSTYPE	16
#define DE_BAUD		17
#define DE_CONTROL	18
#define DE_BOOTBLOCKS	19

#endif