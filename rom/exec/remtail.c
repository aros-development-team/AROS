#include <exec/lists.h>
#include <exec/nodes.h>
#include <aros/libcall.h>

__AROS_LH1I(struct Node *, RemTail,
    __AROS_LA(struct List *, list, A0),
    struct ExecBase *, SysBase, 44, Exec)
{
  __AROS_FUNC_INIT
  struct Node *node;
  node=list->lh_TailPred->ln_Pred;
  if(node!=NULL)
  {
    node->ln_Succ=(struct Node *)&list->lh_Tail;
    node=list->lh_TailPred;
    list->lh_TailPred=node->ln_Pred;
  }
  return node;
  __AROS_FUNC_EXIT
}

