/* -*- c-basic-offset: 4 -*- */
/*
 * Copyright © 2006,2010 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *    Chris Wilson <chris@chris-wilson.co.uk>
 *
 */

/* Each instruction is 3 dwords long, though most don't require all
 * this space.  Maximum of 123 instructions.  Smaller maxes per insn
 * type.
 */
#define _3DSTATE_PIXEL_SHADER_PROGRAM    (CMD_3D|(0x1d<<24)|(0x5<<16))

#define REG_TYPE_R                 0 /* temporary regs, no need to
				      * dcl, must be written before
				      * read -- Preserved between
				      * phases.
				      */
#define REG_TYPE_T                 1 /* Interpolated values, must be
				      * dcl'ed before use.
				      *
				      * 0..7: texture coord,
				      * 8: diffuse spec,
				      * 9: specular color,
				      * 10: fog parameter in w.
				      */
#define REG_TYPE_CONST             2 /* Restriction: only one const
				      * can be referenced per
				      * instruction, though it may be
				      * selected for multiple inputs.
				      * Constants not initialized
				      * default to zero.
				      */
#define REG_TYPE_S                 3 /* sampler */
#define REG_TYPE_OC                4 /* output color (rgba) */
#define REG_TYPE_OD                5 /* output depth (w), xyz are
				      * temporaries.  If not written,
				      * interpolated depth is used?
				      */
#define REG_TYPE_U                 6 /* unpreserved temporaries */
#define REG_TYPE_MASK              0x7
#define REG_TYPE_SHIFT		   4
#define REG_NR_MASK                0xf

/* REG_TYPE_T:
*/
#define T_TEX0     0
#define T_TEX1     1
#define T_TEX2     2
#define T_TEX3     3
#define T_TEX4     4
#define T_TEX5     5
#define T_TEX6     6
#define T_TEX7     7
#define T_DIFFUSE  8
#define T_SPECULAR 9
#define T_FOG_W    10		/* interpolated fog is in W coord */

/* Arithmetic instructions */

/* .replicate_swizzle == selection and replication of a particular
 * scalar channel, ie., .xxxx, .yyyy, .zzzz or .wwww
 */
#define A0_NOP    (0x0<<24)		/* no operation */
#define A0_ADD    (0x1<<24)		/* dst = src0 + src1 */
#define A0_MOV    (0x2<<24)		/* dst = src0 */
#define A0_MUL    (0x3<<24)		/* dst = src0 * src1 */
#define A0_MAD    (0x4<<24)		/* dst = src0 * src1 + src2 */
#define A0_DP2ADD (0x5<<24)		/* dst.xyzw = src0.xy dot src1.xy + src2.replicate_swizzle */
#define A0_DP3    (0x6<<24)		/* dst.xyzw = src0.xyz dot src1.xyz */
#define A0_DP4    (0x7<<24)		/* dst.xyzw = src0.xyzw dot src1.xyzw */
#define A0_FRC    (0x8<<24)		/* dst = src0 - floor(src0) */
#define A0_RCP    (0x9<<24)		/* dst.xyzw = 1/(src0.replicate_swizzle) */
#define A0_RSQ    (0xa<<24)		/* dst.xyzw = 1/(sqrt(abs(src0.replicate_swizzle))) */
#define A0_EXP    (0xb<<24)		/* dst.xyzw = exp2(src0.replicate_swizzle) */
#define A0_LOG    (0xc<<24)		/* dst.xyzw = log2(abs(src0.replicate_swizzle)) */
#define A0_CMP    (0xd<<24)		/* dst = (src0 >= 0.0) ? src1 : src2 */
#define A0_MIN    (0xe<<24)		/* dst = (src0 < src1) ? src0 : src1 */
#define A0_MAX    (0xf<<24)		/* dst = (src0 >= src1) ? src0 : src1 */
#define A0_FLR    (0x10<<24)		/* dst = floor(src0) */
#define A0_MOD    (0x11<<24)		/* dst = src0 fmod 1.0 */
#define A0_TRC    (0x12<<24)		/* dst = int(src0) */
#define A0_SGE    (0x13<<24)		/* dst = src0 >= src1 ? 1.0 : 0.0 */
#define A0_SLT    (0x14<<24)		/* dst = src0 < src1 ? 1.0 : 0.0 */
#define A0_DEST_SATURATE                 (1<<22)
#define A0_DEST_TYPE_SHIFT                19
/* Allow: R, OC, OD, U */
#define A0_DEST_NR_SHIFT                 14
/* Allow R: 0..15, OC,OD: 0..0, U: 0..2 */
#define A0_DEST_CHANNEL_X                (1<<10)
#define A0_DEST_CHANNEL_Y                (2<<10)
#define A0_DEST_CHANNEL_Z                (4<<10)
#define A0_DEST_CHANNEL_W                (8<<10)
#define A0_DEST_CHANNEL_ALL              (0xf<<10)
#define A0_DEST_CHANNEL_SHIFT            10
#define A0_SRC0_TYPE_SHIFT               7
#define A0_SRC0_NR_SHIFT                 2

