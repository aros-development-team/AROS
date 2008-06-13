#ifndef LIBRARIES_CGXVIDEO_H
#define LIBRARIES_CGXVIDEO_H


#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif


struct    VLayerHandle
{
	ULONG	VLH_Void;		/*dont know the structure _yet_ */	
};


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

#define VOA_BaseAddress		(VOA_TAGBASE+0x30)
#define VOA_ColorKeyPen		(VOA_TAGBASE+0x31)
#define VOA_ColorKey		(VOA_TAGBASE+0x32)

/* returned error values for VOA_Error tag */

#define VOERR_OK			0						/* No error */
#define VOERR_INVSCRMODE	1						/* video overlay not possible for selected mode */
#define VOERR_NOOVLMEMORY	2						/* No memory free for video overlay */
#define VOERR_INVSRCFMT		3						/* Source in unsupported format*/
#define VOERR_NOMEMORY		4						/* Not enough free memory */

/* Source data types --------------------- */

#define SRCFMT_YUV16		0
#define SRCFMT_YCbCr16		1
#define SRCFMT_RGB15PC		2						/* for historical reasons this format is byte swapped */
#define SRCFMT_RGB16PC		3						/* for historical reasons this format is byte swapped */

#endif /* LIBRARIES_CGXVIDEO_H */
