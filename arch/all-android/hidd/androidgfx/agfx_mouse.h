#define CLID_Hidd_AMouse "hidd.mouse.android"

struct mouse_data
{
    VOID (*mouse_callback)(APTR, struct pHidd_Mouse_Event *);
    APTR callbackdata;
};
