#include <jni.h>

/* These two variables are referenced from within AROS display driver */
int DisplayPipe;
int InputPipe;

/* The following is for DisplayError() function */
extern JNIEnv *Java_Env;
extern jclass  Java_Class;
extern jobject Java_Object;
extern jmethodID DisplayError_mid;
