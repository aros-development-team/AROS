/* Format IDs: (in littleendian format) */

#define WAVE_FORMAT_UNKNOWN				0x0000 /* Unknown Format */
#define WAVE_FORMAT_PCM					0x0001 /* PCM */
#define WAVE_FORMAT_ADPCM				0x0002 /* Microsoft ADPCM Format */
#define WAVE_FORMAT_IEEE_FLOAT			0x0003 /* IEEE Float */
#define WAVE_FORMAT_VSELP				0x0004 /* Compaq Computer's VSELP */
#define WAVE_FORMAT_IBM_CSVD			0x0005 /* IBM CVSD */
#define WAVE_FORMAT_ALAW				0x0006 /* ALAW */
#define WAVE_FORMAT_MULAW				0x0007 /* MULAW */
#define WAVE_FORMAT_OKI_ADPCM			0x0010 /* OKI ADPCM */
#define WAVE_FORMAT_IMA_ADPCM			0x0011
#define WAVE_FORMAT_DVI_ADPCM			0x0011 /* Intel's DVI ADPCM */
#define WAVE_FORMAT_MEDIASPACE_ADPCM	0x0012 /* Videologic's MediaSpace ADPCM */
#define WAVE_FORMAT_SIERRA_ADPCM		0x0013 /* Sierra ADPCM */
#define WAVE_FORMAT_G723_ADPCM			0x0014 /* G.723 ADPCM */
#define WAVE_FORMAT_DIGISTD				0x0015 /* DSP Solution's DIGISTD */
#define WAVE_FORMAT_DIGIFIX				0x0016 /* DSP Solution's DIGIFIX */
#define WAVE_FORMAT_DIALOGIC_OKI_ADPCM	0x0017 /* Dialogic OKI ADPCM */
#define WAVE_FORMAT_MEDIAVISION_ADPCM	0x0018 /* MediaVision ADPCM */
#define WAVE_FORMAT_CU_CODEC			0x0019 /* HP CU */
#define WAVE_FORMAT_YAMAHA_ADPCM		0x0020 /* Yamaha ADPCM */
#define WAVE_FORMAT_SONARC				0x0021 /* Speech Compression's Sonarc */
#define WAVE_FORMAT_TRUESPEECH			0x0022 /* DSP Group's True Speech */
#define WAVE_FORMAT_ECHOSC1				0x0023 /* Echo Speech's EchoSC1 */
#define WAVE_FORMAT_AUDIOFILE_AF36		0x0024 /* Audiofile AF36 */
#define WAVE_FORMAT_APTX				0x0025 /* APTX */
#define WAVE_FORMAT_AUDIOFILE_AF10		0x0026 /* AudioFile AF10 */
#define WAVE_FORMAT_PROSODY_1612		0x0027 /* Prosody 1612 */
#define WAVE_FORMAT_LRC					0x0028 /* LRC */
#define WAVE_FORMAT_AC2					0x0030 /* Dolby AC2 */
#define WAVE_FORMAT_GSM610				0x0031 /* GSM610 */
#define WAVE_FORMAT_MSNAUDIO			0x0032 /* MSNAudio */
#define WAVE_FORMAT_ANTEX_ADPCME		0x0033 /* Antex ADPCME */
#define WAVE_FORMAT_CONTROL_RES_VQLPC	0x0034 /* Control Res VQLPC */
#define WAVE_FORMAT_DIGIREAL			0x0035 /* Digireal */
#define WAVE_FORMAT_DIGIADPCM			0x0036 /* DigiADPCM */
#define WAVE_FORMAT_CONTROL_RES_CR10	0x0037 /* Control Res CR10 */
#define WAVE_FORMAT_VBXADPCM			0x0038 /* NMS VBXADPCM */
#define WAVE_FORMAT_ROLAND_RDAC			0x0039 /* Roland RDAC */
#define WAVE_FORMAT_ECHOSC3				0x003A /* EchoSC3 */
#define WAVE_FORMAT_ROCKWELL_ADPCM		0x003B /* Rockwell ADPCM */
#define WAVE_FORMAT_ROCKWELL_DIGITALK	0x003C /* Rockwell Digit LK */
#define WAVE_FORMAT_XEBEC				0x003D /* Xebec */
#define WAVE_FORMAT_G721_ADPCM			0x0040 /* Antex Electronics G.721 */
#define WAVE_FORMAT_G728_CELP			0x0041 /* G.728 CELP */
#define WAVE_FORMAT_MSG723				0x0042 /* MSG723 */
#define WAVE_FORMAT_MPEG				0x0050 /* MPEG Layer 1,2 */
#define WAVE_FORMAT_RT24				0x0051 /* RT24 */
#define WAVE_FORMAT_PAC					0x0051 /* PAC */
#define WAVE_FORMAT_MPEGLAYER3			0x0055 /* ISO/MPEG Layer3 Format Tag */
#define WAVE_FORMAT_CIRRUS				0x0059 /* Cirrus */
#define WAVE_FORMAT_ESPCM				0x0061 /* ESPCM */
#define WAVE_FORMAT_VOXWARE				0x0062 /* Voxware (obsolete) */
#define WAVE_FORMAT_CANOPUS_ATRAC		0x0063 /* Canopus Atrac */
#define WAVE_FORMAT_G726_ADPCM			0x0064 /* G.726 ADPCM */
#define WAVE_FORMAT_G722_ADPCM			0x0065 /* G.722 ADPCM */
#define WAVE_FORMAT_DSAT				0x0066 /* DSAT */
#define WAVE_FORMAT_DSAT_DISPLAY		0x0067 /* DSAT Display */
#define WAVE_FORMAT_VOXWARE_BYTE_ALIGNED	0x0069 /* Voxware Byte Aligned (obsolete) */
#define WAVE_FORMAT_VOXWARE_AC8			0x0070 /* Voxware AC8 (obsolete) */
#define WAVE_FORMAT_VOXWARE_AC10		0x0071 /* Voxware AC10 (obsolete) */
#define WAVE_FORMAT_VOXWARE_AC16		0x0072 /* Voxware AC16 (obsolete) */
#define WAVE_FORMAT_VOXWARE_AC20		0x0073 /* Voxware AC20 (obsolete) */
#define WAVE_FORMAT_VOXWARE_RT24		0x0074 /* Voxware MetaVoice (obsolete) */
#define WAVE_FORMAT_VOXWARE_RT29		0x0075 /* Voxware MetaSound (obsolete) */
#define WAVE_FORMAT_VOXWARE_RT29HW		0x0076 /* Voxware RT29HW (obsolete) */
#define WAVE_FORMAT_VOXWARE_VR12		0x0077 /* Voxware VR12 (obsolete) */
#define WAVE_FORMAT_VOXWARE_VR18		0x0078 /* Voxware VR18 (obsolete) */
#define WAVE_FORMAT_VOXWARE_TQ40		0x0079 /* Voxware TQ40 (obsolete) */
#define WAVE_FORMAT_SOFTSOUND			0x0080 /* Softsound */
#define WAVE_FORMAT_VOXWARE_TQ60		0x0081 /* Voxware TQ60 (obsolete) */
#define WAVE_FORMAT_MSRT24				0x0082 /* MSRT24 */
#define WAVE_FORMAT_G729A				0x0083 /* G.729A */
#define WAVE_FORMAT_MVI_MV12			0x0084 /* MVI MV12 */
#define WAVE_FORMAT_DF_G726				0x0085 /* DF G.726 */
#define WAVE_FORMAT_DF_GSM610			0x0086 /* DF GSM610 */
#define WAVE_FORMAT_ISIAUDIO			0x0088 /* ISIAudio */
#define WAVE_FORMAT_ONLIVE				0x0089 /* Onlive */
#define WAVE_FORMAT_SBC24				0x0091 /* SBC24 */
#define WAVE_FORMAT_DOLBY_AC3_SPDIF		0x0092 /* Dolby AC3 SPDIF */
#define WAVE_FORMAT_ZYXEL_ADPCM			0x0097 /* ZyXEL ADPCM */
#define WAVE_FORMAT_PHILIPS_LPCBB		0x0098 /* Philips LPCBB */
#define WAVE_FORMAT_PACKED				0x0099 /* Packed */
#define WAVE_FORMAT_RHETOREX_ADPCM		0x0100 /* Rhetorex ADPCM */
#define WAVE_FORMAT_IRAT				0x0101 /* BeCubed Software's IRAT */
#define WAVE_FORMAT_VIVO_G723			0x0111 /* Vivo G.723 */
#define WAVE_FORMAT_VIVO_SIREN			0x0112 /* Vivo Siren */
#define WAVE_FORMAT_DIGITAL_G723		0x0123 /* Digital G.723 */
#define WAVE_FORMAT_WMA1				0x0160 /* WMA version 1 */
#define WAVE_FORMAT_WMA2				0x0161 /* WMA (v2) 7, 8, 9 Series */
#define WAVE_FORMAT_WMAP				0x0162 /* WMA 9 Professional */
#define WAVE_FORMAT_WMAL				0x0163 /* WMA 9 Lossless */
#define WAVE_FORMAT_CREATIVE_ADPCM		0x0200 /* Creative ADPCM */
#define WAVE_FORMAT_CREATIVE_FASTSPEECH8	0x0202 /* Creative FastSpeech8 */
#define WAVE_FORMAT_CREATIVE_FASTSPEECH10	0x0203 /* Creative FastSpeech10 */
#define WAVE_FORMAT_QUARTERDECK			0x0220 /* Quarterdeck */
#define WAVE_FORMAT_FM_TOWNS_SND		0x0300 /* FM Towns Snd */
#define WAVE_FORMAT_BTV_DIGITAL			0x0400 /* BTV Digital */
#define WAVE_FORMAT_VME_VMPCM			0x0680 /* VME VMPCM */
#define WAVE_FORMAT_OLIGSM				0x1000 /* OLIGSM */
#define WAVE_FORMAT_OLIADPCM			0x1001 /* OLIADPCM */
#define WAVE_FORMAT_OLICELP				0x1002 /* OLICELP */
#define WAVE_FORMAT_OLISBC				0x1003 /* OLISBC */
#define WAVE_FORMAT_OLIOPR				0x1004 /* OLIOPR */
#define WAVE_FORMAT_LH_CODEC			0x1100 /* LH Codec */
#define WAVE_FORMAT_NORRIS				0x1400 /* Norris */
#define WAVE_FORMAT_ISIAUDIO			0x1401 /* ISIAudio */
#define WAVE_FORMAT_SOUNDSPACE_MUSICOMPRESS	0x1500 /* Soundspace Music Compression */
#define WAVE_FORMAT_DVM					0x2000 /* DVM */
#define WAVE_FORMAT_AAC					0xA106 /* ISO/MPEG-4 advanced audio Coding */
#define WAVE_FORMAT_EXTENSIBLE			0xFFFE /* SubFormat */
#define WAVE_FORMAT_DEVELOPMENT			0xFFFF /* Development */

#define WAVE_FORMAT_IBM_MULAW			0x0101 /* IBM MULAW? */
#define WAVE_FORMAT_IBM_ALAW			0x0102 /* IBM ALAW? */
#define WAVE_FORMAT_IBM_ADPCM			0x0103 /* IBM ADPCM? */
