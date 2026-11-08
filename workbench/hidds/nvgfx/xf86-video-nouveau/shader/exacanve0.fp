#ifndef ENVYAS
static uint32_t
NVE0FP_CAComposite[] = {
	0x00021462, /* 0x0000c000 = USES_KIL, MULTI_COLORS */
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x80000000, /* FRAG_COORD_UMASK = 0x8 */
	0x00000a0a, /* FP_INTERP[0x080], 0022 0022 */
	0x00000000, /* FP_INTERP[0x0c0], 0 = OFF */
	0x00000000, /* FP_INTERP[0x100], 1 = FLAT */
	0x00000000, /* FP_INTERP[0x140], 2 = PERSPECTIVE */
	0x00000000, /* FP_INTERP[0x180], 3 = LINEAR */
	0x00000000, /* FP_INTERP[0x1c0] */
	0x00000000, /* FP_INTERP[0x200] */
	0x00000000, /* FP_INTERP[0x240] */
	0x00000000, /* FP_INTERP[0x280] */
	0x00000000, /* FP_INTERP[0x2c0] */
	0x00000000, /* FP_INTERP[0x300] */
	0x00000000,
	0x0000000f, /* FP_RESULT_MASK (0x8000 Face ?) */
	0x00000000, /* 0x2 = FragDepth, 0x1 = SampleMask */
#include "exacanve0.fpc"
};
#else

interp pass f32 $r0 a[0x7c] 0x0 0x0
rcp f32 $r0 $r0
interp mul f32 $r3 a[0x94] $r0 0x0
interp mul f32 $r2 a[0x90] $r0 0x0
tex t lauto live dfp $r4:$r5:$r6:$r7 t2d $t1 $s0 $r2:$r3 ()
interp mul f32 $r1 a[0x84] $r0 0x0
interp mul f32 $r0 a[0x80] $r0 0x0
tex t lauto live dfp $r0:$r1:$r2:$r3 t2d $t0 $s0 $r0:$r1 ()
texbar 0x0
mul ftz rn f32 $r3 $r3 $r7
mul ftz rn f32 $r2 $r2 $r6
mul ftz rn f32 $r1 $r1 $r5
mul ftz rn f32 $r0 $r0 $r4
long exit
#endif
