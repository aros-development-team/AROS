#ifndef _ATA_H
#define _ATA_H

/*
   Copyright © 2004-2011, The AROS Development Team. All rights reserved.
   $Id$

Desc: ata.device main private include file
Lang: English
*/
/*
 * PARTIAL CHANGELOG:
 * DATE        NAME                ENTRY
 * ----------  ------------------  -------------------------------------------------------------------
 * 2008-01-25  T. Wiszkowski       Rebuilt, rearranged and partially fixed 60% of the code here
 *                                 Enabled implementation to scan for other PCI IDE controllers
 *                                 Implemented ATAPI Packet Support for both read and write
 *                                 Corrected ATAPI DMA handling
 *                                 Fixed major IDE enumeration bugs severely handicapping transfers with more than one controller
 *                                 Compacted source and implemented major ATA support procedure
 *                                 Improved DMA and Interrupt management
 *                                 Removed obsolete code
 * 2008-03-23  T. Wiszkowski       Corrected DMA PRD issue (x86_64 systems)
 * 2008-03-30  T. Wiszkowski       Added workaround for interrupt collision handling; fixed SATA in LEGACY mode.
 *                                 nForce and Intel SATA chipsets should now be operational.
 * 2008-03-31  M. Schulz           We do have asm/io.h include for ages... No need to define io functions here anymore.
 *                                 Redefined ata_in and ata_out. On x86-like systems they use inb/outb directly. On other systems
 *                                 they use pci_inb and pci_outb.
 * 2008-04-05  T. Wiszkowski       Improved IRQ management
 * 2008-04-07  T. Wiszkowski       Changed bus timeout mechanism
 * 2008-05-11  T. Wiszkowski       Remade the ata trannsfers altogether, corrected the pio/irq handling
 *                                 medium removal, device detection, bus management and much more
 * 2008-06-24  P. Fedin            Added 'nomulti' flag to disable multisector operations
 * 2009-02-21  M. Schulz           ata_in/ata_out declared as functions, if no PCI-io operations are defined.
 * 2009-10-07  M. Weiss            Rely on definition of AROS_PCI_IO_FUNCS to check if PCI-io operations are defined.
 * 2011-04-05  P. Fedin		   Store PCI HIDD attribute and method bases in ata.device base
 * 2011-05-19  P. Fedin		   The Big rework. Separated bus-specific code. Made 64-bit-friendly.
 */

/*
 * this **might** cause problems with PPC64, which **might** expect both to be 64bit.
 */
struct PRDEntry {
   ULONG   prde_Address;
   ULONG   prde_Length;
};

#define PRDE_EOT    0x80000000
#define PRD_MAX     514

/* Device types */
#define DEV_NONE        0x00
#define DEV_UNKNOWN     0x01
#define DEV_ATA         0x02
#define DEV_SATA        0x03
#define DEV_ATAPI       0x80
#define DEV_SATAPI      0x81

/*
   DriveIdent structure as returned by ATA_IDENTIFY_[DEVICE|ATAPI]
   */
