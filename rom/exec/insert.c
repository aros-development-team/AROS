#include <aros/libcall.h>
#include <exec/lists.h>
#include <exec/nodes.h>

__AROS_LH3I(void, Insert,
    __AROS_LA(struct List *, list, A0),
    __AROS_LA(struct Node *, node, A1),
    __AROS_LA(struct Node *, pred, A2),
    struct ExecBase *, SysBase, 39, Exec)
{
  __AROS_FUNC_INIT
  if(pred==NULL)
    pred=(struct Node *)list;
  node->ln_Succ=pred->ln_Succ;
  node->ln_Succ->ln_Pred=node;
  node->ln_Pred=pred;
  pred->ln_Succ=node;
  __AROS_FUNC_EXIT
}