#define A0_DEST_CHANNEL_XY              (A0_DEST_CHANNEL_X|A0_DEST_CHANNEL_Y)
#define A0_DEST_CHANNEL_XYZ             (A0_DEST_CHANNEL_XY|A0_DEST_CHANNEL_Z)

#define SRC_X        0
#define SRC_Y        1
#define SRC_Z        2
#define SRC_W        3
#define SRC_ZERO     4
#define SRC_ONE      5

#define A1_SRC0_CHANNEL_X_NEGATE         (1<<31)
#define A1_SRC0_CHANNEL_X_SHIFT          28
#define A1_SRC0_CHANNEL_Y_NEGATE         (1<<27)
#define A1_SRC0_CHANNEL_Y_SHIFT          24
#define A1_SRC0_CHANNEL_Z_NEGATE         (1<<23)
#define A1_SRC0_CHANNEL_Z_SHIFT          20
#define A1_SRC0_CHANNEL_W_NEGATE         (1<<19)
#define A1_SRC0_CHANNEL_W_SHIFT          16
#define A1_SRC1_TYPE_SHIFT               13
#define A1_SRC1_NR_SHIFT                 8
#define A1_SRC1_CHANNEL_X_NEGATE         (1<<7)
#define A1_SRC1_CHANNEL_X_SHIFT          4
#define A1_SRC1_CHANNEL_Y_NEGATE         (1<<3)
#define A1_SRC1_CHANNEL_Y_SHIFT          0

#define A2_SRC1_CHANNEL_Z_NEGATE         (1<<31)
#define A2_SRC1_CHANNEL_Z_SHIFT          28
#define A2_SRC1_CHANNEL_W_NEGATE         (1<<27)
#define A2_SRC1_CHANNEL_W_SHIFT          24
#define A2_SRC2_TYPE_SHIFT               21
#define A2_SRC2_NR_SHIFT                 16
#define A2_SRC2_CHANNEL_X_NEGATE         (1<<15)
#define A2_SRC2_CHANNEL_X_SHIFT          12
#define A2_SRC2_CHANNEL_Y_NEGATE         (1<<11)
#define A2_SRC2_CHANNEL_Y_SHIFT          8
#define A2_SRC2_CHANNEL_Z_NEGATE         (1<<7)
#define A2_SRC2_CHANNEL_Z_SHIFT          4
#define A2_SRC2_CHANNEL_W_NEGATE         (1<<3)
#define A2_SRC2_CHANNEL_W_SHIFT          0

/* Texture instructions */
#define T0_TEXLD     (0x15<<24)	/* Sample texture using predeclared
				 * sampler and address, and output
				 * filtered texel data to destination
				 * register */
#define T0_TEXLDP    (0x16<<24)	/* Same as texld but performs a
				 * perspective divide of the texture
				 * coordinate .xyz values by .w before
				 * sampling. */
#define T0_TEXLDB    (0x17<<24)	/* Same as texld but biases the
				 * computed LOD by w.  Only S4.6 two's
				 * comp is used.  This implies that a
				 * float to fixed conversion is
				 * done. */
