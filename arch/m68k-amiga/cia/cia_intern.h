#include <stddef.h>

#define VECTORS_NUM 5

/* One ICR bit's handler, as the Kickstart dispatcher calls it:
 * A1 = iv_Data, A5 = iv_Code. */
struct CIAIntVector
{
    APTR              iv_Data;
    VOID_FUNC         iv_Code;
    struct Interrupt *iv_Node;
};

/*
 * Laid out like the Kickstart 3.1 CIABase up to Vectors[] and the ExecBase
 * pointer: games (Gloom, for one) hook ICR handlers by writing iv_Data and
 * iv_Code straight into Vectors[], and the dispatcher must honour that.
 */
struct CIABase
{
    struct Library lib;                         /* 0x00 */
    volatile struct CIA *hw;                    /* 0x22 */
    UWORD inten_mask;                           /* 0x26 */
    UBYTE enable_mask;                          /* 0x28 */
    UBYTE active_mask;                          /* 0x29 */
    struct Interrupt ciaint;                    /* 0x2A */
    struct CIAIntVector Vectors[VECTORS_NUM];   /* 0x40 */
    struct ExecBase *sysbase;                   /* 0x7C */
    UBYTE executing_mask;
    void (*hook_func)(APTR, APTR, WORD);
    APTR hook_data;
};

typedef char cia_vectors_offset_check[(offsetof(struct CIABase, Vectors) == 0x40) ? 1 : -1];
typedef char cia_sysbase_offset_check[(offsetof(struct CIABase, sysbase) == 0x7C) ? 1 : -1];
