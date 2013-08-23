#ifndef MUIA_Framedisplay_Spec
#define MUIA_Framedisplay_Spec        0x80421794UL // V11 isg struct MUI_FrameSpec
#endif
#ifndef MUIA_Imagedisplay_Spec
#define MUIA_Imagedisplay_Spec        0x8042a547UL // V11 isg struct MUI_ImageSpec
#endif
#ifndef MUIA_Imageadjust_Type
#define MUIA_Imageadjust_Type         0x80422f2bUL // V11 i.. LONG
#endif
#ifndef MUIM_GoActive
#define MUIM_GoActive			            0x8042491aUL
#endif
#ifndef MUIM_GoInactive
#define MUIM_GoInactive 		          0x80422c0cUL
#endif
#ifndef MUIM_Mccprefs_RegisterGadget
#define MUIM_Mccprefs_RegisterGadget  0x80424828UL // V20
#endif
#ifndef MUIA_Text_Copy
#define MUIA_Text_Copy                0x80427727UL /* V20 i.. BOOL              */
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
  char buf[128];
};

#if !defined(__AROS__)
struct MUI_FrameSpec
{
  char buf[128];
};
#endif
