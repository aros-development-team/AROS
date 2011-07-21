#include <hardware/custom.h>

AROS_UFP5(void, IntServer,
    AROS_UFPA(ULONG, intMask, D0),
    AROS_UFPA(struct Custom *, custom, A0),
    AROS_UFPA(struct List *, intList, A1),
    AROS_UFPA(APTR, intCode, A5),
    AROS_UFPA(struct ExecBase *, SysBase, A6));

AROS_UFP5(void, VBlankServer,
    AROS_UFPA(ULONG, intMask, D0),
    AROS_UFPA(struct Custom *, custom, A0),
    AROS_UFPA(struct List *, intList, A1),
    AROS_UFPA(APTR, intCode, A5),
    AROS_UFPA(struct ExecBase *, SysBase, A6));

/* Implementation is in cause.c */
AROS_UFP5(void, SoftIntDispatch,
    AROS_UFPA(ULONG, intReady, D1),
    AROS_UFPA(volatile struct Custom *, custom, A0),
    AROS_UFPA(IPTR, intData, A1),
    AROS_UFPA(ULONG_FUNC, intCode, A5),
    AROS_UFPA(struct ExecBase *, SysBase, A6));
