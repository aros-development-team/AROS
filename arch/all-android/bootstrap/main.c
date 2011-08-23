#include <android/bitmap.h>
#include <sys/stat.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <runtime.h>

#include "android.h"
#include "bootstrap.h"

#define D(x) x

/* These variables are used by DisplayError() */
JNIEnv   *Java_Env;
jobject   Java_Object;
jmethodID DisplayError_mid;
jmethodID HandleExit_mid;

/*
 * This is the main bootstrap entry point.
 * It parses the configuration file, prepares the environment and loads the kickstart.
 * In Android it returns after that, allowing Java code to execute Kick() method,
 * which actually runs the loaded kickstart.
 * Kick() can be executed multiple times, this is how warm reboot works.
 */
int Java_org_aros_bootstrap_AROSBootstrap_Load(JNIEnv* env, jobject this, jstring basedir)
{
    jclass Java_Class;
    jboolean is_copy;
    const char *arospath;
    int res;

    Java_Env    = env;
    Java_Object = this;
    Java_Class  = (*env)->GetObjectClass(env, this);

    DisplayError_mid = (*env)->GetMethodID(env, Java_Class, "DisplayError", "(Ljava/lang/String;)V");
    HandleExit_mid   = (*env)->GetMethodID(env, Java_Class, "HandleExit", "(I)V");

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

/*
 * Wrap a string at a given memory address into ByteBuffer object.
 * FIXME: It would be better to pass address as long here, however looks
 * like Android gcc has a bug and misaligns arguments in this case. I got
 * wrong values ('size' was actually 'addr' and 'addr' contained some weird garbage.
 * Since Android is a 32-bit system, i hope ints are ok for now.
 */

jobject Java_org_aros_bootstrap_AROSBootstrap_MapString(JNIEnv* env, jobject this, jint addr)
{
    jlong size = strlen((void *)addr);

    return (*env)->NewDirectByteBuffer(env, (void *)addr, size);
}

/*
 * Copy a given region from AROS displayable bitmap to Java bitmap object.
 * FIXME: See above why srcAddr is jint.
 */
jint Java_org_aros_bootstrap_AROSBootstrap_GetBitmap(JNIEnv* env, jobject this, jobject bitmap, jint srcAddr,
						     jint x, jint y, jint width, jint height, jint bytesPerLine)
{
    void *src = (void *)srcAddr;
    void *dest;
    AndroidBitmapInfo bmdata;
    int yc;
    jint rc;

    rc = AndroidBitmap_getInfo(env, bitmap, &bmdata);
    if (rc < 0)
    {
    	D(kprintf("[Bootstrap] Failed to obtain information about bitmap %p\n", bitmap));
    	return rc;
    }

    rc = AndroidBitmap_lockPixels(env, bitmap, &dest);
    if (rc < 0)
    {
    	D(kprintf("[Bootstrap] Failed to lock bitmap %p\n", bitmap));
    	return rc;
    }

    /* This is ARGB bitmap, 4 bytes per pixel */
    src  += y * bytesPerLine  + (x << 2);
    dest += y * bmdata.stride + (x << 2);
    width <<= 2;

    for (yc = 0; yc < height; yc++)
    {
    	memcpy(dest, src, width);
    	src  += bytesPerLine;
    	dest += bmdata.stride;
    }

    return AndroidBitmap_unlockPixels(env, bitmap);
}
