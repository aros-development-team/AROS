#pragma once
#include <exec/types.h>
struct MinNode { struct MinNode *mln_Succ, *mln_Pred; };
struct MinList { struct MinNode *mlh_Head, *mlh_Tail, *mlh_TailPred; };
struct Node { struct Node *ln_Succ, *ln_Pred; UBYTE ln_Type; signed char ln_Pri; char *ln_Name; };
struct List { struct Node *lh_Head, *lh_Tail, *lh_TailPred; UBYTE lh_Type; UBYTE l_pad; };
struct MsgPort { char pad[34]; };
struct Message { char pad[20]; };
struct IORequest { char pad[32]; };
