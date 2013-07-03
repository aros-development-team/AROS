#define VECTORS_NUM 5

struct CIABase
{
    struct Library lib;
    volatile struct CIA *hw;
    struct Interrupt ciaint;
    struct Interrupt *Vectors[VECTORS_NUM];
    UWORD inten_mask;
    UBYTE enable_mask;
    UBYTE active_mask;
    UBYTE executing_mask;
    void (*hook_func)(APTR, APTR, WORD);
    APTR hook_data;
};
