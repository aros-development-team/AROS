#include <exec/lists.h>
#include <exec/nodes.h>
#include <aros/libcall.h>

__AROS_LH1I(struct Node *, RemHead,
    __AROS_LA(struct List *, list, A0),
    struct ExecBase *, SysBase, 43, Exec)
{
  __AROS_FUNC_INIT
  struct Node *node;
  node=list->lh_Head->ln_Succ;
  if(node!=NULL)
  {
    node->ln_Pred=(struct Node *)list;
    node=list->lh_Head;
    list->lh_Head=node->ln_Succ;
  }
  return node;
  __AROS_FUNC_EXIT
}

