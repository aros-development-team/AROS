#ifndef CYBERGRAPHX_CYBERGRAPHICS_H
#define CYBERGRAPHX_CYBERGRAPHICS_H

#ifndef GRAPHICS_DISPLAYINFO_H
#   include <graphics/displayinfo.h>
#endif

#ifndef UTILITY_TAGITEM_H
#   include <utility/tagitem.h>
#endif


#define CYBERGFXNAME	    	    "cybergraphics.library"
#define CYBERGFX_INCLUDE_VERSION    41UL


struct CyberModeNode
{
    struct Node     Node;
    char	    ModeText[DISPLAYNAMELEN];
    ULONG	    DisplayID;
    UWORD	    Width;
    UWORD	    Height;
    UWORD	    Depth;
    struct TagItem *DisplayTagList;	
};


#define CYBRMATTR_XMOD		0x80000001
#define CYBRMATTR_BPPIX		0x80000002
#define CYBRMATTR_DISPADR	0x80000003
#define CYBRMATTR_PIXFMT	0x80000004
#define CYBRMATTR_WIDTH		0x80000005
#define CYBRMATTR_HEIGHT	0x80000006
#define CYBRMATTR_DEPTH		0x80000007
#define CYBRMATTR_ISCYBERGFX	0x80000008
#define CYBRMATTR_ISLINEARMEM	0x80000009

#define CYBRIDATTR_PIXFMT	0x80000001
#define CYBRIDATTR_WIDTH	0x80000002
#define CYBRIDATTR_HEIGHT	0x80000003
#define CYBRIDATTR_DEPTH	0x80000004
#define CYBRIDATTR_BPPIX	0x80000005

#define CYBRMREQ_TB	    	(TAG_USER + 0x40000)

#define CYBRMREQ_MinDepth	(CYBRMREQ_TB + 0)
#define CYBRMREQ_MaxDepth	(CYBRMREQ_TB + 1)
#define CYBRMREQ_MinWidth	(CYBRMREQ_TB + 2)
#define CYBRMREQ_MaxWidth	(CYBRMREQ_TB + 3)
#define CYBRMREQ_MinHeight	(CYBRMREQ_TB + 4)
#define CYBRMREQ_MaxHeight	(CYBRMREQ_TB + 5)
#define CYBRMREQ_CModelArray	(CYBRMREQ_TB + 6)
#define CYBRMREQ_WinTitle	(CYBRMREQ_TB + 20)
#define CYBRMREQ_OKText		(CYBRMREQ_TB + 21)
#define CYBRMREQ_CancelText	(CYBRMREQ_TB + 22)
#define CYBRMREQ_Screen		(CYBRMREQ_TB + 30)

#define CYBRBIDTG_TB	    	(TAG_USER + 0x50000)

#define CYBRBIDTG_Depth		(CYBRBIDTG_TB + 0)
#define CYBRBIDTG_NominalWidth	(CYBRBIDTG_TB + 1)
#define CYBRBIDTG_NominalHeight	(CYBRBIDTG_TB + 2)
#define CYBRBIDTG_MonitorID	(CYBRBIDTG_TB + 3)
#define CYBRBIDTG_BoardName	(CYBRBIDTG_TB + 5)

#define PIXFMT_LUT8	0UL
#define PIXFMT_RGB15	1UL
#define PIXFMT_BGR15	2UL
#define PIXFMT_RGB15PC	3UL
#define PIXFMT_BGR15PC	4UL
#define PIXFMT_RGB16	5UL
#define PIXFMT_BGR16	6UL
#define PIXFMT_RGB16PC	7UL
#define PIXFMT_BGR16PC	8UL
#define PIXFMT_RGB24	9UL
#define PIXFMT_BGR24	10UL
#define PIXFMT_ARGB32	11UL
#define PIXFMT_BGRA32	12UL
#define PIXFMT_RGBA32	13UL

#define RECTFMT_RGB	0UL
#define RECTFMT_RGBA	1UL
#define RECTFMT_ARGB	2UL
#define RECTFMT_LUT8	3UL
#define RECTFMT_GREY8	4UL
#define RECTFMT_RAW 	5UL

/* AROS extensions */
#define PIXFMT_ABGR32	100UL

#define RECTFMT_RGB15	100UL
#define RECTFMT_BGR15	101UL
#define RECTFMT_RGB15PC 102UL
#define RECTFMT_BGR15PC 103UL
#define RECTFMT_RGB16	104UL
#define RECTFMT_BGR16	105UL
#define RECTFMT_RGB16PC 106UL
#define RECTFMT_BGR16PC 107UL
#define RECTFMT_RGB24	RECTFMT_RGB
#define RECTFMT_BGR24	109UL
#define RECTFMT_ARGB32	RECTFMT_ARGB
#define RECTFMT_BGRA32	111UL
#define RECTFMT_RGBA32	RECTFMT_RGBA
#define RECTFMT_ABGR32	113UL

#define SETVC_DPMSLevel		0x88002001

#define DPMS_ON		0UL
#define DPMS_STANDBY	1UL
#define DPMS_SUSPEND	2UL
#define DPMS_OFF	3UL


#define LBMI_WIDTH		0x84001001
#define LBMI_HEIGHT		0x84001002
#define LBMI_DEPTH		0x84001003
#define LBMI_PIXFMT		0x84001004
#define LBMI_BYTESPERPIX	0x84001005
#define LBMI_BYTESPERROW	0x84001006
#define LBMI_BASEADDRESS	0x84001007


#define UBMI_UPDATERECTS	0x85001001
#define UBMI_REALLYUNLOCK	0x85001002

struct CDrawMsg
{
    APTR    cdm_MemPtr;
    ULONG   cdm_offx;
    ULONG   cdm_offy;
    ULONG   cdm_xsize;
    ULONG   cdm_ysize;
    UWORD   cdm_BytesPerRow;
    UWORD   cdm_BytePerPix;
    UWORD   cdm_ColorModel;
};

#define CTABFMT_XRGB8	0UL


/*** The extend cybergfx flags are now defined together with the
   old AmigaOS flags, so we have to include them here
*/

#ifndef GRAPHICS_GFX_H
#   include <graphics/gfx.h>
#endif

#define SHIFT_PIXFMT( fmt ) (  ((ULONG)(fmt)) << BMB_PIXFMT_SHIFTUP )
#define DOWNSHIFT_PIXFMT( fmt ) (  ((ULONG)(fmt)) >> BMB_PIXFMT_SHIFTUP )


#endif /* CYBERGRAPHX_CYBERGRAPHICS_H */
