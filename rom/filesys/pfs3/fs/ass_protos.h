/* Prototypes for functions defined in
AssRoutines.asm
 */
void InitStack(void);
void AfsDie(void);
void myStackSwap (struct StackSwapStruct *);
ULONG divide(UWORD, UWORD);
void ctodstr(void *, void *);
void intltoupper(void *);
BOOL intlcmp(void*, void *);
BOOL intlcdcmp(void*, void *);
#ifndef __SASC
int stcd_i(const char *in, int *ivalue);
int stcu_d(char *out, unsigned int val);
#endif
