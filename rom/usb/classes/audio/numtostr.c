
#include "numtostr.h"

#include <proto/poseidon.h>

#define ps nch->nch_Base

/* /// "Terminal Types" */
const struct AudioIDMap audioterminaltypes[] =
{
    { UAUTT_UNDEFINED       , "USB Undefined" },
    { UAUTT_STREAMING       , "USB Stream" },
    { UAUTT_VENDOR          , "USB vendor specific" },
    { UAITT_UNDEFINED       , "Input Undefined" },
    { UAITT_MIC             , "Microphone" },
    { UAITT_DESKTOP_MIC     , "Desktop microphone" },
    { UAITT_PERSONAL_MIC    , "Personal microphone" },
    { UAITT_OMNI_DIR_MIC    , "Omni-directional microphone" },
    { UAITT_MIC_ARRAY       , "Microphone array" },
    { UAITT_PROC_MIC_ARRAY  , "Processing microphone array" },
    { UAOTT_UNDEFINED       , "Output Undefined" },
    { UAOTT_SPEAKER         , "Speaker" },
    { UAOTT_HEADPHONES      , "Headphones" },
    { UAOTT_DISPLAY         , "Head Mounted Display Audio" },
    { UAOTT_DESKTOP_SPEAKER , "Desktop speaker" },
    { UAOTT_ROOM_SPEAKER    , "Room speaker" },
    { UAOTT_COMM_SPEAKER    , "Communication speaker" },
    { UAOTT_LOFI_SPEAKER    , "Low frequency effects speaker" },
    { UABTT_UNDEFINED       , "Bi-directional Undefined" },
    { UABTT_HANDSET         , "Handset" },
    { UABTT_HEADSET         , "Headset" },
    { UABTT_SPEAKERPHONE_NER, "Speakerphone, no echo reduction" },
    { UABTT_SPEAKERPHONE_ES , "Echo-suppressing speakerphone" },
    { UABTT_SPEAKERPHONE_EC , "Echo-canceling speakerphone" },
    { UATTT_UNDEFINED       , "Telephony Undefined" },
    { UATTT_PHONE_LINE      , "Phone line" },
    { UATTT_TELEPHONE       , "Telephone" },
    { UATTT_DOWN_LINE_PHONE , "Down Line Phone" },
    { UAETT_UNDEFINED       , "External Undefined" },
    { UAETT_ANALOG          , "Analog connector" },
    { UAETT_DIGITAL         , "Digital audio interface" },
    { UAETT_LINE            , "Line connector" },
    { UAETT_LEGACY          , "Legacy audio connector" },
    { UAETT_SPDIF           , "S/PDIF interface" },
    { UAETT_1394DA          , "1394 DA stream" },
    { UAETT_1394DV          , "1394 DV stream soundtrack" },
    { UAFTT_UNDEFINED       , "Embedded Undefined" },
    { UAFTT_CALIB_NOISE     , "Level Calibration Noise Source" },
    { UAFTT_EQ_NOISE        , "Equalization Noise" },
    { UAFTT_CD              , "CD player" },
    { UAFTT_DAT             , "DAT" },
    { UAFTT_DCC             , "DCC" },
    { UAFTT_MINIDISK        , "MiniDisk" },
    { UAFTT_TAPE            , "Analog Tape" },
    { UAFTT_PHONO           , "Phonograph" },
    { UAFTT_VCR             , "VCR Audio" },
    { UAFTT_VIDEODISC       , "Video Disc Audio" },
    { UAFTT_DVD             , "DVD Audio" },
    { UAFTT_TV_TUNER        , "TV Tuner Audio" },
    { UAFTT_SAT_RX          , "Satellite Receiver Audio" },
    { UAFTT_CABLE_TUNER     , "Cable Tuner Audio" },
    { UAFTT_DSS             , "DSS Audio" },
    { UAFTT_RADIO_RX        , "Radio Receiver" },
    { UAFTT_RADIO_TX        , "Radio Transmitter" },
    { UAFTT_MULTITRACK      , "Multi-track Recorder" },
    { UAFTT_SYNTHESIZER     , "Synthesizer" },
    { 0x0000, NULL }
};
/* \\\ */

