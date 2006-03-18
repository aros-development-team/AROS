#ifndef RADEON_H_
#define RADEON_H_

#include <exec/types.h>

typedef struct {
    ULONG freq;
    ULONG value;
} RADEONTMDSPll;

typedef struct {
    UWORD   reference_freq;
    UWORD   reference_div;
    ULONG   min_pll_freq;
    ULONG   max_pll_freq;
    UWORD   xclk;
} RADEONPLLRec;

typedef enum
{
    DDC_NONE_DETECTED, DDC_NONE = DDC_NONE_DETECTED,
    DDC_MONID,
    DDC_DVI,
    DDC_VGA,
    DDC_CRT2
} RADEONDDCType;

typedef enum {
    MT_UNKNOWN = -1,
    MT_NONE    = 0,
    MT_CRT     = 1,
    MT_LCD     = 2,
    MT_DFP     = 3,
    MT_CTV     = 4,
    MT_STV     = 5
} RADEONMonitorType;

typedef enum
{
    CONNECTOR_NONE,
    CONNECTOR_PROPRIETARY,
    CONNECTOR_CRT,
    CONNECTOR_DVI_I,
    CONNECTOR_DVI_D,
    CONNECTOR_CTV,
    CONNECTOR_STV,
    CONNECTOR_UNSUPPORTED
} RADEONConnectorType;

typedef enum
{
    CONNECTOR_NONE_ATOM,
    CONNECTOR_VGA_ATOM,
    CONNECTOR_DVI_I_ATOM,
    CONNECTOR_DVI_D_ATOM,
    CONNECTOR_DVI_A_ATOM,
    CONNECTOR_STV_ATOM,
    CONNECTOR_CTV_ATOM,
    CONNECTOR_LVDS_ATOM,
    CONNECTOR_DIGITAL_ATOM,
    CONNECTOR_UNSUPPORTED_ATOM
} RADEONConnectorTypeATOM;

typedef enum
{
    DAC_UNKNOWN = -1,
    DAC_PRIMARY = 0,
    DAC_TVDAC   = 1
} RADEONDacType;

typedef enum
{
    TMDS_UNKNOWN = -1,
    TMDS_INT     = 0,
    TMDS_EXT     = 1
} RADEONTmdsType;

typedef struct
{
    RADEONDDCType DDCType;
    RADEONDacType DACType;
    RADEONTmdsType TMDSType;
    RADEONConnectorType ConnectorType;
    RADEONMonitorType MonType;
} RADEONConnector;

typedef struct {
    ULONG width;
    ULONG height;
    UBYTE bpp;
    ULONG pixelc;
    IPTR base;
    ULONG HDisplay;
    ULONG VDisplay;
    ULONG HSyncStart;
    ULONG HSyncEnd;
    ULONG HTotal;
    ULONG VSyncStart;
    ULONG VSyncEnd;
    ULONG VTotal;
    ULONG Flags;
} RADEONModeInfo;

typedef enum {
    Unknown = 0,
    RAGE,
    RADEON,
    RV100,
    RS100,
    RV200,
    RS200, RS250=RS200,
    R200,
    RV250,
    RS300, RS350=RS300,
    RV280,
    R300,
    R350,  R360=R350,
    RV350, RV360=RV350,
    RV380, RV370=RV380,
    R420,  R423=R420,

    CHIP_FAMILY_LAST
} CardType;

#define IS_RV100_VARIANT ((sd->Card.Type == RV100) ||    \
    (sd->Card.Type == RV200)    || \
    (sd->Card.Type == RS100)    || \
    (sd->Card.Type == RS200)    || \
    (sd->Card.Type == RV250)    || \
    (sd->Card.Type == RV280)    || \
    (sd->Card.Type == RS300))

#define IS_R300_VARIANT ((sd->Card.Type == R300) ||    \
    (sd->Card.Type == RV350)    || \
    (sd->Card.Type == R350)    || \
    (sd->Card.Type == RV380)    || \
    (sd->Card.Type == R420))

struct CardState {
    ULONG             pixelc, HDisplay, bpp;
    
    /* Common registers */
    ULONG             ovr_clr;
    ULONG             ovr_wid_left_right;
    ULONG             ovr_wid_top_bottom;
    ULONG             ov0_scale_cntl;
    ULONG             mpp_tb_config;
    ULONG             mpp_gp_config;
    ULONG             subpic_cntl;
    ULONG             viph_control;
    ULONG             i2c_cntl_1;
    ULONG             gen_int_cntl;
    ULONG             cap0_trig_cntl;
    ULONG             cap1_trig_cntl;
    ULONG             bus_cntl;
    ULONG             surface_cntl;
    ULONG             bios_4_scratch;
    ULONG             bios_5_scratch;
    ULONG             bios_6_scratch;

    /* Other registers to save for VT switches */
    ULONG             dp_datatype;
    ULONG             rbbm_soft_reset;
    ULONG             clock_cntl_index;
    ULONG             amcgpio_en_reg;
    ULONG             amcgpio_mask;

    /* CRTC registers */
    ULONG             crtc_gen_cntl;
    ULONG             crtc_ext_cntl;
    ULONG             dac_cntl;
    ULONG             crtc_h_total_disp;
    ULONG             crtc_h_sync_strt_wid;
    ULONG             crtc_v_total_disp;
    ULONG             crtc_v_sync_strt_wid;
    ULONG             crtc_offset;
    ULONG             crtc_offset_cntl;
    ULONG             crtc_pitch;
    ULONG             disp_merge_cntl;
    ULONG             grph_buffer_cntl;
    ULONG             crtc_more_cntl;

