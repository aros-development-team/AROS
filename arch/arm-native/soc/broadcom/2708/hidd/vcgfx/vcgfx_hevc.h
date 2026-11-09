/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    The HEVC decoder block of the BCM2711 behind gfx.hidd's decode
    interface. Written from a clean-room description of the hardware.

    vcgfx_hevc_cl.c turns the VA-API structures of a picture into the
    block's command list and phase 2 register values; vcgfx_hevc_hw.c runs
    the two phases; vcgfx_decode.c is the decode interface and the
    decode-target bitmaps.
*/

#ifndef VCGFX_HEVC_H
#define VCGFX_HEVC_H

#include <exec/types.h>
#include "vcgfx_vadec.h"

/* Register blocks, the 64 KB decoder and the 4 KB interrupt control. The
 * Pi 5's IOMMU2 is off at hand-over: the block takes physical addresses. */
#define HEVC_BCM2711_REGS       0xFEB00000
#define HEVC_BCM2711_INTC       0xFEB10000
#define HEVC_BCM2712_REGS       0x1000800000ULL
#define HEVC_BCM2712_INTC       0x1000840000ULL
#define HEVC_IRQ                (32 + 98)       /* GIC SPI 98 on both */
#define HEVC_FW_CLOCK_ID        11

/* Phase 1 */
#define HEVC_SPS0               0x0000
#define HEVC_SPS1               0x0004
#define HEVC_PPS                0x0008
#define HEVC_SLICE              0x000C
#define HEVC_TILESTART          0x0010
#define HEVC_TILEEND            0x0014
#define HEVC_SLICESTART         0x0018
#define HEVC_MODE               0x001C
#define HEVC_QP                 0x0030
#define HEVC_CONTROL            0x0034
#define HEVC_STATUS             0x0038
#define HEVC_VERSION            0x003C
#define HEVC_BFBASE             0x0040
#define HEVC_BFNUM              0x0044
#define HEVC_BFCONTROL          0x0048
#define HEVC_PUWBASE            0x0050
#define HEVC_PUWSTRIDE          0x0054
#define HEVC_COEFFWBASE         0x0058
#define HEVC_COEFFWSTRIDE       0x005C
#define HEVC_SLICECMDS          0x0060
#define HEVC_BEGINTILEEND       0x0064
#define HEVC_TRANSFER           0x0068
#define HEVC_CFBASE             0x006C
#define HEVC_CFNUM              0x0070
#define HEVC_CFSTATUS           0x0074
#define HEVC_CABAC_TABLE        0x1000
#define HEVC_SCALING_TABLE      0x2000
#define HEVC_SLICE_MSGS         0x4000

#define HEVC_STATUS_COEFF_FULL  (1 << 3)
#define HEVC_STATUS_PU_FULL     (1 << 4)

#define HEVC_TRANSFER_BACKUP    0x14500
#define HEVC_TRANSFER_RELOAD    0x14014

/* Phase 2 */
#define HEVC_PURBASE            0x8000
#define HEVC_PURSTRIDE          0x8004
#define HEVC_COEFFRBASE         0x8008
#define HEVC_COEFFRSTRIDE       0x800C
#define HEVC_NUMROWS            0x8010
#define HEVC_CONFIG2            0x8014
#define HEVC_OUTYBASE           0x8018
#define HEVC_OUTYSTRIDE         0x801C
#define HEVC_OUTCBASE           0x8020
#define HEVC_OUTCSTRIDE         0x8024
#define HEVC_FRAMESIZE          0x802C
#define HEVC_MVBASE             0x8030
#define HEVC_MVSTRIDE           0x8034
#define HEVC_COLBASE            0x8038
#define HEVC_COLSTRIDE          0x803C
#define HEVC_CURRPOC            0x8040
#define HEVC_REFS               0x9000  /* 16 x {luma, pitch, chroma, pitch} */

/* Interrupt control */
#define HEVC_ICTRL              0x0000
#define HEVC_ICTRL_ACTIVE1_INT  (1 << 0)
#define HEVC_ICTRL_ACTIVE1_EN   (1 << 2)
#define HEVC_ICTRL_ACTIVE1      (1 << 3)    /* phase 1 running (line level) */
#define HEVC_ICTRL_ACTIVE2_INT  (1 << 4)
#define HEVC_ICTRL_ACTIVE2_EN   (1 << 6)
#define HEVC_ICTRL_ACTIVE2      (1 << 7)    /* phase 2 running */
#define HEVC_ICTRL_RESERVED     0x000FF800  /* bits 11-19, written as 0 */
#define HEVC_ICTRL_PENDING      ((1 << 0) | (1 << 4) | (1 << 25) | (1 << 29))

