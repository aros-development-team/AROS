# timerequest Structure
#define io_Device     24
#define tr_time       36
# timeval structure
#define tv_secs       0
#define tv_micro      4

# Timer functions
#define GetSysTime    -44

#define ata_Error       1
#define ata_SectorCnt   2
#define ata_SectorNum   3
#define ata_CylinderL   4
#define ata_CylinderH   5
#define ata_DevHead     6
#define ata_Status      7
#define ata_Command     7
#define ata_AltStatus   0x206
#define ata_Control     0x206

#define ATA_IDENTDEV    0xec
#define ATA_READ	0x20
#define ATA_NOP		0x00

#define ATAPI_IDENTDEV  0xa1

#define ATA_TimeOut      500000
#define ATAPI_TimeOut   1000000
