#include <exec/lists.h>
#include <exec/nodes.h>
#include <aros/libcall.h>

__AROS_LH2I(void,AddTail,
  __AROS_LA(struct List *,list,A0),
  __AROS_LA(struct Node *,node,A1),
  struct ExecBase *,SysBase,41,Exec)
{
  __AROS_FUNC_INIT
  node->ln_Pred=list->lh_TailPred;
  node->ln_Succ=(struct Node *)&list->lh_Tail;
  list->lh_TailPred=node;
  node->ln_Pred->ln_Succ=node;
  __AROS_FUNC_EXIT
}

