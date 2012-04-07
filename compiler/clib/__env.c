/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Internal functions for environment variables handling.
*/

#include "__arosc_privdata.h"

#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <dos/var.h>

#include "__env.h"

#if AROS_AMIGAOS_COMPLIANCE && (AROS_AMIGAOS_COMPLIANCE >= 30)
/* ScanVars is a Morphos addition in dos.library v50, so we
 * have this stub here for arosc.library compatability with
 * dos.library v30..v39.
 */
static LONG v30_ScanVars(struct Hook *hook, ULONG flags, APTR userdata)
{
    /* We scan through the process->pr_LocalVars list */
    struct Process  *pr;
    struct LocalVar *var;
    struct ScanVarsMsg msg;
    LONG res = 0;
    APTR UtilityBase;
    
    UtilityBase = OpenLibrary("utility.library", 0);
    if (UtilityBase == NULL)
        return 0;

    msg.sv_SVMSize = sizeof(struct ScanVarsMsg);
    msg.sv_Flags = flags;
    pr  = (struct Process *)FindTask(NULL);

    /* We know that the callers of this routine
     * will be asking for GVF_LOCAL_ONLY
     */
    var = (struct LocalVar *)pr->pr_LocalVars.mlh_Head;
    
    ForeachNode(&pr->pr_LocalVars, var)
    {
        if (var->lv_Node.ln_Type == LV_VAR)
        {
            msg.sv_Name = var->lv_Node.ln_Name;
            msg.sv_Var = var->lv_Value;
            msg.sv_VarLen = var->lv_Len;
            msg.sv_GDir = "";
            res = CallHookPkt(hook, userdata, &msg);
            if(res != 0) 
                break;
        }
    }

    CloseLibrary(UtilityBase);
    return res;
}
#undef ScanVars
#define ScanVars(a,b,c) v30_ScanVars(a,b,c)
#endif

static __env_item *__env_newvar(const char *name, int valuesize)
{
    __env_item *item;

    item = malloc(sizeof(__env_item));
    if (!item) goto err1;

    item->name = strdup(name);
    if (!item->name) goto err2;

    item->value = malloc(valuesize);
    if (!item->value) goto err3;

    item->next = NULL;

    return item;

err3:
    free(item->name);
err2:
    free(item);
err1:
    return NULL;
}

static __env_item **internal_findvar(register const char *name)
{
    struct aroscbase *aroscbase = __GM_GetBase();

   __env_item **curr;

   for (
       curr = &aroscbase->acb_env_list;
       *curr && strcmp((*curr)->name, name);
       curr = &((*curr)->next)
   );

  return curr;
}

/*
  Allocates space for a variable with name 'name' returning a pointer to it.
  If a variable with this name already exists then returns a pointer to that
  variable.

  Returns NULL on error.
*/
__env_item *__env_getvar(const char *name, int valuesize)
{
    register __env_item **curr;

    curr = internal_findvar(name);

    if (*curr)
    {
        if (strlen((*curr)->value) < valuesize)
        {
	    free((*curr)->value);
	    (*curr)->value = malloc(valuesize);

	    if (!(*curr)->value)
	    {
	        __env_item *tmp = (*curr)->next;

	        free(*curr);
	        *curr = tmp;
   	    }
	}
    }
    else 
    {
	*curr = __env_newvar(name, valuesize);
    }

    return *curr;
}

void __env_delvar(const char *name)
{
    register __env_item **curr;

    curr = internal_findvar(name);

    if (*curr)
    {
	register __env_item *tmp = *curr;

	*curr = (*curr)->next;

	free(tmp->name);
	free(tmp->value);
	free(tmp);
    }
}

struct EnvData
{
   int envcount;
   int varbufsize;
   char * varbufptr;
};

/* Hook function counting the number of variables and computing the buffer size
   needed for name=value character buffer. */
LONG get_var_len(struct Hook *hook, APTR userdata, struct ScanVarsMsg *message)
{
    struct EnvData *data = (struct EnvData *) userdata;
    data->envcount++;
    data->varbufsize += 
	(ptrdiff_t) strlen((char*)message->sv_Name) + 
        message->sv_VarLen + 2;
    return 0;
}

/* Hook function storing name=value strings in buffer */
LONG get_var(struct Hook *hook, APTR userdata, struct ScanVarsMsg *message)
{
    struct EnvData *data = userdata;
    data->varbufptr[0] = '\0';
    strcat(data->varbufptr, (char*) message->sv_Name);
    strcat(data->varbufptr, "=");
    strncat(data->varbufptr, (char*) message->sv_Var, message->sv_VarLen);
    data->varbufptr = data->varbufptr + strlen(data->varbufptr) + 1;
    return 0;
}

/* Function storing name=value strings in given environ buffer. When buffer is
   null, it returns minimal necessary size of the buffer needed to store all
   name=value strings. */
int __env_get_environ(char **environ, int size)
{
    int i;
    struct Hook hook;
    struct EnvData u; 
    char *varbuf;

    u.envcount = 0;
    u.varbufsize = 0;

    memset(&hook, 0, sizeof(struct Hook));
    hook.h_Entry = (HOOKFUNC) get_var_len;
    ScanVars(&hook, GVF_LOCAL_ONLY, &u);

    if(environ == NULL)
	return sizeof(char*) * (u.envcount + 1) + u.varbufsize;
    else if(size < sizeof(char*) *(u.envcount + 1) + u.varbufsize)
	return -1;

    /* store name=value strings after table of pointers */
    varbuf = (char*) (environ + (u.envcount + 1));
	    
    /* time to fill in the buffers */
    u.varbufptr = varbuf;
    u.varbufptr[0] = '\0';   
    hook.h_Entry = (HOOKFUNC) get_var;
    ScanVars(&hook, GVF_LOCAL_ONLY, &u);

    for(i = 0; i < u.envcount; i++)
    {
        environ[i] = varbuf;
        varbuf = strchr(varbuf, '\0') + 1;
    }
    environ[u.envcount] = NULL;

    return 0;	    
}
