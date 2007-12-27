/*
    Copyright © 1995-2007, The AROS Development Team.
    $Id$

    Desc: Task exception test adapted from tasktest.
    Lang: english
*/

#include <proto/alib.h>
#include <proto/exec.h>
#include <exec/tasks.h>
#include <stdio.h>

int           cnt;
int           excbit;
int           sigbit1;
int           sigbit2;
struct Task * parent;

#define STACKSIZE 4096

static ULONG handler(
    ULONG             signals,
    APTR              data,
    struct ExecBase * SysBase)
{
    printf("exception handler called\n");
    /* this call of Signal will not call the handler
       again, because the signals handled are disabled
       during handler execution, we could even avoid
       excbit from being restored after handling with:
       signals ^= 1 << excbit */
    Signal(parent, 1 << sigbit1);
    return signals;
}

static void entry(void)
{
    sigbit2 = AllocSignal(-1);
    /* signal the parent task via a task exceptions */
    Signal(parent, 1 << excbit);
    if (sigbit2 >= 0)
    {
        int i;

        for (i = 0; i < 9; i++)
        {
            Wait(1 << sigbit2);
            cnt++;
        }
        for (i = 0; i < 10000; i++)
            cnt++;

        FreeSignal(sigbit2);
    }

    Wait(0); /* Let the parent remove me */
}

int main(int argc, char* argv[])
{
    parent = FindTask(NULL);

    excbit  = AllocSignal(-1);
    sigbit1 = AllocSignal(-1);
    if (excbit >= 0 && sigbit1 >= 0)
    {
        APTR oldexc           = parent->tc_ExceptCode;
        parent->tc_ExceptCode = &handler;
        /* call handler on excbit */
        SetExcept(1 << excbit, 1 << excbit);

        struct Task * t = CreateTask("new task", 1, &entry, STACKSIZE);
        if (t != NULL)
        {
            Wait(1 << sigbit1);
            /* we only want to get exceptioned once not on
               every new call to Signal, so disable excbit */
            SetExcept(0, 1 << excbit);
            if (sigbit2 >= 0)
            {
                int i;
                for (i = 0; i < 10; i++)
                {
                    Signal(t, 1 << sigbit2);
                    printf("%d\n", cnt);
                }
            }
            DeleteTask(t);
        }

        parent->tc_ExceptCode = oldexc;
        FreeSignal(excbit);
        FreeSignal(sigbit1);
    }
    return 0;
}