#define T0_TEXKILL   (0x18<<24)	/* Does not perform a sampling
				 * operation.  Simply kills the pixel
				 * if any channel of the address
				 * register is < 0.0. */
#define T0_DEST_TYPE_SHIFT                19
/* Allow: R, OC, OD, U */
/* Note: U (unpreserved) regs do not retain their values between
 * phases (cannot be used for feedback)
 *
 * Note: oC and OD registers can only be used as the destination of a
 * texture instruction once per phase (this is an implementation
 * restriction).
 */
#define T0_DEST_NR_SHIFT                 14
/* Allow R: 0..15, OC,OD: 0..0, U: 0..2 */
#define T0_SAMPLER_NR_SHIFT              0 /* This field ignored for TEXKILL */
#define T0_SAMPLER_NR_MASK               (0xf<<0)

#define T1_ADDRESS_REG_TYPE_SHIFT        24 /* Reg to use as texture coord */
/* Allow R, T, OC, OD -- R, OC, OD are 'dependent' reads, new program phase */
#define T1_ADDRESS_REG_NR_SHIFT          17
#define T2_MBZ                           0

/* Declaration instructions */
#define D0_DCL       (0x19<<24)	/* Declare a t (interpolated attrib)
				 * register or an s (sampler)
				 * register. */
#define D0_SAMPLE_TYPE_SHIFT              22
#define D0_SAMPLE_TYPE_2D                 (0x0<<22)
#define D0_SAMPLE_TYPE_CUBE               (0x1<<22)
#define D0_SAMPLE_TYPE_VOLUME             (0x2<<22)
#define D0_SAMPLE_TYPE_MASK               (0x3<<22)

#define D0_TYPE_SHIFT                19
/* Allow: T, S */
#define D0_NR_SHIFT                  14
/* Allow T: 0..10, S: 0..15 */
#define D0_CHANNEL_X                (1<<10)
#define D0_CHANNEL_Y                (2<<10)
#define D0_CHANNEL_Z                (4<<10)
#define D0_CHANNEL_W                (8<<10)
#define D0_CHANNEL_ALL              (0xf<<10)
#define D0_CHANNEL_NONE             (0<<10)

#define D0_CHANNEL_XY               (D0_CHANNEL_X|D0_CHANNEL_Y)
#define D0_CHANNEL_XYZ              (D0_CHANNEL_XY|D0_CHANNEL_Z)

/* I915 Errata: Do not allow (xz), (xw), (xzw) combinations for diffuse
 * or specular declarations.
 *
 * For T dcls, only allow: (x), (xy), (xyz), (w), (xyzw)
 *
 * Must be zero for S (sampler) dcls
 */
#define D1_MBZ                          0
#define D2_MBZ                          0


/* MASK_* are the unshifted bitmasks of the destination mask in arithmetic
 * operations
 */
#define MASK_X			0x1
#define MASK_Y			0x2
#define MASK_Z			0x4
#define MASK_W			0x8
#define MASK_XYZ		(MASK_X | MASK_Y | MASK_Z)
#define MASK_XYZW		(MASK_XYZ | MASK_W)
#define MASK_SATURATE		0x10

/* Temporary, undeclared regs. Preserved between phases */
#define FS_R0			((REG_TYPE_R << REG_TYPE_SHIFT) | 0)
#define FS_R1			((REG_TYPE_R << REG_TYPE_SHIFT) | 1)
#define FS_R2			((REG_TYPE_R << REG_TYPE_SHIFT) | 2)
#define FS_R3			((REG_TYPE_R << REG_TYPE_SHIFT) | 3)

