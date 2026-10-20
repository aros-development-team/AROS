#ifndef LIBRARIES_CGXVIDEO_H
#define LIBRARIES_CGXVIDEO_H


#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif


struct VLayerHandle;


#define VOA_TAGBASE			(0x88000000)

#define VOA_LeftIndent		(VOA_TAGBASE+0x01)
#define VOA_RightIndent		(VOA_TAGBASE+0x02)
#define VOA_TopIndent		(VOA_TAGBASE+0x03)
#define VOA_BottomIndent	(VOA_TAGBASE+0x04)

#define VOA_SrcType			(VOA_TAGBASE+0x05)
#define VOA_SrcWidth		(VOA_TAGBASE+0x06)
#define VOA_SrcHeight		(VOA_TAGBASE+0x07)

#define VOA_Error			(VOA_TAGBASE+0x08)

#define VOA_UseColorKey		(VOA_TAGBASE+0x09)

#define VOA_UseBackfill		(VOA_TAGBASE+0x0a)

#define VOA_UseFilter		(VOA_TAGBASE+0x0c)

#define VOA_BaseAddress		(VOA_TAGBASE+0x30)
#define VOA_ColorKeyPen		(VOA_TAGBASE+0x31)
#define VOA_ColorKey		(VOA_TAGBASE+0x32)

/* V42 */
#define VOA_Identifier		(VOA_TAGBASE+0x0b)

#define VOA_FrameBase0		(VOA_TAGBASE+0x33)
#define VOA_FrameBase1		(VOA_TAGBASE+0x34)
#define VOA_FrameType		(VOA_TAGBASE+0x35)

#define VOA_Width			(VOA_TAGBASE+0x36)
#define VOA_Height			(VOA_TAGBASE+0x37)
#define VOA_Modulo			(VOA_TAGBASE+0x38)	/* also a creation tag since V43 */

/* V43 */
#define VOA_DoubleBuffer	(VOA_TAGBASE+0x0d)
#define VOA_InterLaced		(VOA_TAGBASE+0x0e)
#define VOA_CaptureMode		(VOA_TAGBASE+0x0f)

#define VOA_BaseOffset		(VOA_TAGBASE+0x39)

/* V50 */
#define VOA_FrameIndex		(VOA_TAGBASE+0x10)
#define VOA_MultiBuffer		(VOA_TAGBASE+0x11)
#define VOA_ZoomRect		(VOA_TAGBASE+0x12)

#define VOA_BaseOffset0		(VOA_TAGBASE+0x40)
#define VOA_BaseOffset1		(VOA_TAGBASE+0x41)
#define VOA_BaseOffset2		(VOA_TAGBASE+0x42)
#define VOA_BaseOffset3		(VOA_TAGBASE+0x43)
#define VOA_BaseOffset4		(VOA_TAGBASE+0x44)
#define VOA_BaseOffset5		(VOA_TAGBASE+0x45)

/* V50 subpicture */
#define VOA_Color0SP		(VOA_TAGBASE+0x50)
#define VOA_Color1SP		(VOA_TAGBASE+0x51)
#define VOA_Color2SP		(VOA_TAGBASE+0x52)
#define VOA_Color3SP		(VOA_TAGBASE+0x53)
#define VOA_Color4SP		(VOA_TAGBASE+0x54)
#define VOA_Color5SP		(VOA_TAGBASE+0x55)
#define VOA_Color6SP		(VOA_TAGBASE+0x56)
#define VOA_Color7SP		(VOA_TAGBASE+0x57)
#define VOA_Color8SP		(VOA_TAGBASE+0x58)
#define VOA_Color9SP		(VOA_TAGBASE+0x59)
#define VOA_Color10SP		(VOA_TAGBASE+0x5a)
#define VOA_Color11SP		(VOA_TAGBASE+0x5b)
#define VOA_Color12SP		(VOA_TAGBASE+0x5c)
#define VOA_Color13SP		(VOA_TAGBASE+0x5d)
#define VOA_Color14SP		(VOA_TAGBASE+0x5e)
#define VOA_Color15SP		(VOA_TAGBASE+0x5f)

#define VOA_SubPicture		(VOA_TAGBASE+0x60)
#define VOA_EnableSP		(VOA_TAGBASE+0x61)
#define VOA_StreamRectSP	(VOA_TAGBASE+0x62)
#define VOA_ColConSP		(VOA_TAGBASE+0x63)
#define VOA_HLRectSP		(VOA_TAGBASE+0x64)
#define VOA_HLEnableSP		(VOA_TAGBASE+0x65)
#define VOA_HLColConSP		(VOA_TAGBASE+0x66)
#define VOA_SrcWidthSP		(VOA_TAGBASE+0x67)
#define VOA_SrcHeightSP		(VOA_TAGBASE+0x68)

/* AttachVLayerTagList() */
#define VOA_ColorKeyFill	(VOA_TAGBASE+0x70)

/* QueryVLayerAttr() attributes */

#define VSQ_Dummy				(TAG_USER+0xa5000)
#define VSQ_SupportedFeatures	(VSQ_Dummy+1)
#define VSQ_SupportedFormats	(VSQ_Dummy+2)
#define VSQ_MaxWidth			(VSQ_Dummy+3)
#define VSQ_MaxWidthSP			(VSQ_Dummy+4)

/* VSQ_SupportedFeatures bits */
#define VSQ_FEAT_OVERLAY		(1UL << 0)
#define VSQ_FEAT_DOUBLEBUFFER	(1UL << 1)
#define VSQ_FEAT_MULTIBUFFER	(1UL << 2)
#define VSQ_FEAT_COLORKEYING	(1UL << 3)
#define VSQ_FEAT_FILTERING		(1UL << 4)
#define VSQ_FEAT_CAPTUREMODE	(1UL << 5)
#define VSQ_FEAT_INTERLACE		(1UL << 6)
#define VSQ_FEAT_ZOOMRECT		(1UL << 7)
#define VSQ_FEAT_SUBPICTURE		(1UL << 8)

/* VSQ_SupportedFormats bits */
#define VSQ_FMT_YUYV			(1UL << 0)
#define VSQ_FMT_R5G5B5_LE		(1UL << 1)
#define VSQ_FMT_R5G6B5_LE		(1UL << 2)
#define VSQ_FMT_YUV420_PLANAR	(1UL << 3)

/* returned error values for VOA_Error tag */

#define VOERR_OK			0						/* No error */
#define VOERR_INVSCRMODE	1						/* video overlay not possible for selected mode */
#define VOERR_NOOVLMEMORY	2						/* No memory free for video overlay */
#define VOERR_INVSRCFMT		3						/* Source in unsupported format*/
#define VOERR_NOMEMORY		4						/* Not enough free memory */

/* Source data types --------------------- */

#define SRCFMT_YUV16		0						/* obsolete, use SRCFMT_YCbCr16 */
#define SRCFMT_YCbCr16		1
#define SRCFMT_RGB15PC		2						/* for historical reasons this format is byte swapped */
#define SRCFMT_RGB16PC		3						/* for historical reasons this format is byte swapped */
#define SRCFMT_YCbCr420		4						/* planar */

#define SRCFMT_RGB15		SRCFMT_RGB15PC
#define SRCFMT_R5G5B5PC		SRCFMT_RGB15PC
#define SRCFMT_RGB16		SRCFMT_RGB16PC
#define SRCFMT_R5G6B5PC		SRCFMT_RGB16PC

#endif /* LIBRARIES_CGXVIDEO_H */
