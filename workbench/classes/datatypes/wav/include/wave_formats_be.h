/* Format IDs: (in bigendian format) */

#define WAVE_FORMAT_UNKNOWN				0x0000 /* Unknown Format */
#define WAVE_FORMAT_PCM					0x0100 /* PCM */
#define WAVE_FORMAT_ADPCM				0x0200 /* Microsoft ADPCM Format */
#define WAVE_FORMAT_IEEE_FLOAT			0x0300 /* IEEE Float */
#define WAVE_FORMAT_VSELP				0x0400 /* Compaq Computer's VSELP */
#define WAVE_FORMAT_IBM_CSVD			0x0500 /* IBM CVSD */
#define WAVE_FORMAT_ALAW				0x0600 /* ALAW */
#define WAVE_FORMAT_MULAW				0x0700 /* MULAW */
#define WAVE_FORMAT_OKI_ADPCM			0x1000 /* OKI ADPCM */
#define WAVE_FORMAT_IMA_ADPCM			0x1100
#define WAVE_FORMAT_DVI_ADPCM       	0x1100 /* Intel's DVI ADPCM (same as IMA ADPCM) */
#define WAVE_FORMAT_MEDIASPACE_ADPCM	0x1200 /* Videologic's MediaSpace ADPCM */
#define WAVE_FORMAT_SIERRA_ADPCM		0x1300 /* Sierra ADPCM */
#define WAVE_FORMAT_G723_ADPCM			0x1400 /* G.723 ADPCM */
#define WAVE_FORMAT_DIGISTD				0x1500 /* DSP Solution's DIGISTD */
#define WAVE_FORMAT_DIGIFIX				0x1600 /* DSP Solution's DIGIFIX */
#define WAVE_FORMAT_DIALOGIC_OKI_ADPCM	0x1700 /* Dialogic OKI ADPCM */
#define WAVE_FORMAT_MEDIAVISION_ADPCM	0x1800 /* MediaVision ADPCM */
#define WAVE_FORMAT_CU_CODEC			0x1900 /* HP CU */
#define WAVE_FORMAT_YAMAHA_ADPCM		0x2000 /* Yamaha ADPCM */
#define WAVE_FORMAT_SONARC				0x2100 /* Speech Compression's Sonarc */
#define WAVE_FORMAT_TRUESPEECH			0x2200 /* DSP Group's True Speech */
#define WAVE_FORMAT_ECHOSC1				0x2300 /* Echo Speech's EchoSC1 */
#define WAVE_FORMAT_AUDIOFILE_AF36		0x2400 /* Audiofile AF36 */
#define WAVE_FORMAT_APTX				0x2500 /* APTX */
#define WAVE_FORMAT_AUDIOFILE_AF10		0x2600 /* AudioFile AF10 */
#define WAVE_FORMAT_PROSODY_1612		0x2700 /* Prosody 1612 */
#define WAVE_FORMAT_LRC					0x2800 /* LRC */
#define WAVE_FORMAT_AC2					0x3000 /* Dolby AC2 */
#define WAVE_FORMAT_GSM610				0x3100 /* GSM610 */
#define WAVE_FORMAT_MSNAUDIO			0x3200 /* MSNAudio */
#define WAVE_FORMAT_ANTEX_ADPCME		0x3300 /* Antex ADPCME */
#define WAVE_FORMAT_CONTROL_RES_VQLPC	0x3400 /* Control Res VQLPC */
#define WAVE_FORMAT_DIGIREAL			0x3500 /* Digireal */
#define WAVE_FORMAT_DIGIADPCM			0x3600 /* DigiADPCM */
#define WAVE_FORMAT_CONTROL_RES_CR10    0x3700 /* Control Res CR10 */
#define WAVE_FORMAT_VBXADPCM			0x3800 /* NMS VBXADPCM */
#define WAVE_FORMAT_ROLAND_RDAC			0x3900 /* Roland RDAC */
#define WAVE_FORMAT_ECHOSC3				0x3A00 /* EchoSC3 */
#define WAVE_FORMAT_ROCKWELL_ADPCM		0x3B00 /* Rockwell ADPCM */
#define WAVE_FORMAT_ROCKWELL_DIGITALK	0x3C00 /* Rockwell Digit LK */
#define WAVE_FORMAT_XEBEC				0x3D00 /* Xebec */
#define WAVE_FORMAT_G721_ADPCM			0x4000 /* Antex Electronics G.721 */
#define WAVE_FORMAT_G728_CELP			0x4100 /* G.728 CELP */
#define WAVE_FORMAT_MSG723				0x4200 /* MSG723 */
#define WAVE_FORMAT_MPEG				0x5000 /* MPEG Layer 1,2 */
#define WAVE_FORMAT_RT24				0x5100 /* RT24 */
#define WAVE_FORMAT_PAC					0x5100 /* PAC */
#define WAVE_FORMAT_MPEGLAYER3			0x5500 /* ISO/MPEG Layer3 Format Tag */
#define WAVE_FORMAT_CIRRUS				0x5900 /* Cirrus */
#define WAVE_FORMAT_ESPCM				0x6100 /* ESPCM */
#define WAVE_FORMAT_VOXWARE				0x6200 /* Voxware (obsolete) */
#define WAVE_FORMAT_CANOPUS_ATRAC		0x6300 /* Canopus Atrac */
#define WAVE_FORMAT_G726_ADPCM			0x6400 /* G.726 ADPCM */
#define WAVE_FORMAT_G722_ADPCM			0x6500 /* G.722 ADPCM */
#define WAVE_FORMAT_DSAT				0x6600 /* DSAT */
#define WAVE_FORMAT_DSAT_DISPLAY		0x6700 /* DSAT Display */
#define WAVE_FORMAT_VOXWARE_BYTE_ALIGNED	0x6900 /* Voxware Byte Aligned (obsolete) */
#define WAVE_FORMAT_VOXWARE_AC8			0x7000 /* Voxware AC8 (obsolete) */
#define WAVE_FORMAT_VOXWARE_AC10		0x7100 /* Voxware AC10 (obsolete) */
#define WAVE_FORMAT_VOXWARE_AC16		0x7200 /* Voxware AC16 (obsolete) */
#define WAVE_FORMAT_VOXWARE_AC20		0x7300 /* Voxware AC20 (obsolete) */
#define WAVE_FORMAT_VOXWARE_RT24		0x7400 /* Voxware MetaVoice (obsolete) */
#define WAVE_FORMAT_VOXWARE_RT29		0x7500 /* Voxware MetaSound (obsolete) */
#define WAVE_FORMAT_VOXWARE_RT29HW		0x7600 /* Voxware RT29HW (obsolete) */
#define WAVE_FORMAT_VOXWARE_VR12		0x7700 /* Voxware VR12 (obsolete) */
#define WAVE_FORMAT_VOXWARE_VR18		0x7800 /* Voxware VR18 (obsolete) */
#define WAVE_FORMAT_VOXWARE_TQ40		0x7900 /* Voxware TQ40 (obsolete) */
#define WAVE_FORMAT_SOFTSOUND			0x8000 /* Softsound */
#define WAVE_FORMAT_VOXWARE_TQ60		0x8100 /* Voxware TQ60 (obsolete) */
#define WAVE_FORMAT_MSRT24				0x8200 /* MSRT24 */
#define WAVE_FORMAT_G729A				0x8300 /* G.729A */
#define WAVE_FORMAT_MVI_MV12			0x8400 /* MVI MV12 */
#define WAVE_FORMAT_DF_G726				0x8500 /* DF G.726 */
#define WAVE_FORMAT_DF_GSM610			0x8600 /* DF GSM610 */
/* #define WAVE_FORMAT_ISIAUDIO			0x8800*/ /* ISIAudio */
#define WAVE_FORMAT_ONLIVE				0x8900 /* Onlive */
#define WAVE_FORMAT_SBC24				0x9100 /* SBC24 */
#define WAVE_FORMAT_DOLBY_AC3_SPDIF		0x9200 /* Dolby AC3 SPDIF */
#define WAVE_FORMAT_ZYXEL_ADPCM			0x9700 /* ZyXEL ADPCM */
#define WAVE_FORMAT_PHILIPS_LPCBB		0x9800 /* Philips LPCBB */
#define WAVE_FORMAT_PACKED				0x9900 /* Packed */
#define WAVE_FORMAT_RHETOREX_ADPCM		0x0001 /* Rhetorex ADPCM */
#define WAVE_FORMAT_IRAT				0x0101 /* BeCubed Software's IRAT */
#define WAVE_FORMAT_VIVO_G723			0x1101 /* Vivo G.723 */
#define WAVE_FORMAT_VIVO_SIREN			0x1201 /* Vivo Siren */
#define WAVE_FORMAT_DIGITAL_G723		0x2301 /* Digital G.723 */
#define WAVE_FORMAT_WMA1				0x6001 /* WMA version 1 */
#define WAVE_FORMAT_WMA2				0x6101 /* WMA (v2) 7, 8, 9 Series */
#define WAVE_FORMAT_WMAP				0x6201 /* WMA 9 Professional */
#define WAVE_FORMAT_WMAL				0x6301 /* WMA 9 Lossless */
#define WAVE_FORMAT_CREATIVE_ADPCM		0x0002 /* Creative ADPCM */
#define WAVE_FORMAT_CREATIVE_FASTSPEECH8	0x0202 /* Creative FastSpeech8 */
#define WAVE_FORMAT_CREATIVE_FASTSPEECH10	0x0302 /* Creative FastSpeech10 */
#define WAVE_FORMAT_QUARTERDECK			0x2002 /* Quarterdeck */
#define WAVE_FORMAT_FM_TOWNS_SND		0x0003 /* FM Towns Snd */
#define WAVE_FORMAT_BTV_DIGITAL			0x0004 /* BTV Digital */
#define WAVE_FORMAT_VME_VMPCM			0x8006 /* VME VMPCM */
#define WAVE_FORMAT_OLIGSM				0x0010 /* OLIGSM */
#define WAVE_FORMAT_OLIADPCM			0x0110 /* OLIADPCM */
#define WAVE_FORMAT_OLICELP				0x0210 /* OLICELP */
#define WAVE_FORMAT_OLISBC				0x0310 /* OLISBC */
#define WAVE_FORMAT_OLIOPR				0x0410 /* OLIOPR */
#define WAVE_FORMAT_LH_CODEC			0x0011 /* LH Codec */
#define WAVE_FORMAT_NORRIS				0x0014 /* Norris */
/* #define WAVE_FORMAT_ISIAUDIO			0x0114*/ /* ISIAudio */
#define WAVE_FORMAT_SOUNDSPACE_MUSICOMPRESS	0x0015 /* Soundspace Music Compression */
#define WAVE_FORMAT_DVM					0x0020 /* DVM */
#define WAVE_FORMAT_AAC					0x06A1 /* ISO/MPEG-4 advanced audio Coding */
#define WAVE_FORMAT_EXTENSIBLE			0xFEFF /* SubFormat */
#define WAVE_FORMAT_DEVELOPMENT			0xFFFF /* Development */

#define WAVE_FORMAT_IBM_MULAW			0x0101 /* IBM MULAW */
#define WAVE_FORMAT_IBM_ALAW			0x0201 /* IBM ALAW */
#define WAVE_FORMAT_IBM_ADPCM			0x0301 /* IBM ADPCM */
