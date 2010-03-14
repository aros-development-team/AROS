#ifndef _AHCI_HBA_H
#define _AHCI_HBA_H

/*
    Copyright © 2010, The AROS Development Team. All rights reserved
    $Id$
*/

#include <exec/types.h>
#include <inttypes.h>

#define AHCI_VERSION_0_95   0x00000095
#define AHCI_VERSION_1_00   0x00010000
#define AHCI_VERSION_1_10   0x00010100
#define AHCI_VERSION_1_20   0x00010200
#define AHCI_VERSION_1_30   0x00010300

enum {
	CAP_S64A		= (1 << 31),	// Supports 64-bit Addressing
	CAP_SNCQ		= (1 << 30),	// Supports Native Command Queuing
	CAP_SSNTF		= (1 << 29),	// Supports SNotification Register
	CAP_SMPS		= (1 << 28),	// Supports Mechanical Presence Switch
	CAP_SSS			= (1 << 27),	// Supports Staggered Spin-up
	CAP_SALP		= (1 << 26),	// Supports Aggressive Link Power Management
	CAP_SAL			= (1 << 25),	// Supports Activity LED
	CAP_SCLO		= (1 << 24),	// Supports Command List Override
	CAP_ISS_MASK 	= 0xf, 			// Interface Speed Support
	CAP_ISS_SHIFT	= 20,
	CAP_SNZO 		= (1 << 19),	// Supports Non-Zero DMA Offsets
	CAP_SAM 		= (1 << 18),	// Supports AHCI mode only
	CAP_SPM 		= (1 << 17),	// Supports Port Multiplier
	CAP_FBSS 		= (1 << 16),	// FIS-based Switching Supported
	CAP_PMD 		= (1 << 15),	// PIO Multiple DRQ Block
	CAP_SSC 		= (1 << 14),	// Slumber State Capable
	CAP_PSC 		= (1 << 13),	// Partial State Capable
	CAP_NCS_MASK 	= 0x1f,			// Number of Command Slots (zero-based number)
	CAP_NCS_SHIFT	= 8,
	CAP_CCCS 		= (1 << 7),		// Command Completion Coalescing Supported
	CAP_EMS 		= (1 << 6),		// Enclosure Management Supported
	CAP_SXS 		= (1 << 5), 	// Supports External SATA
	CAP_NP_MASK		= 0x1f,			// Number of Ports (zero-based number)
	CAP_NP_SHIFT	= 0,
};

enum {
    CAP2_APST       = (1 << 2),     // Automatic Partial to Slumber Transitions (APST)
    CAP2_NVNP       = (1 << 1),     // NVMHCI Present (NVMP)
    CAP2_BOH        = (1 << 0),     // BIOS/OS Handoff (BOH)
};

enum {
    BOHC_BB         = (1 << 4),
    BOHC_OOC        = (1 << 3),
    BOHC_SOOE       = (1 << 2),
    BOHC_OOS        = (1 << 1),
    BOHC_BOS        = (1 << 0),
};

enum {
	GHC_AE			= (1 << 31),	// AHCI Enable
	GHC_MRSM		= (1 << 2),		// MSI Revert to Single Message
	GHC_IE			= (1 << 1),		// Interrupt Enable
	GHC_HR			= (1 << 0),		// HBA Reset **RW1**
};

enum {
	INT_CPD			= (1 << 31),	// Cold Port Detect Status/Enable
	INT_TFE			= (1 << 30),	// Task File Error Status/Enable
	INT_HBF			= (1 << 29),	// Host Bus Fatal Error Status/Enable
	INT_HBD			= (1 << 28),	// Host Bus Data Error Status/Enable
	INT_IF			= (1 << 27),	// Interface Fatal Error Status/Enable
	INT_INF			= (1 << 26),	// Interface Non-fatal Error Status/Enable
	INT_OF			= (1 << 24),	// Overflow Status/Enable
	INT_IPM			= (1 << 23),	// Incorrect Port Multiplier Status/Enable
	INT_PRC			= (1 << 22),	// PhyRdy Change Status/Enable
	INT_DMP			= (1 << 7),		// Device Mechanical Presence Status/Enable
	INT_PC			= (1 << 6),		// Port Change Interrupt Status/Enable
	INT_DP			= (1 << 5),		// Descriptor Processed Interrupt/Enable
	INT_UF			= (1 << 4),		// Unknown FIS Interrupt/Enable
	INT_SDB			= (1 << 3),		// Set Device Bits Interrupt/Enable
	INT_DS			= (1 << 2),		// DMA Setup FIS Interrupt/Enable
	INT_PS			= (1 << 1),		// PIO Setup FIS Interrupt/Enable
	INT_DHR			= (1 << 0),		// Device to Host Register FIS Interrupt/Enable
};

