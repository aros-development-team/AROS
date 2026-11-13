#ifndef SYSEXP_ENUMS_H
#define SYSEXP_ENUMS_H

#include <oop/oop.h>
#include <mui/NListtree_mcc.h>

typedef void (*CLASS_ENUMFUNC)(OOP_Object *obj, struct MUI_NListtree_TreeNode *parent);
typedef BOOL (*CLASS_VALIDFUNC)(OOP_Object *obj, ULONG *flags);

struct ObjectUserData
{
    OOP_Object *obj;
    struct MUI_CustomClass *winClass;
    Object *win;
};

struct InsertObjectMsg
{
    OOP_Object *obj;
    struct MUI_CustomClass *winClass;
};

struct ClassHandlerNode
{
    struct Node ch_Node; // ln_Name = classID;
    struct MUI_CustomClass *muiClass;
    CLASS_ENUMFUNC enumFunc;
    CLASS_VALIDFUNC validFunc;
};

struct SysexpHook_data
{
    struct SysexpBase *hd_sysexpbase; 
};

struct SysexpEnum_data
{
    struct SysexpBase *ed_sysexpbase; 
    struct List *ed_list;
};

#endif /*  SYSEXP_ENUMS_H */
