/*****************************************************************************
** This is the IconList custom class, a sub class of List.mui.
******************************************************************************/

#ifndef ICONLISTCLASS_H
#define ICONLISTCLASS_H

#define MAXMASONICONS 25

struct IconListData
{
    Object *mimainlist[MAXMASONICONS];
    Object *mimainbody[MAXMASONICONS];
};

#define TAGBASE_IconList (TAG_USER | 25<<16)

AROS_UFP3(IPTR, IconListDispatcher,
          AROS_UFPA(struct IClass *, cl, A0),
          AROS_UFPA(Object *, obj, A2),
          AROS_UFPA(Msg, msg, A1));

#endif
