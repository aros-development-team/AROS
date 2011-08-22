#define CLID_Hidd_AKbd "hidd.kbd.android"

struct kbd_data
{
    VOID (*callback)(APTR, UWORD);
    APTR   callbackdata;
};

struct KeyEvent;

void AKbd_ReportKey(struct kbd_data *data, struct KeyEvent *e);
