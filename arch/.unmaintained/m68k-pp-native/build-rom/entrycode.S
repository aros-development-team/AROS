	.section entrycodes,"aw",@progbits

	// For the following structur have a look at the POSE source
	// code SrcShared/Palm/Platform/Core/System/IncsPrv/MemoryPrv.h

init_ssp:
	.dc.l  0xfffc                 // initial value for SSP
	.dc.l  hardware_reset_init_1  // where the startup code is at.
	.dc.l  0xfeedbeef             // signature
	.dc.w  0x01                   // hdrVersion
	.dc.w  0x00                   // flags
	.dc.b  'A','R','O','S',' ','f','o','r'
	.dc.b  ' ','P','A','L','M',' ','d','e'
	.dc.b  'v','i','c','e','s',' ','(','A'
	.dc.b  'R','O','S',' ','D','e','v',')' // name - exactly 32 bytes
	.dc.b  'A','R','O','S',' ','f','o','r'
	.dc.b  ' ','P','A','L','M',' ','d','e'
	.dc.b  'v','i','c','e','s',' ','(','A'
	.dc.b  'R','O','S',' ','D','e','v',')' // name - exactly 32 bytes
        .dc.w  1                      // version
        .dc.l  00                     // creation date
        .dc.w  1                      // numRAMBlocks
        .dc.l  0x00000000             // blockListOffset
        .dc.l  0                      // readwriteparmsoffset
        .dc.l  0                      // readwriteparmssize
        .dc.l  0                      // readonlyparmsoffset
        .dc.l  0                      // bigROMOffset
        .dc.l  0                      // checksumBytes
        .dc.w  0                      // checksumValue


	.section entrycodes_bigrom,"aw",@progbits
	.dc.l  0xfffc                 // initial value for SSP
	.dc.l  hardware_reset_init_1  // where the startup code is at.
	.dc.l  0xfeedbeef             // signature
	.dc.w  0x01                   // hdrVersion
	.dc.w  0x00                   // flags
	.dc.b  'A','R','O','S',' ','f','o','r'
	.dc.b  ' ','P','A','L','M',' ','d','e'
	.dc.b  'v','i','c','e','s',' ','(','A'
	.dc.b  'R','O','S',' ','D','e','v',')' // name - exactly 32 bytes
	.dc.b  'A','R','O','S',' ','f','o','r'
	.dc.b  ' ','P','A','L','M',' ','d','e'
	.dc.b  'v','i','c','e','s',' ','(','A'
	.dc.b  'R','O','S',' ','D','e','v',')' // name - exactly 32 bytes
        .dc.w  1                      // version
        .dc.l  00                     // creation date
        .dc.w  1                      // numRAMBlocks
        .dc.l  0x00000000             // blockListOffset
        .dc.l  0                      // readwriteparmsoffset
        .dc.l  0                      // readwriteparmssize
        .dc.l  0                      // readonlyparmsoffset
        .dc.l  0                      // bigROMOffset
        .dc.l  0                      // checksumBytes
        .dc.w  0                      // checksumValue


	.globl hardware_reset_init_1
	.section .text
	.align 8
hardware_reset_init_1:
        jmp hardware_reset_init_2.w(%pc)       // needs to be exactly this
	.dc.b  'b','o','o','t',0x27,0x10,0xff  // way, so xcopilot likes the
                                               // rom

	.globl hardware_reset_init2
	.align 8
hardware_reset_init_2:
	jmp aros_reset;

	.globl software_reset
	.align 8
software_reset:
	lea    init_ssp,%a0
	move.l (%a0),%ssp
	jmp    aros_reset
