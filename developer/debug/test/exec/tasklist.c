
#include <proto/exec.h>
#include <proto/task.h>

#include <resources/task.h>
#include <stdio.h>

/*
    Test for NextTaskEntry to show this bug
    https://sourceforge.net/p/aros/bugs/552/
*/

int main(void)
{
    APTR TaskResBase;
    struct Task *task;
    struct TaskList *tasklist;

    if ((TaskResBase = OpenResource("task.resource")) != NULL)
    {
        tasklist = LockTaskList(LTF_ALL);
        while ((task =  NextTaskEntry(tasklist, LTF_ALL)))
        {
            printf("Task %p Name %s State %d\n", task, task->tc_Node.ln_Name, task->tc_State);
        }
        UnLockTaskList(tasklist, LTF_ALL);
    }
    return 0;
}
