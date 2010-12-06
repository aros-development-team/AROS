#include <stdarg.h>
#include <stdio.h>

#include "android.h"

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
    jstring str = (*jni)->NewStringUTF(jni, buf);

    (*jni)->CallVoidMethod(jni, obj, DisplayAlert_mid, str);
}
