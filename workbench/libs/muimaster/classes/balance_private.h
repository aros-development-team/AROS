#ifndef _BALANCE_PRIVATE_H_
#define _BALANCE_PRIVATE_H_

typedef enum
{
    NOT_CLICKED,
    CLICKED,
    SHIFT_CLICKED,
} State;

/*** Instance data **********************************************************/
struct Balance_DATA
{
    struct MUI_EventHandlerNode ehn;
    ULONG horizgroup;
    State state;
    LONG clickpos;
    LONG lastpos;
    LONG total_weight;
    LONG first_bound;
    LONG second_bound;
    struct List *objs;
    Object *obj_before;
    Object *obj_after;
    LONG lsum;
    LONG oldWeightA;
    LONG oldWeightB;
    LONG rsum;
    LONG lsize;
    LONG rsize;
    WORD lsiblings;
    WORD rsiblings;
    WORD lazy;
};

#endif /* _BALANCE_PRIVATE_H_ */