/* Texture coordinate regs.  Must be declared. */
#define FS_T0			((REG_TYPE_T << REG_TYPE_SHIFT) | 0)
#define FS_T1			((REG_TYPE_T << REG_TYPE_SHIFT) | 1)
#define FS_T2			((REG_TYPE_T << REG_TYPE_SHIFT) | 2)
#define FS_T3			((REG_TYPE_T << REG_TYPE_SHIFT) | 3)
#define FS_T4			((REG_TYPE_T << REG_TYPE_SHIFT) | 4)
#define FS_T5			((REG_TYPE_T << REG_TYPE_SHIFT) | 5)
#define FS_T6			((REG_TYPE_T << REG_TYPE_SHIFT) | 6)
#define FS_T7			((REG_TYPE_T << REG_TYPE_SHIFT) | 7)
#define FS_T8			((REG_TYPE_T << REG_TYPE_SHIFT) | 8)
#define FS_T9			((REG_TYPE_T << REG_TYPE_SHIFT) | 9)
#define FS_T10			((REG_TYPE_T << REG_TYPE_SHIFT) | 10)

/* Constant values */
#define FS_C0			((REG_TYPE_CONST << REG_TYPE_SHIFT) | 0)
#define FS_C1			((REG_TYPE_CONST << REG_TYPE_SHIFT) | 1)
#define FS_C2			((REG_TYPE_CONST << REG_TYPE_SHIFT) | 2)
#define FS_C3			((REG_TYPE_CONST << REG_TYPE_SHIFT) | 3)
#define FS_C4			((REG_TYPE_CONST << REG_TYPE_SHIFT) | 4)
#define FS_C5			((REG_TYPE_CONST << REG_TYPE_SHIFT) | 5)
#define FS_C6			((REG_TYPE_CONST << REG_TYPE_SHIFT) | 6)
#define FS_C7			((REG_TYPE_CONST << REG_TYPE_SHIFT) | 7)

/* Sampler regs */
#define FS_S0			((REG_TYPE_S << REG_TYPE_SHIFT) | 0)
#define FS_S1			((REG_TYPE_S << REG_TYPE_SHIFT) | 1)
#define FS_S2			((REG_TYPE_S << REG_TYPE_SHIFT) | 2)
#define FS_S3			((REG_TYPE_S << REG_TYPE_SHIFT) | 3)

/* Output color */
#define FS_OC			((REG_TYPE_OC << REG_TYPE_SHIFT) | 0)

/* Output depth */
#define FS_OD			((REG_TYPE_OD << REG_TYPE_SHIFT) | 0)

/* Unpreserved temporary regs */
#define FS_U0			((REG_TYPE_U << REG_TYPE_SHIFT) | 0)
#define FS_U1			((REG_TYPE_U << REG_TYPE_SHIFT) | 1)
#define FS_U2			((REG_TYPE_U << REG_TYPE_SHIFT) | 2)
#define FS_U3			((REG_TYPE_U << REG_TYPE_SHIFT) | 3)

#define X_CHANNEL_SHIFT (REG_TYPE_SHIFT + 3)
#define Y_CHANNEL_SHIFT (X_CHANNEL_SHIFT + 4)
#define Z_CHANNEL_SHIFT (Y_CHANNEL_SHIFT + 4)
#define W_CHANNEL_SHIFT (Z_CHANNEL_SHIFT + 4)

#define REG_CHANNEL_MASK 0xf

#define REG_NR(reg)		((reg) & REG_NR_MASK)
#define REG_TYPE(reg)		(((reg) >> REG_TYPE_SHIFT) & REG_TYPE_MASK)
#define REG_X(reg)		(((reg) >> X_CHANNEL_SHIFT) & REG_CHANNEL_MASK)
#define REG_Y(reg)		(((reg) >> Y_CHANNEL_SHIFT) & REG_CHANNEL_MASK)
#define REG_Z(reg)		(((reg) >> Z_CHANNEL_SHIFT) & REG_CHANNEL_MASK)
#define REG_W(reg)		(((reg) >> W_CHANNEL_SHIFT) & REG_CHANNEL_MASK)

enum i915_fs_channel {
	X_CHANNEL_VAL = 0,
	Y_CHANNEL_VAL,
	Z_CHANNEL_VAL,
	W_CHANNEL_VAL,
	ZERO_CHANNEL_VAL,
	ONE_CHANNEL_VAL,

