#ifndef POPOGUI_H
#define POPOGUI_H POPOGUI_H

struct PoPoData
{
	BOOL nix;
};

struct PsdPoPoGadgets
{
    struct Node         pog_Node;         /* Linkage */
    struct PsdDevice   *pog_Device;       /* Linkage to the device used */
    ULONG              *pog_GroupObj;     /* Group object to link */
    ULONG              *pog_BodyObj;      /* Text body */
    ULONG              *pog_GadgetObj[4]; /* gadgets */
    struct timeval      pog_TimeoutTime;  /* time at which the thing destroys itself */
    BOOL                pog_ShowMe;       /* popup window? */
    BOOL                pog_WaitBinding;  /* wait for class scan finished before popping up */
};

struct PsdPoPoSound
{
    struct Node         pps_Node;         /* Linkage */
    ULONG              *pps_DTHandle;     /* Handle to DataType object */
};

void pPoPoGUITaskCleanup(struct PsdBase *ps);
void pEventHandler(struct PsdBase *ps);
void pFreePoPoGadgets(struct PsdBase *ps, struct PsdPoPoGadgets *pog);
struct PsdPoPoGadgets * pAllocPoPoGadgets(struct PsdBase *ps, STRPTR body, STRPTR *gad);
STRPTR pBindingsString(struct PsdBase *ps, struct PsdDevice *pd);
ULONG pCheckConfigurable(struct PsdBase *ps, struct PsdDevice *pd);
struct PsdPoPoSound * pPoPoLoadSound(struct PsdBase *ps, STRPTR name);
BOOL pPoPoPlaySound(struct PsdBase *ps, STRPTR name);
void pPoPoFreeSound(struct PsdBase *ps, struct PsdPoPoSound *pps);

#define PPF_HasBinding    0x0001
#define PPF_HasClassGUI   0x0010
#define PPF_HasClsConfig  0x0020
#define PPF_HasBindingGUI 0x0100
#define PPF_HasBindConfig 0x0200

#define TAGBASE_PoPo (TAG_USER | 29<<16)
#define MUIM_PoPo_RemInfo           (TAGBASE_PoPo | 0x0010)
#define MUIM_PoPo_ConfigureClass    (TAGBASE_PoPo | 0x0011)
#define MUIM_PoPo_ConfigureBinding  (TAGBASE_PoPo | 0x0012)
#define MUIM_PoPo_ShutUp            (TAGBASE_PoPo | 0x0013)
#define MUIM_PoPo_DisablePort       (TAGBASE_PoPo | 0x0014)
#define MUIM_PoPo_PowerCyclePort    (TAGBASE_PoPo | 0x0015)
#define MUIM_PoPo_Sticky            (TAGBASE_PoPo | 0x0020)
#define MUIM_PoPo_SavePrefs         (TAGBASE_PoPo | 0x0021)
#define MUIM_PoPo_About             (TAGBASE_PoPo | 0x0030)
#define MUIM_PoPo_OpenTrident       (TAGBASE_PoPo | 0x0031)

AROS_UFP3(ULONG, PoPoDispatcher,
                 AROS_UFPA(struct IClass *, cl, A0),
                 AROS_UFPA(Object *, obj, A2),
                 AROS_UFPA(Msg, msg, A1));

#endif // POPOGUI_H