struct DriveIdent {
   UWORD       id_General;             // 0
   UWORD       id_OldCylinders;        // 1
   UWORD       id_SpecificConfig;      // 2
   UWORD       id_OldHeads;            // 3
   UWORD       pad1[2];                // 4-5
   UWORD       id_OldSectors;          // 6
   UWORD       pad2[3];                // 7-9
   UBYTE       id_SerialNumber[20];    // 10-19
   UWORD       pad3[3];                // 20-22
   UBYTE       id_FirmwareRev[8];      // 23-26
   UBYTE       id_Model[40];           // 27-46
   UWORD       id_RWMultipleSize;      // 47
   UWORD       pad4;                   // 48
   UWORD       id_Capabilities;        // 49
   UWORD       id_OldCaps;             // 50
   UWORD       id_OldPIO;              // 51
   UWORD       pad5;                   // 52
   UWORD       id_ConfigAvailable;     // 53
   UWORD       id_OldLCylinders;       // 54
   UWORD       id_OldLHeads;           // 55
   UWORD       id_OldLSectors;         // 56
   UWORD       pad6[2];                // 57-58
   UWORD       id_RWMultipleTrans;     // 59
   ULONG       id_LBASectors;          // 60-61
   UWORD       id_DMADir;              // 62
   UWORD       id_MWDMASupport;        // 63
   UWORD       id_PIOSupport;          // 64
   UWORD       id_MWDMA_MinCycleTime;  // 65
   UWORD       id_MWDMA_DefCycleTime;  // 66
   UWORD       id_PIO_MinCycleTime;    // 67
   UWORD       id_PIO_MinCycleTImeIORDY; // 68
   UWORD       pad8[6];                // 69-74
   UWORD       id_QueueDepth;          // 75
   UWORD       pad9[4];                // 76-79
   UWORD       id_ATAVersion;          // 80
   UWORD       id_ATARevision;         // 81
   UWORD       id_Commands1;           // 82
   UWORD       id_Commands2;           // 83
   UWORD       id_Commands3;           // 84
   UWORD       id_Commands4;           // 85
   UWORD       id_Commands5;           // 86
   UWORD       id_Commands6;           // 87
   UWORD       id_UDMASupport;         // 88
   UWORD       id_SecurityEraseTime;   // 89
   UWORD       id_ESecurityEraseTime;  // 90
   UWORD       id_CurrentAdvPowerMode; // 91
   UWORD       id_MasterPwdRevision;   // 92
   UWORD       id_HWResetResult;       // 93
   UWORD       id_AcousticManagement;  // 94
   UWORD       id_StreamMinimunReqSize; // 95
   UWORD       id_StreamingTimeDMA;    // 96
   UWORD       id_StreamingLatency;    // 97
   ULONG       id_StreamingGranularity; // 98-99
   UQUAD       id_LBA48Sectors;        // 100-103
   UWORD       id_StreamingTimePIO;    // 104
   UWORD       pad10;                  // 105
   UWORD       id_PhysSectorSize;      // 106
   UWORD       pad11;                  // 107
   UQUAD       id_UniqueIDi[2];        // 108-115
   UWORD       pad12;                  // 116
   ULONG       id_WordsPerLogicalSector; // 117-118
   UWORD       pad13[8];               // 119-126
   UWORD       id_RemMediaStatusNotificationFeatures; // 127
   UWORD       id_SecurityStatus;      // 128
   UWORD       pad14[127];
} __attribute__((packed));

typedef struct
{
   UBYTE command;       // current ATA command
   UBYTE feature;       // FF to indicate no feature
   UBYTE secmul;        // for read multiple - multiplier. default 1
   UBYTE pad;
   UQUAD blk;
   ULONG sectors;
   APTR  buffer;
   ULONG length;
   ULONG actual;

   enum
   {
      CM_NoData,
      CM_PIORead,
      CM_PIOWrite,
      CM_DMARead,
      CM_DMAWrite
   } method;
   enum
   {
      CT_NoBlock,
      CT_LBA28,
      CT_LBA48,
   } type;
} ata_CommandBlock;

typedef enum
{
   AB_XFER_PIO0 = 0,
   AB_XFER_PIO1,
   AB_XFER_PIO2,
   AB_XFER_PIO3,
   AB_XFER_PIO4,

   AB_XFER_MDMA0,
   AB_XFER_MDMA1,
   AB_XFER_MDMA2,

   AB_XFER_UDMA0,
   AB_XFER_UDMA1,
   AB_XFER_UDMA2,
   AB_XFER_UDMA3,
   AB_XFER_UDMA4,
   AB_XFER_UDMA5,
   AB_XFER_UDMA6,

   AB_XFER_48BIT,
   AB_XFER_RWMULTI,
   AB_XFER_PACKET,
   AB_XFER_LBA,
   AB_XFER_DMA,

} ata_XferMode;

#define AF_XFER_PIO(x)  (1<<(AB_XFER_PIO0+(x)))
#define AF_XFER_MDMA(x) (1<<(AB_XFER_MDMA0+(x)))
#define AF_XFER_UDMA(x) (1<<(AB_XFER_UDMA0+(x)))
#define AF_XFER_48BIT   (1<<(AB_XFER_48BIT))
#define AF_XFER_RWMULTI (1<<(AB_XFER_RWMULTI))
#define AF_XFER_PACKET  (1<<(AB_XFER_PACKET))
#define AF_XFER_LBA     (1<<(AB_XFER_LBA))
#define AF_XFER_DMA     (1<<(AB_XFER_DMA))

