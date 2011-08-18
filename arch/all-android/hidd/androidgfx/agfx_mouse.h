#define CLID_Hidd_AMouse "hidd.mouse.android"

struct mouse_data
{
    VOID (*mouse_callback)(APTR, struct pHidd_Mouse_ExtEvent *);
    APTR callbackdata;
};

struct PointerEvent;

void AMouse_ReportEvent(struct mouse_data *data, struct PointerEvent *pkt);
void AMouse_ReportTouch(struct mouse_data *data, struct PointerEvent *pkt);
