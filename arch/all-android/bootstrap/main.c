#include <sys/stat.h>

#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include <runtime.h>

#include "android.h"
#include "bootstrap.h"

/* Interface variables */
int DisplayPipe;
int InputPipe;

JNIEnv *Java_Env;
jclass  Java_Class;
jobject Java_Object;

jmethodID DisplayError_mid;

int Java_org_aros_bootstrap_AROSBootstrap_Start(JNIEnv* env, jobject this, jstring basedir, jobject readfd, jobject writefd)
{
    jboolean is_copy;
    const char *arospath;
    int res;

    Java_Env    = env;
    Java_Object = this;
    Java_Class  = (*env)->GetObjectClass(env, this);

    DisplayError_mid = (*env)->GetMethodID(env, Java_Class, "DisplayError", "(Ljava/lang/String;)V");

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

	if (res == 0)
	{
	    /* Initialize FileHandle objects by poking the "fd" field with the file descriptor */
	    jclass *class_fdesc = (*env)->GetObjectClass(env, readfd);
	    jfieldID field_fd = (*env)->GetFieldID(env, class_fdesc, "descriptor", "I");
	    
	    if (!field_fd)
	    {
	    	DisplayError("Failed to set up pipe descriptor objects");
	    	return -1;
	    }

	    (*env)->SetIntField(env, readfd, field_fd, DisplayPipe);
	    (*env)->SetIntField(env, writefd, field_fd, InputPipe);
	}
    }

    return res;
}