/* Unit internal flags */
#define AB_DiscPresent          30     /* disc now in drive */
#define AB_DiscChanged          29     /* disc changed */
#define AB_Removable            28     /* media removable */
#define AB_80Wire               27     /* has an 80-wire cable */

#define AF_DiscPresent          (1 << AB_DiscPresent)
#define AF_DiscChanged          (1 << AB_DiscChanged)
#define AF_Removable            (1 << AB_Removable)
#define AF_80Wire               (1 << AB_80Wire)

/* ATA/ATAPI registers */
#define ata_Error           1
#define ata_Feature         1
#define ata_Count           2
#define ata_LBALow          3
#define ata_LBAMid          4
#define ata_LBAHigh         5
#define ata_DevHead         6
#define ata_Status          7
#define ata_Command         7
#define ata_AltStatus       0x2
#define ata_AltControl      0x2

#define atapi_Error         1
#define atapi_Features      1
#define atapi_Reason        2
#define atapi_ByteCntL      4
#define atapi_ByteCntH      5
#define atapi_DevSel        6
#define atapi_Status        7
#define atapi_Command       7

/* Atapi status bits */
#define ATAB_SLAVE          4
#define ATAB_LBA            6
#define ATAB_ATAPI          7
#define ATAB_DATAREQ        3
#define ATAB_ERROR          0
#define ATAB_BUSY           7

#define ATAF_SLAVE          0x10
#define ATAF_LBA            0x40
#define ATAF_ATAPI          0x80
#define ATAF_DATAREQ        0x08
#define ATAF_ERROR          0x01
#define ATAF_BUSY           0x80
#define ATAF_DRDY           0x40

#define ATAPIF_CHECK        0x01

/* ATA/ATAPI commands */
#define ATA_SET_FEATURES    0xef
#define ATA_SET_MULTIPLE    0xc6
#define ATA_DEVICE_RESET    0x08
#define ATA_IDENTIFY_DEVICE 0xec
#define ATA_IDENTIFY_ATAPI  0xa1
#define ATA_NOP             0x00
#define ATA_EXECUTE_DIAG    0x90
#define ATA_PACKET          0xa0
#define ATA_READ_DMA        0xc8
#define ATA_READ_DMA64      0x25
#define ATA_READ            0x20
#define ATA_READ64          0x24
#define ATA_READ_MULTIPLE   0xc4
#define ATA_READ_MULTIPLE64 0x29
#define ATA_WRITE_DMA       0xca
#define ATA_WRITE_DMA64     0x35
#define ATA_WRITE           0x30
#define ATA_WRITE64         0x34
#define ATA_WRITE_MULTIPLE  0xc5
#define ATA_WRITE_MULTIPLE64 0x39
#define ATA_MEDIA_EJECT     0xed

#define ATAPIF_MASK         0x03
#define ATAPIF_COMMAND      0x01
#define ATAPIF_READ         0x02
#define ATAPIF_WRITE        0x00

/* SFF-8038i DMA registers */
#define dma_Command         0x00
#define dma_Status          0x02
#define dma_PRD             0x04

/* DMA command register */
#define DMA_READ            0x00    /* PCI *READS* from memory to drive */
#define DMA_WRITE           0x08    /* PCI *WRITES* to memory from drive */
#define DMA_START           0x01    /* DMA Start/Stop */

#define DMAB_Active         0
#define DMAB_Error          1
#define DMAB_Interrupt      2
#define DMAB_Simplex        7

#define DMAF_Active         (1 << DMAB_Active)
#define DMAF_Error          (1 << DMAB_Error)
#define DMAF_Interrupt      (1 << DMAB_Interrupt)
#define DMAF_Simplex        (1 << DMAB_Simplex)

#define ATAPI_SS_EJECT  0x02
#define ATAPI_SS_LOAD   0x03

struct atapi_StartStop
{
    UBYTE   command;
    UBYTE   immediate;
    UBYTE   pad1[2];
    UBYTE   flags;
    UBYTE   pad2[7];
};

#endif // _ATA_H

