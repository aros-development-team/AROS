#include <exec/lists.h>
#include <exec/nodes.h>
#include <aros/libcall.h>

__AROS_LH2I(void, Enqueue,
    __AROS_LA(struct List *, list, A0),
    __AROS_LA(struct Node *, node, A1),
    struct ExecBase *, SysBase, 45, Exec)
{
  __AROS_FUNC_INIT
  struct Node *n;
  n=list->lh_Head;
  while(n->ln_Succ!=NULL)
  {
    if(n->ln_Pri<node->ln_Pri)
      break;
     n=n->ln_Succ;
  }
  node->ln_Succ=n;
  node->ln_Pred=n->ln_Pred;
  node->ln_Pred->ln_Succ=node;
  n->ln_Pred=node;
  __AROS_FUNC_EXIT
}

