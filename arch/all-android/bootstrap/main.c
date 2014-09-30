/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <android/bitmap.h>
#include <sys/stat.h>

#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <runtime.h>

#include "android.h"
#include "bootstrap.h"

#define D(x)

/* These variables are used by DisplayError() */
JNIEnv   *Java_Env;
jobject   Java_Object;
jmethodID DisplayError_mid;
jmethodID HandleExit_mid;

/* Dynamically loaded libjnigraphics.so functions */
void *libjnigraphics;
static int (*bm_GetInfo)(JNIEnv* env, jobject jbitmap, AndroidBitmapInfo* info);
static int (*bm_Lock)(JNIEnv* env, jobject jbitmap, void** addrPtr);
static int (*bm_Unlock)(JNIEnv* env, jobject jbitmap);

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
 * Wrap a given memory region into ByteBuffer object.
 * Needed for accessing AROS shared RAM.
 * FIXME: It would be better to pass in longs here, however looks
 * like Android gcc has a bug and misaligns arguments in this case. I got
 * wrong values ('size' was actually 'addr' and 'addr' contained some weird garbage.
 * Since Android is a 32-bit system, i hope ints are ok for now.
 */
jobject Java_org_aros_bootstrap_AROSBootstrap_MapMemory(JNIEnv* env, jobject this, jint addr, jint size)
{
    D(kprintf("[Bootstrap] Mapping %u bytes at %p\n", size, addr));

    return (*env)->NewDirectByteBuffer(env, (void *)addr, size);
}

/*
 * Retrieve a string at a given memory address as array of bytes.
 * FIXME: See above why srcAddr is jint.
 */

jbyteArray Java_org_aros_bootstrap_AROSBootstrap_GetString(JNIEnv* env, jobject this, jint addr)
{
    jsize size = strlen((void *)addr);
    jbyteArray ret = (*env)->NewByteArray(env, size);

    (*env)->SetByteArrayRegion(env, ret, 0, size - 1, (const jbyte *)addr);

    return ret;
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

    rc = bm_GetInfo(env, bitmap, &bmdata);
    if (rc < 0)
    {
    	D(kprintf("[Bootstrap] Failed to obtain information about bitmap %p\n", bitmap));
    	return rc;
    }

    rc = bm_Lock(env, bitmap, &dest);
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

    return bm_Unlock(env, bitmap);
}

jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
    libjnigraphics = dlopen("libjnigraphics.so", RTLD_NOW);
    D(kprintf("[Bootstrap] libjnigraphics.so handle: %p\n"));

    if (libjnigraphics)
    {
    	bm_GetInfo = dlsym(libjnigraphics, "AndroidBitmap_getInfo");
    	bm_Lock    = dlsym(libjnigraphics, "AndroidBitmap_lockPixels");
    	bm_Unlock  = dlsym(libjnigraphics, "AndroidBitmap_unlockPixels");
    }

    /* Dalvik wants a minimum of 1.4. Returning 1.1 causes exception. */
    return JNI_VERSION_1_4;
}

jboolean Java_org_aros_bootstrap_AROSBootstrap_HaveNativeGraphics(JNIEnv* env, jobject this)
{
    return libjnigraphics ? JNI_TRUE : JNI_FALSE;
}
