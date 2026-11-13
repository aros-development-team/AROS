#include <jni.h>

/* The following is for DisplayError() function */
extern JNIEnv   *Java_Env;
extern jobject   Java_Object;
extern jmethodID DisplayError_mid;
extern jmethodID HandleExit_mid;