enum {
	PORT_CMD_ICC_ACTIVE	 = (1 << 28),	// Interface Communication control
	PORT_CMD_ICC_SLUMBER = (6 << 28),	// Interface Communication control
	PORT_CMD_ICC_MASK    = (0xf<<28),	// Interface Communication control
	PORT_CMD_ATAPI	= (1 << 24),	// Device is ATAPI
	PORT_CMD_CR		= (1 << 15),	// Command List Running (DMA active)
	PORT_CMD_FR		= (1 << 14),	// FIS Receive Running
	PORT_CMD_FER	= (1 << 4),		// FIS Receive Enable
	PORT_CMD_CLO	= (1 << 3),		// Command List Override
	PORT_CMD_POD	= (1 << 2),		// Power On Device
	PORT_CMD_SUD	= (1 << 1),		// Spin-up Device
	PORT_CMD_ST		= (1 << 0),		// Start DMA
};

enum {
	PORT_INT_CPD	= (1 << 31),	// Cold Presence Detect Status/Enable
	PORT_INT_TFE	= (1 << 30),	// Task File Error Status/Enable
	PORT_INT_HBF	= (1 << 29),	// Host Bus Fatal Error Status/Enable
	PORT_INT_HBD	= (1 << 28),	// Host Bus Data Error Status/Enable
	PORT_INT_IF		= (1 << 27),	// Interface Fatal Error Status/Enable
	PORT_INT_INF	= (1 << 26),	// Interface Non-fatal Error Status/Enable
	PORT_INT_OF		= (1 << 24),	// Overflow Status/Enable
	PORT_INT_IPM	= (1 << 23),	// Incorrect Port Multiplier Status/Enable
	PORT_INT_PRC	= (1 << 22),	// PhyRdy Change Status/Enable
	PORT_INT_DI		= (1 << 7),		// Device Interlock Status/Enable
	PORT_INT_PC		= (1 << 6),		// Port Change Status/Enable
	PORT_INT_DP		= (1 << 5),		// Descriptor Processed Interrupt
	PORT_INT_UF		= (1 << 4),		// Unknown FIS Interrupt 
	PORT_INT_SDB	= (1 << 3),		// Set Device Bits FIS Interrupt 
	PORT_INT_DS		= (1 << 2),		// DMA Setup FIS Interrupt 
	PORT_INT_PS		= (1 << 1),		// PIO Setup FIS Interrupt
	PORT_INT_DHR	= (1 << 0),		// Device to Host Register FIS Interrupt
};

struct ahci_hwport {
	volatile ULONG      clb;			// Port x Command List Base Address (alignment 1024 byte)
	volatile ULONG      clbu;			// Port x Command List Base Address Upper 32-Bits
	volatile ULONG      fb;				// Port x FIS Base Address (alignment 256 byte)
	volatile ULONG      fbu;			// Port x FIS Base Address Upper 32-Bits
	volatile ULONG      is;				// Port x Interrupt Status
	volatile ULONG      ie;				// Port x Interrupt Enable
	volatile ULONG      cmd;			// Port x Command and Status
	volatile ULONG      res1;			// Port x Reserved
	volatile ULONG      tfd;			// Port x Task File Data
	volatile ULONG      sig;			// Port x Signature
	volatile ULONG      ssts;			// Port x Serial ATA Status (SCR0: SStatus)
	volatile ULONG      sctl;			// Port x Serial ATA Control (SCR2: SControl)
	volatile ULONG      serr;			// Port x Serial ATA Error (SCR1: SError)
	volatile ULONG      sact;			// Port x Serial ATA Active (SCR3: SActive)
	volatile ULONG      ci;				// Port x Command Issue
	volatile ULONG      sntf;			// Port x Serial ATA Notification (SCR4: SNotification)
	volatile ULONG      res2;			// Port x FIS-based Switching Control
	volatile ULONG      res[11];		// Port x Reserved
	volatile ULONG      vendor[4];		// Port x Vendor Specific
} __attribute__((__packed__));

struct ahci_hwhba {
    volatile ULONG      cap;			// 0x00 Host Capabilities
    volatile ULONG      ghc;			// 0x04 Global Host Control
    volatile ULONG      is;				// 0x08 Interrupt Status
    volatile ULONG      pi;				// 0x0c Ports Implemented
    volatile ULONG      vs;				// 0x10 Version
    volatile ULONG      ccc_ctl;		// 0x14 Command Completion Coalescing Control
    volatile ULONG      ccc_ports;		// 0x18 Command Completion Coalsecing Ports
    volatile ULONG      em_loc;			// 0x1c Enclosure Management Location
    volatile ULONG      em_ctl;			// 0x20 Enclosure Management Control
    volatile ULONG      cap2;           // 0x24 Host Capabilities Extended
    volatile ULONG      bohc;           // 0x28 BIOS/OS Handoff Control and Status
    volatile ULONG      res[29];        // 0x2c-0x9f Reserved
    volatile ULONG      vendor[24];     // 0xa0-0xff Vendor Specific registers
    struct ahci_hwport  port[32];       // 0x100
} __attribute__((__packed__));

#endif // _AHCI_HBA_H


