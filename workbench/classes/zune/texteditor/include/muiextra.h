#ifndef MUIA_Framedisplay_Spec
#define MUIA_Framedisplay_Spec        0x80421794 // V11 isg struct MUI_FrameSpec
#endif
#ifndef MUIA_Imagedisplay_Spec
#define MUIA_Imagedisplay_Spec        0x8042a547 // V11 isg struct MUI_ImageSpec
#endif
#ifndef MUIA_Imageadjust_Type
#define MUIA_Imageadjust_Type         0x80422f2b // V11 i.. LONG
#endif
#ifndef MUIA_Prop_Release
#define MUIA_Prop_Release             0x80429839
#endif
#ifndef MUIA_Prop_DeltaFactor
#define MUIA_Prop_DeltaFactor         0x80427C5E
#endif
#ifndef MUIA_Prop_DoSmooth
#define MUIA_Prop_DoSmooth            0x804236ce // V4  i.. LONG
#endif
#ifndef MUIM_GoActive
#define MUIM_GoActive			        0x8042491a
#endif
#ifndef MUIM_GoInactive
#define MUIM_GoInactive 		        0x80422c0c
#endif
#ifndef MUIM_Mccprefs_RegisterGadget
#define MUIM_Mccprefs_RegisterGadget  0x80424828 // V20
#endif
#ifndef MBQ_MUI_MAXMAX
#define MBQ_MUI_MAXMAX (10000)          /* use this for unlimited MaxWidth/Height */
#endif
#ifndef PopframeObject
#define PopframeObject MUI_NewObject(MUIC_Popframe
#endif
#ifndef PopimageObject
#define PopimageObject MUI_NewObject("Popimage.mui"
#endif
#ifndef CrawlingObject
#define CrawlingObject MUI_NewObject("Crawling.mcc"
#endif

struct MUI_ImageSpec
{
  char buf[64];
};

#if !defined(__AROS__)
struct MUI_FrameSpec
{
  char buf[8];
};
#endif
