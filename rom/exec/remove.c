#include <aros/libcall.h>
#include <exec/nodes.h>

__AROS_LH1I(void, Remove,
    __AROS_LA(struct Node *, node, A1),
    struct ExecBase *, SysBase, 42, Exec)
{
  __AROS_FUNC_INIT
  node->ln_Succ->ln_Pred=node->ln_Pred;
  node->ln_Pred->ln_Succ=node->ln_Succ;
  __AROS_FUNC_EXIT
}

