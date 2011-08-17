#define CLID_Hidd_AKbd "hidd.kbd.android"

struct kbd_data
{
    VOID (*callback)(APTR, UWORD);
    APTR   callbackdata;
};
