#include <unistd.h>
#include <signal.h>

int raise(int signal)
{
    kill(getpid(), signal);
}

