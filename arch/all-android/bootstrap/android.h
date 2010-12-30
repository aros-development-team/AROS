#include <jni.h>

/* These three variables are referenced from AROS drivers */
extern JNIEnv *Java_Env;
extern jclass  Java_Class;
extern jobject Java_Object;

extern jmethodID DisplayAlert_mid;
extern jmethodID DisplayError_mid;
