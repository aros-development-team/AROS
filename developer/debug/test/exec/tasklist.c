#include <resources/task.h>
#include <proto/task.h>
#include <stdio.h>

/*
    Test for NextTaskEntry to show this bug
    https://sourceforge.net/p/aros/bugs/552/
*/

int main(void)
{
    struct Task *task;
    struct TaskList *tasklist = LockTaskList(LTF_ALL);
    while ((task =  NextTaskEntry(tasklist, LTF_ALL)))
    {
        printf("Task %p Name %s State %d\n", task, task->tc_Node.ln_Name, task->tc_State);
    }
    UnLockTaskList(tasklist, LTF_ALL);

    return 0;
}
