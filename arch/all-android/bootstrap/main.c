#include <android/bitmap.h>
#include <sys/stat.h>

#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include <runtime.h>

#include "android.h"
#include "bootstrap.h"

#define D(x) x

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
    D(kprintf("[Bootstrap] Mapping %lu bytes at %p\n", size, addr));

    return (*env)->NewDirectByteBuffer(env, (void *)addr, size);
}

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
