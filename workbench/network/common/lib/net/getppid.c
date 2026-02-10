#define AROS_MORPHOS_COMPATIBLE

#include <exec/tasks.h>
#include <sys/types.h>
#include <proto/exec.h>

pid_t
getppid(void)
{
  struct Task *ThisTask;

  ThisTask = FindTask(NULL);
  return (pid_t)ThisTask->tc_ETask->Parent;
}

