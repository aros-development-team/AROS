/*****************************************************************************
** This is the CfgList custom class, a sub class of List.mui.
******************************************************************************/
#ifndef CFGLISTCLASS_H
#define CFGLISTCLASS_H

struct CfgListData
{
    BOOL cl_Dragging;
};

#define TAGBASE_CfgList (TAG_USER | 3242<<16)

AROS_UFP3(IPTR, CfgListDispatcher,
          AROS_UFPA(struct IClass *, cl, A0),
          AROS_UFPA(Object *, obj, A2),
          AROS_UFPA(Msg, msg, A1));

#endif 