	NEG_X_CHANNEL_VAL = X_CHANNEL_VAL | 0x8,
	NEG_Y_CHANNEL_VAL = Y_CHANNEL_VAL | 0x8,
	NEG_Z_CHANNEL_VAL = Z_CHANNEL_VAL | 0x8,
	NEG_W_CHANNEL_VAL = W_CHANNEL_VAL | 0x8,
	NEG_ONE_CHANNEL_VAL = ONE_CHANNEL_VAL | 0x8
};

#define i915_fs_operand(reg, x, y, z, w) \
	(reg) | \
(x##_CHANNEL_VAL << X_CHANNEL_SHIFT) | \
(y##_CHANNEL_VAL << Y_CHANNEL_SHIFT) | \
(z##_CHANNEL_VAL << Z_CHANNEL_SHIFT) | \
(w##_CHANNEL_VAL << W_CHANNEL_SHIFT)

/**
 * Construct an operand description for using a register with no swizzling
 */
#define i915_fs_operand_reg(reg)					\
	i915_fs_operand(reg, X, Y, Z, W)

#define i915_fs_operand_reg_negate(reg)					\
	i915_fs_operand(reg, NEG_X, NEG_Y, NEG_Z, NEG_W)

/**
 * Returns an operand containing (0.0, 0.0, 0.0, 0.0).
 */
#define i915_fs_operand_zero() i915_fs_operand(FS_R0, ZERO, ZERO, ZERO, ZERO)

/**
 * Returns an unused operand
 */
#define i915_fs_operand_none() i915_fs_operand_zero()

/**
 * Returns an operand containing (1.0, 1.0, 1.0, 1.0).
 */
#define i915_fs_operand_one() i915_fs_operand(FS_R0, ONE, ONE, ONE, ONE)

#define i915_get_hardware_channel_val(val, shift, negate) \
	(((val & 0x7) << shift) | ((val & 0x8) ? negate : 0))

/**
 * Outputs a fragment shader command to declare a sampler or texture register.
 */
#define i915_fs_dcl(reg)						\
	do {									\
		OUT_BATCH(D0_DCL | \
			  (REG_TYPE(reg) << D0_TYPE_SHIFT) | \
			  (REG_NR(reg) << D0_NR_SHIFT) | \
			  ((REG_TYPE(reg) != REG_TYPE_S) ? D0_CHANNEL_ALL : 0)); \
		OUT_BATCH(0); \
		OUT_BATCH(0); \
	} while (0)

#define i915_fs_texld(dest_reg, sampler_reg, address_reg)		\
	do {									\
		OUT_BATCH(T0_TEXLD | \
			  (REG_TYPE(dest_reg) << T0_DEST_TYPE_SHIFT) | \
			  (REG_NR(dest_reg) << T0_DEST_NR_SHIFT) | \
			  (REG_NR(sampler_reg) << T0_SAMPLER_NR_SHIFT)); \
		OUT_BATCH((REG_TYPE(address_reg) << T1_ADDRESS_REG_TYPE_SHIFT) | \
			  (REG_NR(address_reg) << T1_ADDRESS_REG_NR_SHIFT)); \
		OUT_BATCH(0); \
	} while (0)

#define i915_fs_texldp(dest_reg, sampler_reg, address_reg)		\
	do {									\
		OUT_BATCH(T0_TEXLDP | \
			  (REG_TYPE(dest_reg) << T0_DEST_TYPE_SHIFT) | \
			  (REG_NR(dest_reg) << T0_DEST_NR_SHIFT) | \
			  (REG_NR(sampler_reg) << T0_SAMPLER_NR_SHIFT)); \
		OUT_BATCH((REG_TYPE(address_reg) << T1_ADDRESS_REG_TYPE_SHIFT) | \
			  (REG_NR(address_reg) << T1_ADDRESS_REG_NR_SHIFT)); \
		OUT_BATCH(0); \
	} while (0)

#define i915_fs_arith_masked(op, dest_reg, dest_mask, operand0, operand1, operand2)	\
	_i915_fs_arith_masked(A0_##op, dest_reg, dest_mask, operand0, operand1, operand2)

#define i915_fs_arith(op, dest_reg, operand0, operand1, operand2)	\
	_i915_fs_arith(A0_##op, dest_reg, operand0, operand1, operand2)

#define _i915_fs_arith_masked(cmd, dest_reg, dest_mask, operand0, operand1, operand2) \
	do { \
		/* Set up destination register and write mask */ \
		OUT_BATCH(cmd | \
			  (REG_TYPE(dest_reg) << A0_DEST_TYPE_SHIFT) | \
			  (REG_NR(dest_reg) << A0_DEST_NR_SHIFT) | \
			  (((dest_mask) & ~MASK_SATURATE) << A0_DEST_CHANNEL_SHIFT) | \
			  (((dest_mask) & MASK_SATURATE) ? A0_DEST_SATURATE : 0) | \
			  /* Set up operand 0 */ \
			  (REG_TYPE(operand0) << A0_SRC0_TYPE_SHIFT) | \
			  (REG_NR(operand0) << A0_SRC0_NR_SHIFT)); \
		OUT_BATCH(i915_get_hardware_channel_val(REG_X(operand0), \
							A1_SRC0_CHANNEL_X_SHIFT, \
							A1_SRC0_CHANNEL_X_NEGATE) | \
			  i915_get_hardware_channel_val(REG_Y(operand0), \
							A1_SRC0_CHANNEL_Y_SHIFT, \
							A1_SRC0_CHANNEL_Y_NEGATE) | \
			  i915_get_hardware_channel_val(REG_Z(operand0), \
							A1_SRC0_CHANNEL_Z_SHIFT, \
							A1_SRC0_CHANNEL_Z_NEGATE) | \
			  i915_get_hardware_channel_val(REG_W(operand0), \
							A1_SRC0_CHANNEL_W_SHIFT, \
							A1_SRC0_CHANNEL_W_NEGATE) | \
			  /* Set up operand 1 */ \
			  (REG_TYPE(operand1) << A1_SRC1_TYPE_SHIFT) | \
			  (REG_NR(operand1) << A1_SRC1_NR_SHIFT) | \
			  i915_get_hardware_channel_val(REG_X(operand1), \
							A1_SRC1_CHANNEL_X_SHIFT, \
							A1_SRC1_CHANNEL_X_NEGATE) | \
			  i915_get_hardware_channel_val(REG_Y(operand1), \
							A1_SRC1_CHANNEL_Y_SHIFT, \
							A1_SRC1_CHANNEL_Y_NEGATE)); \
		OUT_BATCH(i915_get_hardware_channel_val(REG_Z(operand1), \
							A2_SRC1_CHANNEL_Z_SHIFT, \
							A2_SRC1_CHANNEL_Z_NEGATE) | \
			  i915_get_hardware_channel_val(REG_W(operand1), \
							A2_SRC1_CHANNEL_W_SHIFT, \
							A2_SRC1_CHANNEL_W_NEGATE) | \
			  /* Set up operand 2 */ \
			  (REG_TYPE(operand2) << A2_SRC2_TYPE_SHIFT) | \
			  (REG_NR(operand2) << A2_SRC2_NR_SHIFT) | \
			  i915_get_hardware_channel_val(REG_X(operand2), \
							A2_SRC2_CHANNEL_X_SHIFT, \
							A2_SRC2_CHANNEL_X_NEGATE) | \
			  i915_get_hardware_channel_val(REG_Y(operand2), \
							A2_SRC2_CHANNEL_Y_SHIFT, \
							A2_SRC2_CHANNEL_Y_NEGATE) | \
			  i915_get_hardware_channel_val(REG_Z(operand2), \
							A2_SRC2_CHANNEL_Z_SHIFT, \
							A2_SRC2_CHANNEL_Z_NEGATE) | \
			  i915_get_hardware_channel_val(REG_W(operand2), \
							A2_SRC2_CHANNEL_W_SHIFT, \
							A2_SRC2_CHANNEL_W_NEGATE)); \
	} while (0)

#define _i915_fs_arith(cmd, dest_reg, operand0, operand1, operand2) do {\
	/* Set up destination register and write mask */ \
	OUT_BATCH(cmd | \
		  (REG_TYPE(dest_reg) << A0_DEST_TYPE_SHIFT) | \
		  (REG_NR(dest_reg) << A0_DEST_NR_SHIFT) | \
		  (A0_DEST_CHANNEL_ALL) | \
		  /* Set up operand 0 */ \
		  (REG_TYPE(operand0) << A0_SRC0_TYPE_SHIFT) | \
		  (REG_NR(operand0) << A0_SRC0_NR_SHIFT)); \
	OUT_BATCH(i915_get_hardware_channel_val(REG_X(operand0), \
						A1_SRC0_CHANNEL_X_SHIFT, \
						A1_SRC0_CHANNEL_X_NEGATE) | \
		  i915_get_hardware_channel_val(REG_Y(operand0), \
						A1_SRC0_CHANNEL_Y_SHIFT, \
						A1_SRC0_CHANNEL_Y_NEGATE) | \
		  i915_get_hardware_channel_val(REG_Z(operand0), \
						A1_SRC0_CHANNEL_Z_SHIFT, \
						A1_SRC0_CHANNEL_Z_NEGATE) | \
		  i915_get_hardware_channel_val(REG_W(operand0), \
						A1_SRC0_CHANNEL_W_SHIFT, \
						A1_SRC0_CHANNEL_W_NEGATE) | \
		  /* Set up operand 1 */ \
		  (REG_TYPE(operand1) << A1_SRC1_TYPE_SHIFT) | \
		  (REG_NR(operand1) << A1_SRC1_NR_SHIFT) | \
		  i915_get_hardware_channel_val(REG_X(operand1), \
						A1_SRC1_CHANNEL_X_SHIFT, \
						A1_SRC1_CHANNEL_X_NEGATE) | \
		  i915_get_hardware_channel_val(REG_Y(operand1), \
						A1_SRC1_CHANNEL_Y_SHIFT, \
						A1_SRC1_CHANNEL_Y_NEGATE)); \
	OUT_BATCH(i915_get_hardware_channel_val(REG_Z(operand1), \
						A2_SRC1_CHANNEL_Z_SHIFT, \
						A2_SRC1_CHANNEL_Z_NEGATE) | \
		  i915_get_hardware_channel_val(REG_W(operand1), \
						A2_SRC1_CHANNEL_W_SHIFT, \
						A2_SRC1_CHANNEL_W_NEGATE) | \
		  /* Set up operand 2 */ \
		  (REG_TYPE(operand2) << A2_SRC2_TYPE_SHIFT) | \
		  (REG_NR(operand2) << A2_SRC2_NR_SHIFT) | \
		  i915_get_hardware_channel_val(REG_X(operand2), \
						A2_SRC2_CHANNEL_X_SHIFT, \
						A2_SRC2_CHANNEL_X_NEGATE) | \
		  i915_get_hardware_channel_val(REG_Y(operand2), \
						A2_SRC2_CHANNEL_Y_SHIFT, \
						A2_SRC2_CHANNEL_Y_NEGATE) | \
		  i915_get_hardware_channel_val(REG_Z(operand2), \
						A2_SRC2_CHANNEL_Z_SHIFT, \
						A2_SRC2_CHANNEL_Z_NEGATE) | \
		  i915_get_hardware_channel_val(REG_W(operand2), \
						A2_SRC2_CHANNEL_W_SHIFT, \
						A2_SRC2_CHANNEL_W_NEGATE)); \
} while (0)

#define i915_fs_mov(dest_reg, operand0)					\
	i915_fs_arith(MOV, dest_reg, \
		      operand0,			\
		      i915_fs_operand_none(),			\
		      i915_fs_operand_none())

#define i915_fs_mov_masked(dest_reg, dest_mask, operand0)		\
	i915_fs_arith_masked (MOV, dest_reg, dest_mask, \
			      operand0, \
			      i915_fs_operand_none(), \
			      i915_fs_operand_none())


#define i915_fs_frc(dest_reg, operand0)					\
	i915_fs_arith (FRC, dest_reg, \
		       operand0,			\
		       i915_fs_operand_none(),			\
		       i915_fs_operand_none())

/** Add operand0 and operand1 and put the result in dest_reg */
#define i915_fs_add(dest_reg, operand0, operand1)			\
	i915_fs_arith (ADD, dest_reg, \
		       operand0, operand1,	\
		       i915_fs_operand_none())

/** Multiply operand0 and operand1 and put the result in dest_reg */
#define i915_fs_mul(dest_reg, operand0, operand1)			\
	i915_fs_arith (MUL, dest_reg, \
		       operand0, operand1,	\
		       i915_fs_operand_none())

/** Computes 1/sqrt(operand0.replicate_swizzle) puts the result in dest_reg */
#define i915_fs_rsq(dest_reg, dest_mask, operand0)		\
	do {									\
		if (dest_mask) {							\
			i915_fs_arith_masked (RSQ, dest_reg, dest_mask, \
					      operand0,			\
					      i915_fs_operand_none (),			\
					      i915_fs_operand_none ());			\
		} else { \
			i915_fs_arith (RSQ, dest_reg, \
				       operand0, \
				       i915_fs_operand_none (), \
				       i915_fs_operand_none ()); \
		} \
	} while (0)

/** Puts the minimum of operand0 and operand1 in dest_reg */
#define i915_fs_min(dest_reg, operand0, operand1)			\
	i915_fs_arith (MIN, dest_reg, \
		       operand0, operand1, \
		       i915_fs_operand_none())

/** Puts the maximum of operand0 and operand1 in dest_reg */
#define i915_fs_max(dest_reg, operand0, operand1)			\
	i915_fs_arith (MAX, dest_reg, \
		       operand0, operand1, \
		       i915_fs_operand_none())

#define i915_fs_cmp(dest_reg, operand0, operand1, operand2)		\
	i915_fs_arith (CMP, dest_reg, operand0, operand1, operand2)

/** Perform operand0 * operand1 + operand2 and put the result in dest_reg */
#define i915_fs_mad(dest_reg, dest_mask, op0, op1, op2)	\
	do {									\
		if (dest_mask) {							\
			i915_fs_arith_masked (MAD, dest_reg, dest_mask, op0, op1, op2); \
		} else { \
			i915_fs_arith (MAD, dest_reg, op0, op1, op2); \
		} \
	} while (0)

#define i915_fs_dp2add(dest_reg, dest_mask, op0, op1, op2)	\
	do {									\
		if (dest_mask) {							\
			i915_fs_arith_masked (DP2ADD, dest_reg, dest_mask, op0, op1, op2); \
		} else { \
			i915_fs_arith (DP2ADD, dest_reg, op0, op1, op2); \
		} \
	} while (0)

/**
 * Perform a 3-component dot-product of operand0 and operand1 and put the
 * resulting scalar in the channels of dest_reg specified by the dest_mask.
 */
#define i915_fs_dp3(dest_reg, dest_mask, op0, op1)	\
	do {									\
		if (dest_mask) {							\
			i915_fs_arith_masked (DP3, dest_reg, dest_mask, \
					      op0, op1,\
					      i915_fs_operand_none());			\
		} else { \
			i915_fs_arith (DP3, dest_reg, op0, op1,\
				       i915_fs_operand_none());			\
		} \
	} while (0)

/**
 * Sets up local state for accumulating a fragment shader buffer.
 *
 * \param x maximum number of shader commands that may be used between
 *        a FS_START and FS_END
 */
#define FS_LOCALS()							\
	uint32_t _shader_offset

#define FS_BEGIN()							\
	do {									\
		_shader_offset = intel->batch_used++;				\
	} while (0)

#define FS_END()							\
	do {									\
		intel->batch_ptr[_shader_offset] =					\
		_3DSTATE_PIXEL_SHADER_PROGRAM |					\
		(intel->batch_used - _shader_offset - 2);			\
	} while (0);
