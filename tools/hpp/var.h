#ifndef VAR_H
#define VAR_H

#ifndef TOOLLIB_TOOLLIB_H
#   include <toollib/toollib.h>
#endif
#ifndef TOOLLIB_VSTRING_H
#   include <toollib/vstring.h>
#endif
#ifndef TOOLLIB_CALLBACK_H
#   include <toollib/callback.h>
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
    CB	   cb;
    CBD    cbd;
}
Function;

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
extern String Var_Subst PARAMS ((const char * str));

extern void Func_Add PARAMS ((const char * name, CB cb, CBD cbd));
extern Function * Func_Find PARAMS ((const char * name));

#endif /* VAR_H */