    /* CRTC2 registers */
    ULONG             crtc2_gen_cntl;

    ULONG             dac2_cntl;
    ULONG             disp_output_cntl;
    ULONG             disp_hw_debug;
    ULONG             disp2_merge_cntl;
    ULONG             grph2_buffer_cntl;
    ULONG             crtc2_h_total_disp;
    ULONG             crtc2_h_sync_strt_wid;
    ULONG             crtc2_v_total_disp;
    ULONG             crtc2_v_sync_strt_wid;
    ULONG             crtc2_offset;
    ULONG             crtc2_offset_cntl;
    ULONG             crtc2_pitch;
    /* Flat panel registers */
    ULONG             fp_crtc_h_total_disp;
    ULONG             fp_crtc_v_total_disp;
    ULONG             fp_gen_cntl;
    ULONG             fp2_gen_cntl;
    ULONG             fp_h_sync_strt_wid;
    ULONG             fp2_h_sync_strt_wid;
    ULONG             fp_horz_stretch;
    ULONG             fp_panel_cntl;
    ULONG             fp_v_sync_strt_wid;
    ULONG             fp2_v_sync_strt_wid;
    ULONG             fp_vert_stretch;
    ULONG             lvds_gen_cntl;
    ULONG             lvds_pll_cntl;
    ULONG             tmds_pll_cntl;
    ULONG             tmds_transmitter_cntl;

                /* Computed values for PLL */
    ULONG             dot_clock_freq;
    ULONG             pll_output_freq;
    int               feedback_div;
    int               post_div;

                /* PLL registers */
    unsigned          ppll_ref_div;
    unsigned          ppll_div_3;
    ULONG             htotal_cntl;

                /* Computed values for PLL2 */
    ULONG             dot_clock_freq_2;
    ULONG             pll_output_freq_2;
    int               feedback_div_2;
    int               post_div_2;

                /* PLL2 registers */
    ULONG             p2pll_ref_div;
    ULONG             p2pll_div_0;
    ULONG             htotal_cntl2;

                /* Pallet */
    BOOL              palette_valid;
    ULONG             palette[256];
    ULONG             palette2[256];

    ULONG             tv_dac_cntl;
};

struct Card {
    UWORD           ProductID;
    UWORD           VendorID;
    CardType        Type;
    
    ULONG           *MMIO;
    UBYTE           *VBIOS, *vbios_org;
    IPTR            FbAddress;
    ULONG           FbUsableSize;
    IPTR            FrameBuffer;
    UWORD           ROMHeaderStart;
    UWORD           MasterDataStart;

    IPTR            CursorStart;

    BOOL            IsIGP;
    BOOL            cursorVisible;
    BOOL            IsAtomBios;
    BOOL            IsMobility;
#warning TODO: Remove IsSecondary! Clone screens instead
    BOOL            IsSecondary;
    BOOL            HasSecondary;
    BOOL            R300CGWorkaround;
    BOOL            IsDellServer;
    BOOL            HasCRTC2;
    BOOL            HasSingleDAC;
    BOOL            IsDDR;
    BOOL            DDC1;
    BOOL            DDC2;
    BOOL            DDCBios;
    BOOL            OverlayOnCRTC2;
    BOOL            Busy;

    BOOL ReversedDAC;     /* TVDAC used as primary dac */
    BOOL ReversedTMDS;    /* DDC_DVI is used for external TMDS */
    RADEONMonitorType   MonType2;
    RADEONMonitorType   MonType1;

    RADEONConnector PortInfo[2];
    RADEONTMDSPll   tmds_pll[4];
    RADEONPLLRec    pll;

    float           sclk,mclk;

    ULONG           PanelXRes, PanelYRes;
    ULONG           HBlank, HOverPlus, HSyncWidth;
    ULONG           VBlank, VOverPlus, VSyncWidth;
    ULONG           DotClock;
    ULONG           Flags;

    ULONG           DDCReg;
    ULONG           PanelPwrDly;
    ULONG           RamWidth;
    ULONG           BusCntl;
    ULONG           MemCntl;

};

#define V_DBLSCAN       0x00000001
#define V_CSYNC         0x00000002
#define V_NHSYNC        0x00000004
#define V_NVSYNC        0x00000008
#define V_INTERLACE     0x10000000
#define RADEON_USE_RMX  0x80000000

#define RADEON_IDLE_ENTRY   16
#define RADEON_TIMEOUT      2000000
#define RADEON_MMIOSIZE     0x80000

struct ati_staticdata;
void SaveState(struct ati_staticdata *sd, struct CardState *save);
void LoadState(struct ati_staticdata *sd, struct CardState *restore);
void DPMS(struct ati_staticdata *sd, HIDDT_DPMSLevel level);
void ShowHideCursor(struct ati_staticdata *sd, BOOL visible);
void InitMode(struct ati_staticdata *sd, struct CardState *save,
                ULONG width, ULONG height, UBYTE bpp, ULONG pixelc, IPTR base,
                ULONG HDisplay, ULONG VDisplay, 
                ULONG HSyncStart, ULONG HSyncEnd, ULONG HTotal,
                ULONG VSyncStart, ULONG VSyncEnd, ULONG VTotal);
BOOL RADEONInit(struct ati_staticdata *sd);

IPTR AllocBitmapArea(struct ati_staticdata *sd, ULONG width, ULONG height,
    ULONG bpp, BOOL must_have);
VOID FreeBitmapArea(struct ati_staticdata *sd, IPTR bmp, ULONG width, ULONG height, ULONG bpp);



#endif /*RADEON_H_*/
