# Put markers at the end of all data sections.

.section	.rodata
.globl	Purify_Endrodata
Purify_Endrodata:
	.string "P"

.data
.stabs "Purify_Enddata:G1",32,0,4,0
	.align 4
	.type	 Purify_Enddata,@object
	.size	 Purify_Enddata,4
.globl	Purify_Enddata
Purify_Enddata:
	.long 0

	.local	_Purify_Endstaticdata
	.comm	_Purify_Endstaticdata,512,4

.section	.rodata
.globl Purify_Endstaticdata
Purify_Endstaticdata:
	.int _Purify_Endstaticdata

	.comm Purify_Endbss,4,4
