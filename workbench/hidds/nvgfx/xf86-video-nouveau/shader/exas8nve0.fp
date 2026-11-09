#ifndef ENVYAS
static uint32_t
NVE0FP_Source_A8[] = {
	0x00021462,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x80000000,
	0x0000000a,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x0000000f,
	0x00000000,
#include "exas8nve0.fpc"
};
#else

interp pass f32 $r0 a[0x7c] 0x0 0x0
rcp f32 $r0 $r0
interp mul f32 $r1 a[0x84] $r0 0x0
interp mul f32 $r0 a[0x80] $r0 0x0
tex t lauto live dfp #:#:#:$r0 t2d $t0 $s0 $r0:$r1 ()
texbar 0x0
long mov b32 $r3 $r0
long mov b32 $r2 $r0
long mov b32 $r1 $r0
long exit
#endif