/* /// "Audio Format types" */
const struct AudioIDMap audioformattypes[] =
{
    { UAADF_TYPE_I_UNDEFINED     , "Undefined Type I" },
    { UAADF_PCM                  , "signed PCM" },
    { UAADF_PCM8                 , "8 bit unsigned PCM" },
    { UAADF_IEEE_FLOAT           , "IEEE Floating Point" },
    { UAADF_ALAW                 , "aLaw" },
    { UAADF_MULAW                , "µLaw" },

    { UAADF_TYPE_II_UNDEFINED    , "Undefined Type II" },
    { UAADF_MPEG                 , "MPEG" },
    { UAADF_AC3                  , "AC3" },

    { UAADF_TYPE_III_UNDEFINED   , "Undefined Type III" },
    { UAADF_IEC1937_AC3          , "IEC1937 AC3" },
    { UAADF_IEC1937_MPEG1_L1     , "IEC1937 MPEG1 Layer 1" },
    { UAADF_IEC1937_MPEG1_L2_3   , "IEC1937 MPEG1 Layer 2/3" },
    { UAADF_IEC1937_MPEG2_NOEXT  , "IEC1937 MPEG2 NoExt." },
    { UAADF_IEC1937_MPEG2_EXT    , "IEC1937 MPEG2 Ext." },
    { UAADF_IEC1937_MPEG2_L1_LS  , "IEC1937 MPEG2 Layer 1 LS" },
    { UAADF_IEC1937_MPEG2_L2_3_LS, "IEC1937 MPEG2 Layer 2/3 LS" },
    { 0x0000, NULL }
};
/* \\\ */

/* /// "Spatial Location Bits" */
const struct AudioIDMap audiospatiallocation[] =
{
    {  0, "Left Front (L)", },
    {  1, "Right Front (R)", },
    {  2, "Center Front (C)", },
    {  3, "Subwoofer (SW)", },
    {  4, "Left Surround (LS)", },
    {  5, "Right Surround (RS)", },
    {  6, "Left of Center (LC)", },
    {  7, "Right of Center (RC)", },
    {  8, "Surround (S)", },
    {  9, "Side Left (SL)", },
    { 10, "Side Right (SR)", },
    { 11, "Top (T)", },
    {  0, NULL }
};
/* \\\ */

/* /// "Spatial Location Bits (Bitmasks)" */
const struct AudioIDMap audiospatiallocationbits[] =
{
    { 0x03f, "5.1" },
    { 0x003, "Stereo" },
    { 1<< 0, "Left", },
    { 1<< 1, "Right", },
    { 1<< 2, "Center", },
    { 1<< 3, "Subwoofer", },
    { 1<< 4, "LSurr.", },
    { 1<< 5, "RSurr.", },
    { 1<< 6, "LCent.", },
    { 1<< 7, "RCent.", },
    { 1<< 8, "Surr.", },
    { 1<< 9, "SideL", },
    { 1<<10, "SideR", },
    { 1<<11, "Top", },
    {     0, NULL }
};
/* \\\ */

/* /// "Feature Unit Control Selectors" */
const struct AudioIDMap audiofeature[] =
{
    { UAFUCS_MUTE          , "Mute", },
    { UAFUCS_VOLUME        , "Volume", },
    { UAFUCS_BASS          , "Bass", },
    { UAFUCS_MID           , "Mid", },
    { UAFUCS_TREBLE        , "Treble", },
    { UAFUCS_EQUALIZER     , "Graphic Equalizer", },
    { UAFUCS_AUTOMATIC_GAIN, "Automatic Gain", },
    { UAFUCS_DELAY         , "Delay", },
    { UAFUCS_BASS_BOOST    , "Bass Boost", },
    { UAFUCS_LOUDNESS      , "Loudness", },
    {  0, NULL }
};
/* \\\ */

