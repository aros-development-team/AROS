#ifndef VAR_H
#define VAR_H

#ifndef _TOOLLIB_H
#   include <toollib.h>
#endif

typedef struct
{
    Node   node;
    char * value;
}
Var;

typedef struct
{
    Node   node;
    int    type;
    void * value;
}
TVar;

typedef struct
{
    Node node;
    List vars;
}
VarLevel;

extern void   Var_Init PARAMS ((void));
extern void   Var_Exit PARAMS ((void));
extern void   Var_Set  PARAMS ((const char * name, const char * value));
extern char * Var_Get  PARAMS ((const char * name));
extern void   Var_UnSet PARAMS ((const char * name));
extern Var  * Var_Find PARAMS ((const char * name));
extern void   Var_PushLevel PARAMS ((void));
extern VarLevel * Var_PopLevel PARAMS ((void));
extern void Var_FreeLevel PARAMS ((VarLevel *));
extern void Var_Free PARAMS ((Var *));

#endif /* VAR_H */
