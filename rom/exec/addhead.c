#include <aros/libcall.h>
#include <exec/lists.h>
#include <exec/nodes.h>

__AROS_LH2I(void, AddHead,
  __AROS_LA(struct List *,list,A0),
  __AROS_LA(struct Node *,node,A1),
  struct ExecBase *,SysBase,40,Exec)
{
  __AROS_FUNC_INIT
  node->ln_Succ=list->lh_Head;
  node->ln_Pred=(struct Node *)list;
  list->lh_Head=node;
  node->ln_Succ->ln_Pred=node;
  __AROS_FUNC_EXIT
}
