/*** Variables **************************************************************/
extern struct MUI_CustomClass *Keymap_CLASS;

/*** Macros *****************************************************************/
#define KeymapObject BOOPSIOBJMACRO_START(Keymap_CLASS->mcc_Class)

/*** Attributes *************************************************************/
#define MUIA_Keymap_Base (TAG_USER + 0x9000)

#define MUIA_Keymap_Keymap  (MUIA_Keymap_Base + 1)
