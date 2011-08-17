#define CLID_Hidd_AMouse "hidd.mouse.android"

struct mouse_data
{
    VOID (*mouse_callback)(APTR, struct pHidd_Mouse_Event *);
    APTR callbackdata;
};

struct PointerEvent;

void AMouse_ReportEvent(struct mouse_data *data, struct PointerEvent *pkt, UWORD flags);
