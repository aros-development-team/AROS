
/* Tags */

#define MUIA_Framedisplay_Spec        0x80421794 // V11 isg struct MUI_FrameSpec
#define MUIA_Imagedisplay_Spec        0x8042a547 // V11 isg struct MUI_ImageSpec
#define MUIA_Imageadjust_Type         0x80422f2b // V11 i.. LONG
#define MUIA_Prop_DeltaFactor         0x80427C5E
#define MUIA_Prop_DoSmooth 	          0x804236ce // V4  i.. LONG
#define MUIA_Prop_Release             0x80429839
#define MUIA_Window_DisableKeys       0x80424c36 // V15 isg ULONG
#define MUIA_Application_UsedClasses  0x8042e9a7 // V20 isg STRPTR*

#define MUIM_GoActive			            0x8042491a
#define MUIM_GoInactive 		          0x80422c0c
#define MUIM_Mccprefs_RegisterGadget  0x80424828 // V20

#define MUI_EHF_GUIMODE               (1<<1)     // set this if you dont want your handler to be called

struct MUI_ImageSpec
{
	char buf[64];
};

struct MUI_FrameSpec
{
	char buf[8];
};

