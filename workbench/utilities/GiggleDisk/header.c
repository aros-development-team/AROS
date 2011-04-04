
/*
** header.c
**
** (c) 1998-2011 Guido Mersmann
*/

/*************************************************************************/

#define SOURCENAME "header.c"
#define NODEBUG  /* turns off debug of this file */

#include <internal/debug.h>
#include <internal/memory.h>

#include <exec/types.h>

#include "header.h"
#include "functions.h"

/*************************************************************************/

/* /// "FSIDARRAY[]" */

/*************************************************************************/

TYPFSID FSIDARRAY[]={

	{ 0x00 ,"Empty" },

/*
** To be precise: this is not used to designate unused area on the disk, but
** marks an unused partition table entry. (All other fields should be zero as
** well.) Unused area is not designated. Plan9 assumes that it can use
** everything not claimed for other systems in the partition table.
*/

	{ 0x01 ,"DOS 12-bit FAT" },

/*
** DOS is a family of single-user operating systems for PCs. 86-DOS (`QDOS' -
** Quick and Dirty OS) was a CP/M-like operating system written by Tim Paterson
** of Seattle Computer Products (1979). Microsoft bought it, renamed it to
** MS-DOS 1.0 and sold it to IBM (1980) to be delivered together with the first
** IBM PCs (1981). MS-DOS 2.0 (1983) was rather different, and designed to be
** somewhat Unix-like. It supported a hard disk (up to 16MB; up to 32MB for
** version 2.1). Version 3.3+ added the concept of partitions, where each
** partition is at most 32MB. (Compaq DOS 3.31 relaxed this restriction.) Since
** version 4.0 partitions can be 512 MB. Version 5.0 supports partitions up to
** 2 GB. Several clones exist: DR-DOS (from Digital Research, later part of
** Novell and called NovellDOS or NDOS, then owned by Caldera and called
** OpenDOS, then by its subsidiary Lineo who named it back to DR-DOS. See
** http://www.drdos.com/), PC-DOS (from IBM), FreeDOS, ... See Types of DOS.
** See comp.os.msdos.* and MSDOS part itioning summary. The type 01 is for
** partitions up to 15 MB.
*/

	{ 0x02 ,"XENIX root" },

	{ 0x03 ,"XENIX /usr" },

/*
** Xenix is an old port of Unix V7. Microsoft Xenix OS was announced August
** 1980, a portable and commercial version of the Unix operating system for the
** Intel 8086, Zilog Z8000, Motorola M68000 and Digital Equipment PDP-11.
** Microsoft introduces XENIX 3.0 in April 1983. ( Timeline of Microcomputers)
** SCO delivered its first Xenix for 8088/8086 in 1983. See
** comp.unix.xenix.sco.
*/

	{ 0x04 ,"DOS 3.0+ 16-bit FAT (up to 32M)" },

/*
** Matthias Paul writes: Some old DOS versions have had a bug which requires
** this partition to be located in the 1st physical 32 MB of the hard disk,
** hence for compatibility with these old issues, partitions located elsewhere
** should better be assigned the ID FAT16B (06h).
*/

	{ 0x05 ,"DOS 3.3+ Extended Partition" },

/*
** Supports at most 8.4 GB disks: with type 05 DOS/Windows will not use the
** extended BIOS call, even if it is available. See type 0f below. Using type
** 05 for extended partitions beyond 8 GB may lead to data corruption with
** MSDOS.
**
** An extended partition is a box containing a linked list of logical
** partitions. This chain (linked list) can have arbitrary length, but some
** FDISK versions refuse to make more logical partitions than there are drive
** letters available (e.g. MS-DOS LASTDRIVE=26 is good for at most 24 disk
** partitions; Novell DOS 7+ allows LASTDRIVE=32).
*/

	{ 0x06 ,"DOS 3.31+ 16-bit FAT (over 32M)" },

/* Partitions, or at least the FAT16 filesystems created on them, are at most 2
** GB for DOS and Windows 95/98 (at most 65536 clusters, each at most 32 KB).
** Windows NT can create up to 4 GB FAT16 filesystems (using 64 KB clusters),
** but these cause problems for DOS and Windows 95/98. Note that VFAT is 16-bit
** FAT with long filenames; FAT32 is a different filesystem.
*/

	{ 0x07, "Windows NT NTFS" },

/* It is rumoured that the Windows NT boot partition must be primary, and
** within the first 2 GB of the disk.
*/

	{ 0x07, "OS/2 IFS (e.g., HPFS)" },

/* IFS = Installable File System. The best known example is HPFS. OS/2 will
** only look at partitions with ID 7 for any installed IFS (that's why the
** EXT2.IFS packet includes a special "Linux partition filter" device driver to
** fool OS/2 into thinking Linux partitions have ID 07). (Kai Henningsen
** (kai@khms.westfalen.de))
*/

	{ 0x07 ,"dvanced Unix" },

	{ 0x07 ,"QNX2.x pre-1988 (see below under IDs 4d-4f)" },

	{ 0x08 ,"OS/2 (v1.0-1.3 only)" },

	{ 0x08 ,"AIX boot partition" },

	{ 0x08 ,"SplitDrive" },

	{ 0x08 ,"Commodore DOS" },

/* Matthias Paul writes: "This indicates a Commodore MS-DOS 3.x logically
** sectored FAT partition."
*/

	{ 0x08, "DELL partition spanning multiple drives" },

	{ 0x08, "QNX 1.x and 2.x (\"qny\")" },

/* (according to QNX Partitions)
*/

	{ 0x09, "AIX data partition" },

/* Some reports interchange AIX boot & data. AIX is IBM's version of Unix. See
** comp.unix.aix.
*/

	{ 0x09, "Coherent filesystem" },

/* Coherent was a UNIX-type OS for the 286-386-486, marketed by Mark Williams
** Company led by Bob Swartz, renowned for its good documentation. It was
** introduced in 1980 and died 1 Feb 1995. The last versions are V3.2 for
** 286-386-486 and V4.0 (May 1992, using protected mode) for 386-486 only. It
** sold for $99 a copy, and the FAQ says that 40000 copies have been sold. See
** comp.os.coherent and this page. A Coherent partition has to be primary.
*/

	{ 0x09, "QNX 1.x and 2.x (\"qnz\")" },

/* (according to QNX Partitions)
*/

	{ 0x0a, "OS/2 Boot Manager" },

/* OS/2 is the operating system designed by Microsoft and IBM to be the
** successor of MS-DOS. Dropped by Microsoft. See comp.os.os2. Windows 2000
** actively tries to destroy OS/2 Boot Manager. See below.
*/

	{ 0x0a, "Coherent swap partition" },

	{ 0x0a, "OPUS" },

/* Open Parallel Unisys Server. See Unisys.
*/

	{ 0x0b, "WIN95 OSR2 FAT32" },

/* Partitions up to 2047GB. See Partition Types
*/

	{ 0x0c, "WIN95 OSR2 FAT32, LBA-mapped" },

/* Extended-INT13 equivalent of 0b.
*/

	{ 0x0e, "WIN95: DOS 16-bit FAT, LBA-mapped" },

	{ 0x0f, "WIN95: Extended partition, LBA-mapped" },

/* Windows 95 uses 0e and 0f as the extended-INT13 equivalents of 06 and 05.
** For the problems this causes, see Windows 95 fdisk problems and Possible
** data loss with LBA and INT13 extensions. (Especially when going back and
** forth between MSDOS and Windows 95, strange things may happen with a type 0e
** or 0f partition.) Windows NT does not recognize the four W95 types 0b, 0c,
** 0e, 0f ( Win95 Partition Types Not Recognized by Windows NT). DRDOS 7.03
** does not support this type (but DRDOS 7.04 does).
*/

	{ 0x10, "OPUS (?)" },

/* Maybe decimal, for type 0a. */

	{ 0x11, "Hidden DOS 12-bit FAT" },

/* When it boots a DOS partition, OS/2 Boot Manager will hide all primary DOS
** partitions except the one that is booted, by changing its ID: 01, 04, 06
** becomes 11, 14, 16. Also 07 becomes 17.
*/

	{ 0x11, "Leading Edge DOS 3.x logically sectored FAT" },

/* (According to Matthias Paul.)
*/

	{ 0x12, "Configuration/diagnostics partition" },

/* ID 12 (decimal 18) is used by Compaq for their configuration utility
** partition. It is a FAT-compatible partition (about 6 MB) that boots into
** their utilities, and can be added to a LILO menu as if it were MS-DOS.
** (David C. Niemi) Stephen Collins reports a 12 MB partition with ID 12 on a
** Compaq 7330T. Tigran A. Aivazian reports a 40 MB partition with ID 12 on a
** 64 MB Compaq Proliant 1600. ID 12 is used by the Compaq Contura to denote
** its hibernation partition. (dan@fch.wimsey.bc.ca)
*/

/* NCR has used ID 0x12 MS-DOS partitions for diagnostics and firmware support
** on their WorldMark systems since the mid-90s. DataLight's ROM-DOS has
** replaced MS-DOS on more recent systems. Partition sizes were once 72M
** (MS-DOS) but are now 40M (ROM-DOS).
*/

/* Intel has begun offering ROM-DOS based "Service Partition" support on many
** OEM systems. This support initially used ID 0x98 but has recently changed to
** ID 0x12. Intel provides their own support for this partition in the form of
** a System Resource CD. Partition size has remained constant at 40M. See e.g.
** sds2.pdf. (Chuck Rouillard)
*/

	{ 0x14, "Hidden DOS 16-bit FAT <32M" },

/* (Ralf Brown's interrupt list adds: `ID 14 resulted from using Novell DOS 7.0
** FDISK to delete Linux Native partition')
*/

	{ 0x14, "AST DOS with logically sectored FAT" },

/* AST MS-DOS 3.x was an OEM version supporting 8 instead of the usual 4
** partition entries in the MBR. These special MBRs can be detected by another
** signature in the MBR stored in front of the partition table.
*/

	{ 0x16, "Hidden DOS 16-bit FAT >=32M" },

	{ 0x17, "Hidden IFS (e.g., HPFS)" },

	{ 0x18, "AST SmartSleep Partition" },

/* Ascentia laptops have a `Zero Volt Suspend Partition' or `SmartSleep
** Partition' of size 2MB+memory size. See AST. Ralf Brown calls this the "AST
** Windows swapfile".
*/

	{ 0x19, "Unused" },

/* Claimed for Willowtech Photon coS (completely optimized system) by Willow
** Schlanger willow@dezine.net. See dejanews.
*/

	{ 0x1b, "Hidden WIN95 OSR2 FAT32" },

	{ 0x1c, "Hidden WIN95 OSR2 FAT32, LBA-mapped" },

	{ 0x1e, "Hidden WIN95 16-bit FAT, LBA-mapped" },

	{ 0x20, "Unused" },

/* Rumoured to be used by Willowsoft Overture File System (OFS1), if there is
** such a thing.
*/

	{ 0x21, "Reserved" },

/* (according to delorie). And Powerquest writes `Officially listed as reserved
** (HP Volume Expansion, SpeedStor variant)'. See also ID a1.)
*/

	{ 0x21, "Unused" },

/* Claimed for FSo2 (Oxygen File System) by Dave Poirier (ekstazya@sprint.ca).
** See dejanews.
*/

	{ 0x22, "Unused" },

/* Claimed for Oxygen Extended Partition Table by ekstazya@sprint.ca. See
** dejanews.
*/

	{ 0x23, "Reserved" },

	{ 0x24, "NEC DOS 3.x" },

/* This is NEC MS-DOS 3.30 logically sectored FAT. Similar to type 14 above,
** the MBR could have up to 8 partition entries.
*/

	{ 0x26, "Reserved" },

	{ 0x2a, "AtheOS File System (AFS)" },

/* AtheOS is an open source operating system written by Kurt Skauen. It is dead
** now - for a single page, see www.atheos.cx or sourceforge. For the history,
** see wikipedia. When progress seemed to stop, the project forked and the
** Syllable OS was started by Kristian van der Vliet (2002). See also
** wikipedia. It uses the same filesystem, AthFS or AFS, an extension of BeFS,
** the filesystem of BeOS. There is an attempt at a Linux driver at
** sourceforge.
*/

	{ 0x2b, "SyllableSecure (SylStor)" },

/* A variation on AthFS is Sylstor, with added security.
*/

	{ 0x31, "Reserved" },

	{ 0x32, "NOS" },

/* Simon Butcher (simonb@alien.net.au) writes: This type is being used by an
** operating system being developed by Alien Internet Services in Melbourne
** Australia called NOS. The id '32' was chosen not only because it's one of
** the few that are left available, but 32k is the size of the EEPROM the OS
** was originally targetted for.
*/

	{ 0x33, "Reserved" },

	{ 0x34, "Reserved" },

	{ 0x35, "JFS on OS/2 or eCS" },

/* David van Enckevort (david@mensys.nl) writes: Type 0x35 is used by OS/2 Warp
** Server for e-Business, OS/2 Convenience Pack (aka version 4.5) and
** eComStation (eCS, an OEM version of OS/2 Convenience Pack) for the OS/2
** implementation of JFS (IBM AIX Journaling Filesystem). Since JFS is a
** non-bootable file system, you cannot install eCS to a JFS partition.
*/

	{ 0x36, "Reserved" },

	{ 0x38, "THEOS ver 3.2 2gb partition" },

	{ 0x39, "Plan 9 partition" },

/* Plan 9 is an operating system developed at Bell Labs for many architectures.
** Source is available. See comp.os.plan9. Originally Plan 9 used an
** unallocated portion at the end of the disk. Plan 9 3rd edition uses
** partitions of type 0x39, subdivided into subpartitions described in the Plan
** 9 partition table in the second sector of the partition.
*/

	{ 0x39, "THEOS ver 4 spanned partition" },

	{ 0x3a, "THEOS ver 4 4gb partition" },

	{ 0x3b, "THEOS ver 4 extended partition" },

/* THEOS is a multiuser multitasking OS for PCs founded by Timothy Williams in
** 1983. Current release 4.0, previous release 3.2. They say about themselves:
** `THEOS with over 150,000 customers and over 1,000,000 users around the world
** brings a mainframe look and feel to computers without the complexity and
** high maintenance costs. Hundreds of applications exist with networking and
** Windows integration.' See the Theos home page
*/

	{ 0x3c, "PartitionMagic recovery partition" },

/* Cody Batt (codyb@powerquest.com) writes: When a PowerQuest product like
** PartitionMagic or Drive Image makes changes to the disk, it first changes
** the type flag to 0x3C so that the OS won't try to modify it etc. At the end
** of the process, it gets changed back to what it was at first. So, the only
** time you should see a 0x3C type flag is if the process was interrupted
** somehow (power outage, user reboot etc). If you change it back manually with
** a partition table editor or something then most of the time everything is
** okay.
*/

	{ 0x3d, "Hidden NetWare" },

/* According to Powerquest.
*/

	{ 0x40, "Venix 80286" },

/* A very old Unix-like operating system for PCs.
*/

	{ 0x41, "Linux/MINIX (sharing disk with DRDOS)" },

/* Very old FAQs recommended to use 41 etc instead of 81 etc on a disk shared
** with DRDOS because DRDOS allegedly disregards the high order bit of the
** partition type. These types are not used anymore today. Roger Wolff
** (R.E.Wolff@BitWizard.nl) confirms: I remember installing DRDOS, and getting
** a few extra drive letters that I didn't expect. Turns out those are my Minix
** partitions. It is looking at them as a FAT filesystem. Looks like a big
** mess. After finding no other possibility than to just "not touch those drive
** letters" I continue with the install. After a few minutes DRDOS
** automatically decides to write a copy of the FAT into a file on one of my
** MINIX partitions. Bye bye Minix partition.
*/

	{ 0x41, "Personal RISC Boot" },

	{ 0x41, "PPC PReP (Power PC Reference Platform) Boot" },

	{ 0x42, "Linux swap (sharing disk with DRDOS)" },

	{ 0x42, "SFS (Secure Filesystem)" },

/* SFS is an encrypted filesystem driver for DOS on 386+ PCs, written by Peter
** Gutmann.
*/

	{ 0x42, "Windows 2000 dynamic extended partition marker" },

/* If a partition table entry of type 0x42 is present in the legacy partition
** table, then W2K ignores the legacy partition table and uses a proprietary
** partition table and a proprietary partitioning scheme (LDM or DDM). As the
** Microsoft KnowledgeBase writes: Pure dynamic disks (those not containing any
** hard-linked partitions) have only a single partition table entry (type 42)
** to define the entire disk. Dynamic disks store their volume configuration in
** a database located in a 1-MB private region at the end of each dynamic disk.
*/

	{ 0x43, "Linux native (sharing disk with DRDOS)" },

	{ 0x44, "GoBack partition" },

/* GoBack is a utility that records changes made to the disk, allowing you to
** view or go back to some earlier state. It takes over disk I/O like a Disk
** Manager would, and stores its logs in its own partition.
*/

	{ 0x45, "Boot-US boot manager" },

/* Ulrich Straub (ustraub@boot-us.de) writes: The boot manager can be installed
** to MBR, a separate primary partition or diskette. When installed to a
** primary partition this partition gets the ID 45h. This partition does not
** contain a file system, it contains only the boot manager and occupies a
** single cylinder (below 8 GB). See www.boot-us.com.
*/

	{ 0x45, "Priam" },

/* According to Powerquest. See also ID 5c. */

	{ 0x45, "EUMEL/Elan" },

	{ 0x46, "EUMEL/Elan" },

	{ 0x47, "EUMEL/Elan" },

	{ 0x48, "EUMEL/Elan" },

/* Eumel, and later Ergos L3, are multiuser multitasking systems developed by
** Jochen Liedtke at GMD. It was used at German schools for the computer
** science education. ( Elan was the programming language used.) */

	{ 0x4a, "Mark Aitchison's ALFS/THIN lightweight filesystem for DOS" },

/* According to Powerquest. */

	{ 0x4a, "AdaOS Aquila (Withdrawn)" },

/* Nick Roberts at some point in time announced that he would use 4a for
** Aquila, but now plans to use the AODPS 7f. */

	{ 0x4c, "Oberon partition" },

/* See http://www.oberon.ethz.ch/betadocu.html. This partition type (decimal
** 76) is used for the Aos filesystem. Type 4f is used for the Nat filesystem.
** One may have several partitions of this type.*/

	{ 0x4d, "QNX4.x" },

	{ 0x4e, "QNX4.x 2nd part" },

	{ 0x4f, "QNX4.x 3rd part" },

/* QNX is a POSIX-certified, microkernel, distributed, fault-tolerant OS for
** the 386 and up, including support for the 386EX in embedded applications.
** For info see http://www.qnx.com/ or ftp.qnx.com. See also comp.os.qnx. ID 7
** is outdated - QNX2 used 07, QNX4.x uses 77, and optionally 78 and 79 for
** additional QNX partitions on a single drive. These values 77, 78, 79 seem to
** be the decimal values in view of QNX Partitions and Neutrino filesystems. */

	{ 0x4f, "Oberon partition" },

/* See http://www.oberon.ethz.ch/native/. (The partition ID is given in this
** posting in comp.lang.oberon. The install instructions say that at most one
** partition can have this type (decimal 79), and that one needs a different
** type, like 50 (decimal 80) for a second Oberon system. Moreover, that users
** of System Commander must avoid types containing the 0x10 bit.) See also type
** 4c (decimal 76) above. */

	{ 0x50, "OnTrack Disk Manager (older versions) RO" },

/* Disk Manager is a program of OnTrack, to enable people to use IDE disks that
** are larger than 504MB under DOS. For info see http://www.ontrack.com. Linux
** kernel versions older than 1.3.14 do not coexist with DM.*/

	{ 0x50, "Lynx RTOS" },

/* "Beginning with version 3.0, LynxOS gives users the ability to place up to
** 14 partitions of 2 GB each on both SCSI and IDE drives, for a total of up to
** 28 GB of file system space." See www.lynuxworks.com.*/

	{ 0x50, "Native Oberon (alt)" },

	{ 0x51, "OnTrack Disk Manager RW (DM6 Aux1)" },

	{ 0x51, "Novell" },

	{ 0x52, "CP/M" },

	{ 0x52, "Microport SysV/AT" },

	{ 0x53, "Disk Manager 6.0 Aux3" },

	{ 0x54, "Disk Manager 6.0 Dynamic Drive Overlay (DDO)" },

	{ 0x55, "EZ-Drive" },

/* EZ-Drive is another disk manager (by MicroHouse, 1992). Linux kernel
** versions older than 1.3.29 do not coexist with EZD. (On 990323 MicroHouse
** International was acquired by EarthWeb; MicroHouse Solutions split off and
** changed its name into StorageSoft. MicroHouse Development split off and
** changed its name into ImageCast. It is StorageSoft that now markets EZDrive
** and DrivePro.) */

	{ 0x56, "Golden Bow VFeature Partitioned Volume." },

/* This is a Non-Standard DOS Volume. (Disk Manager type utility software) */

	{ 0x56, "DM converted to EZ-BIOS" },

	{ 0x56, "AT&T MS-DOS 3.x logically sectored FAT." },

	{ 0x57, "DrivePro" },

/* Doug Anderson (DougA@ImageCast.com), with his brother Steve cofounder of
** MicroHouse (1989), writes: We actually use three different partition types:
** $55: `StorageSoft EZ-BIOS' - EZ-Drive, Maxtor, MaxBlast, and DriveGuide
** install this type if the drive needs to be handled by our INT13 redirector.
** $56: `StorageSoft EZ-BIOS DM Conversion' - Same as $55 but used when a
** DiskManager "skewed" partition has been converted to EZ-BIOS. $57:
** `StorageSoft DrivePro' - Used by our DrivePro product. */

	{ 0x57, "VNDI Partition" },

/* (According to disk.c in the Netware source. Not in actual use.) */

	{ 0x5c, "Priam EDisk" },

/* Priam EDisk Partitioned Volume. This is a Non-Standard DOS Volume. (Disk
** Manager type utility software) */

	{ 0x61, "SpeedStor" },

/* Storage Dimensions SpeedStor Volume. This is a Non-Standard DOS Volume.
** (Disk Manager type utility software) */

	{ 0x63, "Unix System V (SCO, ISC Unix, UnixWare, ...), Mach, GNU Hurd" },

/* A Unixware 7.1 partition must start below the 4GB limit. (If the
** /stand/stage3.blm is located past this limit, booting will fail with "FATAL
** BOOT ERROR: Can't load stage3".) */

	{ 0x64, "PC-ARMOUR protected partition" },

/* Used by PC-ARMOUR, a disk protection by Dr. A.Solomon, intended to keep the
** disk inaccessible until the right password was given (and then an int13 hook
** was loaded above top-of-memory that showed c/h/s 0/0/2, with a copy of the
** real partition table, when 0/0/1 was requested). (loekw@worldonline.nl) */

	{ 0x64, "Novell Netware 286, 2.xx" },

	{ 0x65, "Novell Netware 386, 3.xx or 4.xx" },

/* (Novell Netware used to be the main Network Operating System available.
** Netware 68 or S-Net (1983) was for a Motorola 68000, Netware 86 for an Intel
** 8086 or 8088. Netware 286 was for an Intel 80286 and existed in various
** versions that were later merged to Netware 2.2. Netware 386 was a rewrite in
** C for the Intel 386, later renamed 3.x - it existed at least in versions
** 3.0, 3.1, 3.10, 3.11, 3.12. Its successor Netware 4.xx had versions 4.00,
** 4.01, 4.02, 4.10, 4.11. Then came Intranetware.) Netware >= 3.0 uses one
** partition per drive. It allocates logical Volumes inside these partitions.
** The volumes can be split over several drives. The filesystem used is called
** "Turbo FAT"; it only very vaguely resembles the DOS FAT file system. (Kai
** Henningsen (kai@khms.westfalen.de)) */

	{ 0x66, "Novell Netware SMS Partition" },

/* According to disk.c in the Netware source. SMS: Storage Management Services.
** No longer used. */

	{ 0x67, "Novell" },

/* Roman Gruber reports: this code has frozen my version of norton disk-editor
** (so I think it has to be something special). Jeff Merkey says: 67 is for
** Wolf Mountain. */

	{ 0x68, "Novell" },

	{ 0x69, "Novell Netware 5+, Novell Netware NSS Partition" },

/* According to disk.c in the Netware source. NSS = Novell Storage Services. */

	{ 0x6e, "??" },

/* Reported once. */

	{ 0x70, "DiskSecure Multi-Boot" },

	{ 0x71, "Reserved" },

	{ 0x73, "Reserved" },

	{ 0x74, "Reserved" },

	{ 0x74, "Scramdisk partition" },

/* Scramdisk is freeware and shareware disk encryption software. It supports
** container files, dedicated partitions (type 0x74) and disks hidden in WAV
** audio files. (Shaun Hollingworth (moatlane@btconnect.com)) */

	{ 0x75, "IBM PC/IX" },

	{ 0x76, "Amiga VirtualHardDrive" },

	{ 0x76, "Reserved" },

	{ 0x77, "M2FS/M2CS partition" },

/* Jeff Merkey writes: 77 is one we are using internally for M2FS/M2CS
** partitions. */

	{ 0x77, "VNDI Partition" },

/* (According to disk.c in the Netware source. Not in actual use.) */

	{ 0x78, "XOSL FS" },

/* XOSL Bootloader filesystem, see www.xosl.org. */

	{ 0x7e, "Unused" },

/* Claimed for F.I.X. by gruberr@kapsch.net. See dejanews. */

	{ 0x7f, "Unused" },

/* Proposed for the Alt-OS-Development Partition Standard. */

	{ 0x80, "MINIX until 1.4a" },

	{ 0x81, "MINIX since 1.4b, early Linux" },

/* Minix is a Unix-like operating system written by Andy Tanenbaum and students
** at the Vrije Universiteit, Amsterdam, around 1989-1991. It runs on PCs (8086
** and up), MacIntosh, Atari, Amiga, Sparc. Ref: Operating Systems: Design and
** Implementation, Andrew S. Tanenbaum, Prentice-Hall, ISBN 0-13-637406-9 Since
** 950601 Minix is freely available - site: ftp.cs.vu.nl. See also
** comp.os.minix. */

	{ 0x81, "Mitac disk manager" },

	{ 0x82, "Prime" },

	{ 0x82, "Solaris x86" },

/* Solaris creates a single partition with id 0x82, then uses Sun disk labels
** within the partition to split it further. (Brandon S. Allbery
** (allbery@kf8nh.apk.net)) Starting from 2005, newly installed systems will
** use 0xbf. */

	{ 0x82, "Linux swap" },

	{ 0x83, "Linux native partition" },

/* Linux is a Unix-like operating system written by Linus Torvalds and many
** others on the internet since Fall 1991. It runs on PCs (386 and up) and a
** variety of other hardware. It is distributed under GPL. Software can be
** found numerous places, like ftp.funet.fi, metalab.unc.edu and
** tsx-11.mit.edu. See also comp.os.linux.* and http://www.linux.org/. Various
** filesystem types like xiafs, ext2, ext3, reiserfs, etc. all use ID 83. Some
** systems mistakenly assume that 83 must mean ext2. */

	{ 0x84, "OS/2 hidden C: drive" },

/* OS/2-renumbered type 04 partition. */

	{ 0x84, "Hibernation partition" },

/* (following Appendix E of the Microsoft APM 1.1f specification). Reported for
** various laptop models. E.g., used on Dell Latitudes (with Dell BIOS) that
** use the MKS2D utility. APM 1.2 hibernation partitions can be used by Windows
** 98 or higher. */

	{ 0x85, "Linux extended partition" },

	{ 0x86, "Old Linux RAID partition superblock" },

/* See fd. */

	{ 0x86, "FAT16 volume set" },

/* Legacy Fault Tolerant FAT16 volume. Windows NT 4.0 or earlier will add 0x80
** to the partition type for partitions that are part of a Fault Tolerant set
** (mirrored or in a RAID-5 volume). Thus, one gets types 86, 87, 8b, 8c. See
** also Windows NT Boot Process and Hard Disk Constraints. */

	{ 0x87, "NTFS volume set" },

/* Legacy Fault Tolerant NTFS volume. HPFS Fault-Tolerant mirrored partition. */

	{ 0x88, "Linux plaintext partition table" },

	{ 0x8a, "Linux Kernel Partition (used by AiR-BOOT)" },

/* Martin Kiewitz (KiWi@vision.fido.de) writes: I'm currently writing a pretty
** nice boot-loader. For this I'm using Linux Boot Loader ID A0h, and
** partitition type 8Ah for the partition holding the kernel image. */

	{ 0x8b, "Legacy Fault Tolerant FAT32 volume" },

	{ 0x8c, "Legacy Fault Tolerant FAT32 volume using BIOS extd INT 13h" },

	{ 0x8d, "Free FDISK hidden Primary DOS FAT12 partitition" },

/* Free FDISK is the FDISK used by FreeDOS. It hides types 01, 04, 05, 06, 0b,
** 0c, 0e, 0f by adding decimal 140 (0x8c). */

	{ 0x8e, "Linux Logical Volume Manager partition" },

/* See pvcreate(8) as found under http://linux.msede.com/lvm. (For a while this
** was 0xfe.) */

	{ 0x90, "Free FDISK hidden Primary DOS FAT16 partitition" },

	{ 0x91, "Free FDISK hidden DOS extended partitition" },

	{ 0x92, "Free FDISK hidden Primary DOS large FAT16 partitition" },

	{ 0x93, "Hidden Linux native partition" },

	{ 0x93, "Amoeba" },

	{ 0x94, "Amoeba bad block table" },

/* Amoeba is a distributed operating system written by Andy Tanenbaum, together
** with Frans Kaashoek, Sape Mullender, Robert van Renesse and others since
** 1981. It runs on PCs (386 and up), Sun3, Sparc, 68030. It is free for
** universities for research/teaching purposes. For information, see
** ftp.cs.vu.nl. */

	{ 0x95, "MIT EXOPC native partitions" },

/* http://www.pdos.lcs.mit.edu/exo/ (Andrew Purtell, Andrew_Purtell@NAI.com) */

	{ 0x97, "Free FDISK hidden Primary DOS FAT32 partitition" },

	{ 0x98, "Free FDISK hidden Primary DOS FAT32 partitition (LBA)" },

	{ 0x98, "Datalight ROM-DOS Super-Boot Partition" },

/* See www.datalight.com, and type 12 above. */

	{ 0x99, "DCE376 logical drive" },

/* No, it's not a hibernation partition; it's closest to a DOS extended
** partition. It's used by the Mylex DCE376 EISA SCSI adaptor for partitions
** which are beyond the 1024th cylinder of a drive. I've only seen references
** to type 99 with the DCE376. (Christian Carey, ccarey@CapAccess.ORG) */

	{ 0x9a, "Free FDISK hidden Primary DOS FAT16 partitition (LBA)" },

	{ 0x9b, "Free FDISK hidden DOS extended partitition (LBA)" },

	{ 0x9f, "BSD/OS" },

/* Current sysid for BSDI. The types b7 and b8 given below are for an older
** version of the filesystem used in pre-v3.0 versions of the OS. These days
** the system is v4.1 BSD/OS. BSDI reports 2.1 million installed servers and 12
** million licenses sold. See http://www.bsdi.com/. */

	{ 0xa0, "Laptop hibernation partition" },

/* Reported for various laptops like IBM Thinkpad, Phoenix NoteBIOS, Toshiba
** under names like zero-volt suspend partition, suspend-to-disk partition,
** save-to-disk partition, power-management partition, hibernation partition.
** Usually at the start or end of the disk area. (This is also the number used
** by Sony on the VAIO. Recent VAIOs can also hibernate to a file in the
** filesystem, the choice being made from the BIOS setup screen.) */

	{ 0xa1, "Laptop hibernation partition" },

/* Reportedly used as "Save-to-Disk" partition on a NEC 6000H notebook. Types
** a0 and a1 are used on systems with Phoenix BIOS; the Phoenix PHDISK utility
** is used with these. */

	{ 0xa1, "HP Volume Expansion (SpeedStor variant)" },

/* IDs 21, a1, a3, a4, a6, b1, b3, b4, b6 are for HP Volume Expansion
** (SpeedStor variant). */

	{ 0xa3, "HP Volume Expansion (SpeedStor variant)" },

	{ 0xa4, "HP Volume Expansion (SpeedStor variant)" },

	{ 0xa5, "BSD/386, 386BSD, NetBSD, FreeBSD" },

/* 386BSD is a Unix-like operating system, a port of 4.3BSD Net/2 to the PC
** done by Bill Jolitz around 1991. When Jolitz seemed to stop development, an
** updated version was called FreeBSD (1992). The outcome of a Novell vs. UCB
** law suit was that Net/2 contained AT&T code, and hence was not free, but
** that 4.4BSD-Lite was free. After that, FreeBSD and NetBSD were restructured,
** and FreeBSD 2.0 and NetBSD 1.0 are based on 4.4BSD-Lite. FreeBSD runs on
** PCs. See http://www.freebsd.org/FreeBSD.html. For NetBSD, see below - it
** changed partition type to a9. 386BSD seems to be dead now. The kernel source
** is being published - see Operating System Source Code Secrets by Bill and
** Lynne Jolitz. See comp.os.386bsd.*. See
** http://www.paranoia.com/~vax/boot.html for NetBSD boot and partitioning
** info. */

	{ 0xa6, "OpenBSD" },

/* OpenBSD, led by Theo de Raadt, split off from NetBSD. It tries to emphasize
** on security. See http://www.openbsd.org/. */

	{ 0xa6, "HP Volume Expansion (SpeedStor variant)" },

	{ 0xa7, "NeXTStep" },

/* Based on Mach 2.6 and features of Mach 3.0, is a true object-oriented
** operating system and user environment. See http://www.next.com/. */

	{ 0xa8, "Mac OS-X" },

/* Apple's OS-X ( Darwin Intel) uses this type for its filesystem partition (a
** UFS file system, in NeXT flavour, only differing from the *BSD formats in
** the first 8 KB). See also type ab. */

	{ 0xa9, "NetBSD" },

/* NetBSD is one of the children of *BSD (see above). It runs on PCs and a
** variety of other hardware. Since 19-Feb-98 NetBSD uses a9 instead of a5. See
** http://www.netbsd.org/. It is freely obtainable - see
** http://www.netbsd.org/Sites/net.html. */

	{ 0xaa, "Olivetti Fat 12 1.44MB Service Partition" },

/* Contains a bare DOS 6.22 and a utility to exchange types 06 and aa in the
** partition table. (loekw@worldonline.nl) */

	{ 0xab, "Mac OS-X Boot partition" },

/* Apple's OS-X (Darwin Intel) uses this type for its boot partition. The image
** (/usr/standalone/i386/boot) starts at sector 1. See also type a8. */

	{ 0xab, "GO! partition" },

/* Unused. Claimed by Stanislav Karchebny for his GO! OS. */

	{ 0xae, "ShagOS filesystem" },

	{ 0xaf, "ShagOS swap partition" },

/* Unused. Claimed by Frank Barrus for his ShagOS.  */

	{ 0xb0, "BootStar Dummy" },

/* The boot manager BootStar manages its own partition table, with up to 15
** primary partitions. It fills unused entries in the MBR with BootStar Dummy
** values. See www.star-tools.com. If you use this, don't use a disk manager,
** do not put LILO in the MBR and do not use fdisk. */

	{ 0xb1, "HP Volume Expansion (SpeedStor variant)" },

	{ 0xb3, "HP Volume Expansion (SpeedStor variant)" },

	{ 0xb4, "HP Volume Expansion (SpeedStor variant)" },

	{ 0xb6, "HP Volume Expansion (SpeedStor variant)" },

	{ 0xb6, "Corrupted Windows NT mirror set (master), FAT16 file system" },

	{ 0xb7, "Corrupted Windows NT mirror set (master), NTFS file system" },

	{ 0xb7, "BSDI BSD/386 filesystem" },

	{ 0xb8, "BSDI BSD/386 swap partition" },

/* BSDI (Berkeley Software Design, Inc.) was founded by former CSRG (UCB
** Computer Systems Research Group) members. Their operating system, based on
** Net/2, was called BSD/386. After the USL (Unix System Laboratories,
** Inc./Novell Corp.) vs. BSDI lawsuit, new releases were based on BSD4.4-Lite.
** Now they are announcing BSD/OS V2.0.1. This is an operating for PCs (386 and
** up), boasting 3000 customers. (That was long ago. The current partition id
** is 9f, see above.) */

	{ 0xbb, "Boot Wizard hidden" },

/* (PTS) BootWizard 4.0 and its new version Acronis OS Selector 5.0 use this id
** (i) when hiding partitions with types other than 01, 04, 06, 07, 0b, 0c, 0e,
** and (ii) when creating a partition without file system. See
** www.PhysTechSoft.com. The boot software was purchased on 2001-01-05 by
** SWsoft. See www.acronis.com. */

	{ 0xbe, "Solaris 8 boot partition" },

	{ 0xbf, "New Solaris x86 partition" },

/* The old 0x82 id conflicted with Linux swap. New Solaris installations will
** use the id 0xbf. (Larry Lee <lclee@west.sun.com>) */

	{ 0xc0, "CTOS" },

	{ 0xc0, "REAL/32 secure small partition" },

/* See d0 below.

	{ 0xc0, "NTFT Partition" },

** According to disk.c in the Netware source. */

	{ 0xc0, "DR-DOS/Novell DOS secured partition" },

/* DR-DOS 7.02+ / OpenDOS 7.01 / Novell DOS 7 secured partition. */

	{ 0xc1, "DRDOS/secured (FAT-12)" },

	{ 0xc2, "Unused" },

/* According to Powerquest IDs c2, c3, c8, c9, ca, cd are reserved for DR-DOS
** 7+. According to Matthias Paul c2, c3, cd are no longer reserved for DR-DOS. */

	{ 0xc2, "Hidden Linux" },

	{ 0xc3, "Hidden Linux swap" },

/* Benedict Chong (bchong@blueskyinnovations.com) writes: BlueSky Innovations
** LLC does a boot manager product called Power Boot and we use, in addition,
** 0C2h and 0C3h for hidden Linux partitions (swap and ext2fs). See also ID c2. */

	{ 0xc4, "DRDOS/secured (FAT-16, < 32M)" },

	{ 0xc5, "DRDOS/secured (extended)" },

/* This ID may also be used in obscure trickery: on a shared MS-DOS / DR-DOS
** machine with DR-DOS 6.0-7.03 (so that the DR_DOS does not understand type 0f
** and the MS-DOS does not understand type c5) one may have two extended
** partitions, where each operating system sees only one. */

	{ 0xc6, "DRDOS/secured (FAT-16, >= 32M)" },

/* DR-DOS 6.0 and higher (NetWare PalmDOS 1.0, Novell DOS 7, OpenDOS 7.01,
** DR-DOS 7.02+) will add 0xc0 to the partition type for a LOGIN.EXE-secured
** partition (so that people cannot avoid the password check by booting from an
** MS-DOS floppy). Otherwise it seems that the types c1, c4, c5, c6 and d1, d4,
** d5, d6 are used precisely like 01, 04, 05, 06 (but are accepted only when
** booting from disk). */

	{ 0xc6, "Windows NT corrupted FAT16 volume/stripe set" },

/* NTFS will add 0xc0 to the partition type for disabled parts of a Fault
** Tolerant set. Thus, one gets types c6, c7. See also Windows NT Boot Process
** and Hard Disk Constraints and Switching from DR-DOS 6.0 to MS-DOS 5.0. */

	{ 0xc7, "Windows NT corrupted NTFS volume/stripe set" },

	{ 0xc7, "Syrinx boot" },

/* Primary partition only. */

	{ 0xc8, "Reserved for DR-DOS 8.0+" },

	{ 0xc9, "Reserved for DR-DOS 8.0+" },

	{ 0xca, "Reserved for DR-DOS 8.0+" },

	{ 0xcb, "DR-DOS 7.04+ secured FAT32 (CHS)/" },

	{ 0xcc, "DR-DOS 7.04+ secured FAT32 (LBA)/" },

	{ 0xcd, "CTOS Memdump?" },

	{ 0xce, "DR-DOS 7.04+ FAT16X (LBA)/" },

	{ 0xcf, "DR-DOS 7.04+ secured EXT DOS (LBA)/" },

	{ 0xd0, "REAL/32 secure big partition" },

/* REAL/32 is a continuation of DR Multiuser DOS. It supports FAT12, FAT16 and
** REAL/32 7.90 also supports FAT32. Andrew Freeman (afreeman@imsltd.com)
** writes: For partitions which have been marked as secure we use 0xC0 and 0xD0
** as partition markers (C0 < 32mb, D0 >= 32mb). REAL/32 is an advanced 32-bit
** multitasking & multi-user MS-DOS & Windows compatible operating system. Home
** page is www.imsltd.com. */

	{ 0xd0, "Multiuser DOS secured partition" },

/* This applies to the whole MDOS family range, Digital Research DR Multiuser
** DOS and Novell DR Multiuser DOS, as well as to Concurrent Controls Multiuser
** DOS, Datapaq Australasia System Manager 7, and IMS Multiuser DOS. */

	{ 0xd1, "Old Multiuser DOS secured FAT12" },

	{ 0xd4, "Old Multiuser DOS secured FAT16 <32M" },

	{ 0xd5, "Old Multiuser DOS secured extended partition" },

	{ 0xd6, "Old Multiuser DOS secured FAT16 >=32M" },

	{ 0xd8, "CP/M-86" },

	{ 0xda, "Non-FS Data" },

/* Added on request of John Hardin (johnh@aproposretail.com). */

	{ 0xdb, "Digital Research CP/M, Concurrent CP/M, Concurrent DOS" },

	{ 0xdb, "CTOS (Convergent Technologies OS -Unisys)" },

	{ 0xdb, "KDG Telemetry SCPU boot" },

/* Mark Morgan Lloyd (markMLl.in@telemetry.co.uk) writes: KDG Telemetry uses
** type 0xdb to store a protected-mode binary image of the code to be run on a
** 'x86-based SCPU (Supervisory CPU) module from the DT800 range. */

	{ 0xdd, "Hidden CTOS Memdump?" },

	{ 0xde, "Dell PowerEdge Server utilities (FAT fs)" },

	{ 0xdf, "DG/UX virtual disk manager partition" },

/* Glenn Steen (glenn.steen@ap1.se) writes: When I made an old Aviion 2000
** triple-boot (DOS, DG/UX and Linux) I saw that Linux fdisk reported the DG/UX
** virtual disk manager partition as type 0xdf. */

	{ 0xdf, "BootIt EMBRM" },

/* The boot manager BootIt manages its own partition table, with up to 255
** primary partitions. See www.terabyteunlimited.com. If you use this, don't
** use a disk manager, do not put LILO in the MBR and do not use fdisk.
** Reference for the ID: BOOTIT.TXT. */

	{ 0xe0, "Reserved by STMicroelectronics for a filesystem called ST AVFS." },

	{ 0xe1, "DOS access or SpeedStor 12-bit FAT extended partition" },

/* Kevin Cummings reports in alt.os.linux: it's a SSTOR partition on cylinders
** > 1023. */

	{ 0xe3, "DOS R/O or SpeedStor" },

	{ 0xe4, "SpeedStor 16-bit FAT extended partition < 1024 cyl." },

	{ 0xe5, "Tandy MSDOS with logically sectored FAT" },

	{ 0xe6, "Storage Dimensions SpeedStor" },

	{ 0xeb, "BeOS BFS" },

/* BeOS is an operating system that runs on Power PCs and on Intel PCs. Version
** 5 (the last version) is distributed freely to individuals. The system was
** sold to Palm and is not developed any more. OpenBeOS tries to create an open
** source version. */

	{ 0xec, "SkyOS SkyFS" },

/* SkyOS is an operating system written by Robert Szeleney. Its filesystem
** SkyFS is based on OpenBeFS. */

	{ 0xed, "Unused" },

/* Matthias Paul plans to use this for an OS called Sprytix. */

	{ 0xee, "Indication that this legacy MBR is followed by an EFI header" },

	{ 0xef, "Partition that contains an EFI file system" },

/* Bob Griswold (rogris@Exchange.Microsoft.com) writes: MS plans on using EE
** and EF in the future for support of non-legacy BIOS booting. Mark Doran
** (mark.doran@intel.com) adds: these types are used to support the Extensible
** Firmware Interface specification (EFI); go to developer.intel.com and search
** for EFI. (For the types ee and ef, see Tables 16-6 and 16-7 of the EFI
** specification, EFISpec_091.pdf.) */

	{ 0xf0, "Linux/PA-RISC boot loader" },

/* Paul Bame (bame@debian.org) writes: the F0 partition will be located in the
** first 2GB of a drive and used to store the Linux/PA-RISC boot loader and
** boot command line, optionally including a kernel and ramdisk. */

	{ 0xf1, "Storage Dimensions SpeedStor" },

	{ 0xf2, "DOS 3.3+ secondary partition" },

/* Matthias Paul writes: "This ID was originally used by Sperry IT MS-DOS 3.xx
** for a logically sectored variant of FAT. When Sperry IT became part of
** Unisys, the operating system was called Unisys MS-DOS 3.3. Digital
** Research's DOS Plus 2.1 (for OEM machines such as the Amstrad/Schneider
** PC1512, the T.R.A.N. Jasmin Turbo (Speed 8M), or the Acorn BBC Master 512
** also supports this ID and logs it in, as if this would be either a type 01h
** FAT12 or a type 04h FAT16 partition." */

	{ 0xf3, "Reserved" },

/* Powerquest writes: Storage Dimensions SpeedStor. */

	{ 0xf4, "SpeedStor large partition" },

	{ 0xf4, "Prologue single-volume partition" },

	{ 0xf5, "Prologue multi-volume partition" },

/* The type F4 partition contains one volume, and is not used anymore. The type
** F5 partition contains 1 to 10 volumes (called MD0 to MD9). It supports one
** or more systems (Prologue 3, 4, 5, Twin Server). Each volume can have as
** file system the NGF file system or TwinFS file system. NGF (old): volume
** size at most 512 MB, at most 895 files per directory, at most 256
** directories per volume. TwinFS (new): volume size up to 4 GB. No limit in
** number of files and directories. See Prologue. */

	{ 0xf6, "Storage Dimensions SpeedStor" },

	{ 0xf7, "Unused" },

/* Maybe Natalia Portillo plans to use this for O.S.G. EFAT ("Enhanced File
** Allocation Techniques"). */

	{ 0xf9, "pCache" },

/* Ed Sawicki writes: "We propose using the F9 partition type as a pCache
** partition, which is our name for an "ext2/ext3 persistent cache partition".
** See www.alcpress.com. */

	{ 0xfa, "Bochs" },

/* Rob Judd writes: MandrakeSoft's Bochs x86 emulator (similar to VMWare) uses
** fa as a partition identifier. */

	{ 0xfb, "VMware File System partition" },

	{ 0xfc, "VMware Swap partition" },

/* VMware offers virtual machines in which one can run Linux, Windows, FreeBSD.
** These partition IDs announced by Dan Scales (scales@vmware.com). */

	{ 0xfd, "Linux raid partition with autodetect using persistent superblock" },

/* See the HOWTO and the kernel patches. Earlier, 86 was used instead of fd. */

/* Powerquest writes: Reserved for FreeDOS ( www.freedos.org), but it seems
** FreeDOS never used this ID. */

	{ 0xfe, "SpeedStor > 1024 cyl." },

	{ 0xfe, "LANstep" },

	{ 0xfe, "IBM PS/2 IML (Initial Microcode Load) partition, located at the end of the disk." },

	{ 0xfe, "Windows NT Disk Administrator hidden partition" },

/* Mark Morgan Lloyd (markMLl.in@telemetry.co.uk) writes: Windows NT Disk
** Administrator marks hidden partitions (i.e. present but not to be accessed)
** as type 0xfe. A primary partition of this type is also used by IBM to hold
** an image of the "Reference Diskettes" on many of their machines,
** particularly newer PS/2 systems (at a rough guess, anything built after
** about 1994). This clash can cause major confusion and grief if running NT on
** IBM kit. When this Reference Partition is activated, it changes its type
** into 1 (FAT12) and hides all other partitions by adding 0x10 to the type. */

	{ 0xfe, "Linux Logical Volume Manager partition (old)" },

/* This has been in use since the early LVM */

	{ 0xff, "Xenix Bad Block Table" },

/**************************************************************************
** RDB partitions
**************************************************************************/

	{ ID_NTFS_DISK, "NTFS" }, /* by Marek Szyprowski <marek@amiga.pl> */
	{ ID_EXT2_DISK, "EXT2" }, /* by Marek Szyprowski <marek@amiga.pl> */
	{ ID_XFS0_DISK, "XFS FileSystem" }, /* by Marek Szyprowski <marek@amiga.pl> */

	{ ID_MSDOS_DISK, "MS-DOS - CrossDosFileSystem (MSD\\0)" },
	{ ID_MSH_DISK, "MS-DOS - CrossDOSFileSystem (MSH\\0)" },

	{ ID_FAT0_DISK, "MS-DOS - FAT95 (FAT\\0)" },
	{ ID_FAT1_DISK, "MS-DOS - FAT95 (FAT\\1)" },
	{ ID_FAT2_DISK, "MS-DOS - FAT95 (FAT\\2)" },
	{ ID_FAT3_DISK, "MS-DOS - FAT95 (FAT\\3)" },
	{ ID_FAT4_DISK, "MS-DOS - FAT95 (FAT\\4)" },
	{ ID_FAT5_DISK, "MS-DOS - FAT95 (FAT\\5)" },
	{ ID_FAT6_DISK, "MS-DOS - FAT95 (FAT\\6)" },
	{ ID_FAT7_DISK, "MS-DOS - FAT95 (FAT\\7)" },

	{ ID_CFS_DISK, "CustomFileSystem" },
	{ ID_DOS_DISK, "Old File System (DOS\\0)" },
	{ ID_NOT_REALLY_DOS, "No File System (NDOS)" },
	{ ID_FFS_DISK, "Fast File System (DOS\\1)" },
	{ ID_INTER_DOS_DISK, "Old File System (DOS\\2)" },
	{ ID_INTER_FFS_DISK, "Fast File System (DOS\\3)" },
	{ ID_FASTDIR_DOS_DISK, "Old File System (DOS\\4)" },
	{ ID_FASTDIR_FFS_DISK, "Fast File System (DOS\\5)" },
	{ ID_FASTDIRINTER_DOS_DISK, "Old File System (DOS\\6)" },
	{ ID_FASTDIRINTER_FFS_DISK, "Fast File System (DOS\\7)" },
	{ ID_KICKSTART_DISK, "KickStart (KICK)" },

	{ FSID_TERMINATED, "Unknown" }

};                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     





/* \\\ */

/*************************************************************************/


/*
** Our partition list
*/

struct List partitionlist;

ULONG partitionnumber;

/*
** Our device structure
*/

struct MDevice device = {NULL};

/*
** our nsd check
*/

BOOL device64;
