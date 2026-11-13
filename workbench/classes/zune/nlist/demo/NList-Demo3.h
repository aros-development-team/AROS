
extern struct MUI_CustomClass *NLI_Class;

extern struct MUI_CustomClass *NLI_Create(void);
extern void NLI_Delete(void);


#define MUIA_NLIMG_Spec 0x9d51ffff


#define NImageObject NewObject(NLI_Class->mcc_Class,NULL,MUIA_FillArea,FALSE,NoFrame,End

