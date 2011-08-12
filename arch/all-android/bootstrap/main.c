#include <sys/stat.h>

#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include <runtime.h>

#include "android.h"
#include "bootstrap.h"

/* Interface variables */
const char *DisplayPipe;

JNIEnv *Java_Env;
jclass  Java_Class;
jobject Java_Object;

jmethodID DisplayAlert_mid;
jmethodID DisplayError_mid;

int Java_org_aros_bootstrap_AROSBootstrap_Start(JNIEnv* env, jobject this, jstring basedir, jstring pipe)
{
    jboolean is_copy;
    const char *arospath;
    int res;

    Java_Env    = env;
    Java_Object = this;
    Java_Class  = (*env)->GetObjectClass(env, this);

    DisplayAlert_mid = (*env)->GetMethodID(env, Java_Class, "DisplayAlert", "(Ljava/lang/String;)V");
    DisplayError_mid = (*env)->GetMethodID(env, Java_Class, "DisplayError", "(Ljava/lang/String;)V");

    DisplayPipe = (*env)->GetStringUTFChars(env, pipe, &is_copy);

    /* This call may file because the pipe already exists, it's ok */
    res = mkfifo(DisplayPipe, 0500);
    if (res && (errno != EEXIST))
    {
    	DisplayError("Failed to create display pipe (%s): %s", DisplayPipe, strerror(errno));
    	(*env)->ReleaseStringUTFChars(env, pipe, DisplayPipe);
    	return res;
    }

    /* We don't release DisplayPipe here because we need it */

    arospath = (*env)->GetStringUTFChars(env, basedir, &is_copy);
    res = chdir(arospath);

    if (res)
    {
	DisplayError("Failed to locate AROS root directory (%s): %s", arospath, strerror(errno));
	(*env)->ReleaseStringUTFChars(env, basedir, arospath);
    }
    else
    {
	(*env)->ReleaseStringUTFChars(env, basedir, arospath);
	res = bootstrap(0, NULL); /* We can't pass any arguments (yet) */
    }

    return res;
}