#define HEVC_REF_SLOTS          16
#define HEVC_CABAC_STATES       154
#define HEVC_SCALING_WORDS      1016
#define HEVC_MAX_MSGS           259

/* A buffer as the block sees it: 64-byte aligned bus address */
#define HEVC_ADDR(a)            ((ULONG)((a) >> 6))
#define HEVC_LEN(l)             ((ULONG)(((l) + 63) >> 6))

/* SAND128 column format (8-bit): luma rows then chroma rows per column */
#define HEVC_SAND_WIDTH(w)      (((w) + 127) & ~127)
#define HEVC_SAND_HEIGHT(h)     (((h) + 7) & ~7)
#define HEVC_SAND_ROWS(h)       (HEVC_SAND_HEIGHT(h) * 3 / 2)
#define HEVC_SAND_PITCH(h)      (HEVC_SAND_ROWS(h) * 128)
#define HEVC_SAND_SIZE(w, h)    (HEVC_SAND_ROWS(h) * HEVC_SAND_WIDTH(w))

/* 10-bit: three samples per 32-bit word, 96 per column row; same rows */
#define HEVC_SAND10_SAMPLES(w)  (((((w) + 2) / 3 + 31) & ~31) * 3)
#define HEVC_SAND10_BYTES(w)    (HEVC_SAND10_SAMPLES(w) * 4 / 3)
#define HEVC_SAND10_SIZE(w, h)  (HEVC_SAND_ROWS(h) * HEVC_SAND10_BYTES(w))

/* Where a picture of the stream lives, as bus addresses */
struct hevc_surface {
    UQUAD luma;             /* 0 = unknown */
    UQUAD chroma;
    UQUAD colmv;            /* this picture's motion vectors, 0 = none */
};

struct hevc_cmd {
    ULONG addr;
    ULONG value;
};

/* The slices of one picture: VA parameters and where their data lies */
struct hevc_slice {
    VASliceParameterBufferHEVC sp;
    UQUAD data;             /* bus address of the slice data buffer */
    ULONG datasize;         /* its bytes */
};

/* Everything phase 2 needs, worked out with the command list */
struct hevc_phase2 {
    ULONG config2;
    ULONG framesize;
    ULONG currpoc;
    ULONG numrows;
    UQUAD outy, outc;
    ULONG outstride;        /* column pitch, bytes */
    UQUAD mvbase, colbase;
    ULONG mvstride;
    UQUAD refy[HEVC_REF_SLOTS], refc[HEVC_REF_SLOTS];
};

/* The picture to translate */
struct hevc_picture {
    const VAPictureParameterBufferHEVC *pp;
    const VAIQMatrixBufferHEVC *iq;     /* NULL: none submitted */
    const struct hevc_slice *slices;
    ULONG nslices;
    struct hevc_surface out;
    struct hevc_surface refs[15];       /* ReferenceFrames[i] resolved */
    ULONG pitch;                        /* column pitch of every picture, bytes */
};

/* A growing command list */
struct hevc_cl {
    struct hevc_cmd *cmds;
    ULONG n, max;
    BOOL overflow;
};

/* Picture size limits of the block */
#define HEVC_MIN_SIZE           32
#define HEVC_MAX_SIZE           4096

/* Sizes of the stream's working buffers */
ULONG hevc_colmv_size(ULONG width, ULONG height);
ULONG hevc_colmv_stride(ULONG width);

/*
 * Build the command list and the phase 2 values of a picture. Returns NULL
 * on success, or why the picture can't be decoded. map: CTB tile-scan
 * tables, (W * H) ULONGs twice, from hevc_tiles_size().
 */
/* CtbLog2SizeY 4..6: checked before the sizes below shift by it */
BOOL hevc_ctb_ok(const VAPictureParameterBufferHEVC *pp);
ULONG hevc_tiles_size(const VAPictureParameterBufferHEVC *pp);
const char *hevc_build(const struct hevc_picture *pic, struct hevc_cl *cl,
                       struct hevc_phase2 *p2, ULONG *map);

/* What the command list needs: enough for any picture of this size */
ULONG hevc_cl_estimate(const VAPictureParameterBufferHEVC *pp, ULONG nslices);

#endif /* VCGFX_HEVC_H */
