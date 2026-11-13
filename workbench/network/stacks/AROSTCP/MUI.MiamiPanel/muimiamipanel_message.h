
/****************************************************************************/

#define MAXFREENUM 32
#define STDSIZE    64

/****************************************************************************/

struct MPS_Msg *createMsg(ULONG size, struct MiamiPanelBase_intern *MiamiPanelBaseIntern);
void freeMsg(struct MPS_Msg *msg, struct MiamiPanelBase_intern *MiamiPanelBaseIntern);
