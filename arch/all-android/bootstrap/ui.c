#include <signal.h>
#include <stdarg.h>
#include <stdio.h>

#include "android.h"

#define D(x)

static char buf[1024];

void DisplayError(char *fmt, ...)
{
    va_list args;
    jstring str;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    str = (*jni)->NewStringUTF(jni, buf);
    (*jni)->CallVoidMethod(jni, obj, DisplayError_mid, str);
}

/* This function is linked in by exec.library and used for displaying alerts */
void DisplayAlert(char *text)
{
    struct sigaction sa;
    jstring str;

    D(fprintf(stderr, "[Bootstrap] DisplayAlert():\n%s\n", text));

    /*
     * Dalvik VM will enable signals, this will cause our task switcher to run.
     * Also it seems to reset the stack pointer. This causes task switcher
     * to re-issue AN_StackProbe guru. Nested alert screws up the VM.
     * Anyway we are not going to continue, so this is acceptable solution
     * here. However this means trouble with input - we can't call VM from
     * within AROS code. Well, have to invent something (separate input thread?)
     */
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = SA_RESTART;
    sa.sa_restorer = NULL;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGALRM, &sa, NULL);

    str = (*jni)->NewStringUTF(jni, text);
    (*jni)->CallVoidMethod(jni, obj, DisplayAlert_mid, str);

    D(fprintf(stderr, "DisplayAlert() method returned\n"));
}
