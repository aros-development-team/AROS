
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

typedef void (*CLASS_ENUMFUNC)(OOP_Object *obj, struct MUI_NListtree_TreeNode *parent);
typedef BOOL (*CLASS_VALIDFUNC)(OOP_Object *obj, ULONG *flags);

struct ClassHandlerNode
{
    struct Node ch_Node; // ln_Name = classID;
    struct MUI_CustomClass **muiClass;
    CLASS_ENUMFUNC enumFunc;
    CLASS_VALIDFUNC validFunc;
};

extern Object *hidd_tree;

extern BOOL RegisterClassHandler(CONST_STRPTR, BYTE pri, struct MUI_CustomClass **, CLASS_ENUMFUNC enumfunc, CLASS_VALIDFUNC validfunc);
extern struct ClassHandlerNode *FindClassHandler(CONST_STRPTR, struct List *);
extern struct ClassHandlerNode *FindObjectHandler(OOP_Object *, struct List *);

extern void hwEnum(OOP_Object *obj, struct MUI_NListtree_TreeNode *tn);

