#include <aros/libcall.h>
#include <exec/lists.h>
#include <exec/nodes.h>

__AROS_LH2I(struct Node *,FindName,
  __AROS_LA(struct List *,list,A0),
  __AROS_LA(STRPTR,name,A1),
  struct ExecBase *,SysBase,46,Exec)
{
  __AROS_FUNC_INIT
  struct Node *node;
  node=list->lh_Head;
  while(node->ln_Succ!=NULL)
  {
    char *s1=node->ln_Name;
    char *s2=name;
    while(*s1++==*s2)
      if(!*s2++)
	return node;
    node=node->ln_Succ;
  }
  return NULL;
  __AROS_FUNC_EXIT
}

