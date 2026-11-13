VERSION = 50
REVISION = 3

.macro DATE
.ascii "20.11.2007"
.endm

.macro VERS
.ascii "wave.datatype 50.3"
.endm

.macro VSTRING
.ascii "wave.datatype 50.3 (20.11.2007)"
.byte 13,10,0
.endm

.macro VERSTAG
.byte 0
.ascii "$VER: wave.datatype 50.3 (20.11.2007)"
.byte 0
.endm
