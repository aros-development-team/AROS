#include <libraries/uuid.h>

/* Type ID for legacy MBR */
#define MBRT_GPT 0xEE

/* GPT partition table header */
struct GPTHeader
{
    char   Signature[8];	/* ID signature						  */
    ULONG  Revision;		/* Revision number					  */
    ULONG  HeaderSize;		/* Size of this structure				  */
    ULONG  HeaderCRC32;		/* CRC32 of the header					  */
    ULONG  Reserved;
    UQUAD  CurrentBlock;	/* Number of block where this structure is placed	  */
    UQUAD  BackupBlock;		/* Number of block where backup partition table is placed */
    UQUAD  DataStart;		/* Number of the first usable block for data		  */
    UQUAD  DataEnd;		/* Number of the last usable block for data		  */
    uuid_t DiskID;		/* Disk UUID */
    UQUAD  PartBlock;		/* Number if the first partition entry block		  */
    ULONG  NumEntries;		/* Number of partitions in the table			  */
    ULONG  EntrySize;		/* Size of one entry					  */
    ULONG  PartCRC32;		/* CRC32 of the partition entries array			  */
				/* The rest is reserved					  */
};

#define GPT_SIGNATURE   "EFI PART"
#define GPT_HEADER_SIZE 92		/* sizeof(struct GPTHeader) gives 96 on x86-64 because of padding */
