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

    str = (*Java_Env)->NewStringUTF(Java_Env, buf);
    (*Java_Env)->CallVoidMethod(Java_Env, Java_Object, DisplayError_mid, str);
}

/* This function is linked in by exec.library and used for displaying alerts */
void DisplayAlert(char *text)
{
    jstring str;

    D(fprintf(stderr, "[Bootstrap] DisplayAlert():\n%s\n", text));

    str = (*Java_Env)->NewStringUTF(Java_Env, text);
    (*Java_Env)->CallVoidMethod(Java_Env, Java_Object, DisplayAlert_mid, str);

    D(fprintf(stderr, "DisplayAlert() method returned\n"));
}