/* /// "Feature Unit Control Bits" */
const struct AudioIDMap audiofeaturebits[] =
{
    { UAFUF_MUTE          , "Mute", },
    { UAFUF_VOLUME        , "Volume", },
    { UAFUF_BASS          , "Bass", },
    { UAFUF_MID           , "Mid", },
    { UAFUF_TREBLE        , "Treble", },
    { UAFUF_EQUALIZER     , "Graphic Equalizer", },
    { UAFUF_AUTOMATIC_GAIN, "Automatic Gain", },
    { UAFUF_DELAY         , "Delay", },
    { UAFUF_BASS_BOOST    , "Bass Boost", },
    { UAFUF_LOUDNESS      , "Loudness", },
    {     0, NULL }
};
/* \\\ */

/* /// "Unit Types" */
const struct AudioIDMap audiounittype[] =
{
    { UDST_AUDIO_CTRL_INPUT_TERMINAL , "Input" },
    { UDST_AUDIO_CTRL_OUTPUT_TERMINAL, "Output" },
    { UDST_AUDIO_CTRL_MIXER_UNIT     , "Mixer" },
    { UDST_AUDIO_CTRL_SELECTOR_UNIT  , "Selector" },
    { UDST_AUDIO_CTRL_FEATURE_UNIT   , "Feature" },
    { UDST_AUDIO_CTRL_PROCESSING_UNIT, "Processing" },
    { UDST_AUDIO_CTRL_EXTENSION_UNIT , "Extension" },
    { 0, NULL }
};
/* \\\ */

/* /// "nNumToStr()" */
STRPTR nNumToStr(struct NepClassAudio *nch, UWORD type, ULONG id, STRPTR defstr)
{
    const struct AudioIDMap *aim = NULL;

    switch(type)
    {
        case NTS_TERMINALTYPE:
            aim = audioterminaltypes;
            break;

        case NTS_AUDIOFORMAT:
            aim = audioformattypes;
            break;

        case NTS_SPATIALLOCATION:
            aim = audiospatiallocation;
            break;

        case NTS_FEATURE:
            aim = audiofeature;
            break;

        case NTS_UNITTYPE:
            aim = audiounittype;
            break;
    }
    if(aim)
    {
        while(aim->aim_String)
        {
            if(aim->aim_ID == id)
            {
                return(aim->aim_String);
            }
            aim++;
        }
    }
    return(defstr);
}
/* \\\ */

/* /// "nConcatBitsStr()" */
STRPTR nConcatBitsStr(struct NepClassAudio *nch, UWORD type, ULONG bits)
{
    const struct AudioIDMap *iteraim = NULL;
    const struct AudioIDMap *aim = NULL;
    ULONG len = 0;
    STRPTR strptr;
    STRPTR locstr;
    STRPTR tarptr;
    ULONG iterbits;

    switch(type)
    {
        case NTS_SPATIALLOCATION:
            if(bits == 1)
            {
                return(psdCopyStr("Mono")); // avoid left
            }
            aim = audiospatiallocationbits;
            break;

        case NTS_FEATURE:
            aim = audiofeaturebits;
            break;
    }
    if(!aim)
    {
        return(NULL);
    }

    iteraim = aim;
    iterbits = bits;
    while((strptr = iteraim->aim_String) && iterbits)
    {
        if((iterbits & iteraim->aim_ID) == iteraim->aim_ID)
        {
            iterbits ^= iteraim->aim_ID;
            if(len)
            {
                len += 2;
            }
            while(*strptr++)
            {
                len++;
            }
        }
        iteraim++;
    }
    locstr = tarptr = psdAllocVec(len + 1);
    if(!locstr)
    {
        return(NULL);
    }
    iteraim = aim;
    iterbits = bits;
    while((strptr = iteraim->aim_String) && iterbits)
    {
        if((iterbits & iteraim->aim_ID) == iteraim->aim_ID)
        {
            iterbits ^= iteraim->aim_ID;
            if(locstr != tarptr)
            {
                *tarptr++ = ',';
                *tarptr++ = ' ';
            }
            while((*tarptr++ = *strptr++));
            tarptr--;
        }
        iteraim++;
    }
    return locstr;
}
/* \\\ */
