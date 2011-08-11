#include <sys/stat.h>

#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include <runtime.h>

#include "android.h"
#include "bootstrap.h"

/* Interface variables */
JNIEnv *Java_Env;
jclass  Java_Class;
jobject Java_Object;

jmethodID DisplayAlert_mid;
jmethodID DisplayError_mid;

int Java_org_aros_bootstrap_AROSBootstrap_Start(JNIEnv* env, jobject this, jstring basedir, jstring tmpdir)
{
    jboolean is_copy;
    const char *arospath = (*env)->GetStringUTFChars(env, basedir, &is_copy);
    const char *tmp_path = (*env)->GetStringUTFChars(env, tmpdir, &is_copy);
    int res;

    Java_Env    = env;
    Java_Object = this;
    Java_Class  = (*env)->GetObjectClass(env, this);

    DisplayAlert_mid = (*env)->GetMethodID(env, Java_Class, "DisplayAlert", "(Ljava/lang/String;)V");
    DisplayError_mid = (*env)->GetMethodID(env, Java_Class, "DisplayError", "(Ljava/lang/String;)V");

    res = chdir(tmp_path);
    (*env)->ReleaseStringUTFChars(env, basedir, tmp_path);
    if (res)
    {
	(*env)->ReleaseStringUTFChars(env, basedir, arospath);
    	DisplayError("Failed to locate temporary directory (%s): %s", tmp_path, strerror(errno));

    	return res;
    }
    
    res = mkfifo("AROS.display", 0500);
    /* This call may file because the pipe already exists, it's ok */
    if (res && (errno != EEXIST))
    {
	(*env)->ReleaseStringUTFChars(env, basedir, arospath);
    	DisplayError("Failed to create display pipe: %s", strerror(errno));

    	return res;
    }

    res = chdir(arospath);
    (*env)->ReleaseStringUTFChars(env, basedir, arospath);

    /* We can't pass any arguments (yet) */
    if (res)
	DisplayError("Failed to locate AROS root directory (%s): %s", arospath, strerror(errno));
    else
	res = bootstrap(0, NULL);

    return res;
}
