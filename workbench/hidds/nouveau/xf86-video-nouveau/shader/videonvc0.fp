#ifndef ENVYAS
static uint32_t
NVC0FP_NV12[] = {
	0x00021462,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x80000000,
	0x00000a0a,
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
#include "videonvc0.fpc"
};
#else

interp pass f32 $r2 a[0x7c] 0x0 0x0
rcp f32 $r2 $r2
interp mul f32 $r0 a[0x80] $r2 0x0
interp mul f32 $r1 a[0x84] $r2 0x0
tex t lauto live dfp #:#:#:$r0 t2d $t0 $s0 $r0:$r1 ()
mul ftz rn f32 $r5 $r0 c0[0x0]
add ftz rn f32 $r3 $r5 c0[0x4]
add ftz rn f32 $r4 $r5 c0[0x8]
add ftz rn f32 $r5 $r5 c0[0xc]
interp mul f32 $r0 a[0x80] $r2 0x0
interp mul f32 $r1 a[0x84] $r2 0x0
tex t lauto live dfp #:#:$r0:$r1 t2d $t1 $s0 $r0:$r1 ()
fma ftz rn f32 $r3 $r0 c0[0x10] $r3
fma ftz rn f32 $r4 $r0 c0[0x14] $r4
fma ftz rn f32 $r5 $r0 c0[0x18] $r5
fma ftz rn f32 $r0 $r1 c0[0x1c] $r3
fma ftz rn f32 $r2 $r1 c0[0x24] $r5
fma ftz rn f32 $r1 $r1 c0[0x20] $r4
exit
#endif
