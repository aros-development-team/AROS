# Put markers at the begin of all data sections.

.section	.rodata
.globl Purify_Beginrodata
Purify_Beginrodata:
	.string "P"

.data
.stabs "Purify_Begindata:G1",32,0,4,0
	.align 4
	.type	 Purify_Begindata,@object
	.size	 Purify_Begindata,4
.globl Purify_Begindata
Purify_Begindata:
	.long 0

	.local	_Purify_Beginstaticdata
	.comm	_Purify_Beginstaticdata,512,4

.section	.rodata
.globl Purify_Beginstaticdata
Purify_Beginstaticdata:
	.int _Purify_Beginstaticdata

	.comm Purify_Beginbss,4,4
