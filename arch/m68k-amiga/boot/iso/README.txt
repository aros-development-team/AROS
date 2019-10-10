AROS m68k-amiga
---------------

This disk (image) contains AROS
for the amiga-m68k architecture.

To boot, you can:

1. copy the content of this image or archive to a bootable
partition, best case with the file system installed to the
RDB (Rigid Disk Block) of your media, and attach it to your 
Amiga via an autoconfig mass storage controller (internal
IDE ports or most SCSI/IDE expansions will do) , or

2. image the 'Emergency-Boot.adf' file to a floppy,
or copy the contents of the 'Emergency-Boot' directory
to an empty OFS formatted floppy, and run 'C:Install DF0:'

Whether you boot with the AROS Emergency-Boot floppy, 
or directly from your boot partition, it will start the
AROSBootstrap program, which will softkick the AROS
relocatable ROM image (aros.hunk.gz) into the RAM.

The Emergency-Boot should boot on any Amiga hardware with
KickStart 1.3 or later, and at least 2M of KICK RAM, and
a total of 5M of RAM to run Wanderer.

If you have MAPROM style hardware, or want to burn an
EEPROM, you can use the aros-rom.bin and aros-ext.bin
files. Both 512K images must be present in the Amiga
address space as follows:

  0xE00000 aros-ext.bin  (512K)
  0xF80000 aros-rom.bin  (512K)

None of the programs on this disk are expected to operate
under any version of AmigaOS at this time, except for
AROSBootstrap.

Enjoy this technology preview!
None of the programs on this disk are expected to operate
under any version of AmigaOS at this time, except for
AROSBootstrap.

Enjoy this technology preview!
