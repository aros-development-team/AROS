/*
    Copyright  1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Common data structures of androidgfx.hidd
    Lang: English.
*/

#include <jni.h>

struct agfx_staticdata
{
    APTR HostLibBase;

    OOP_AttrBase *AttrBases;

    OOP_Class  *gfxclass;
    OOP_Class  *bmclass;
    OOP_Class  *mouseclass;
    OOP_Class  *kbdclass;

    OOP_Object *mousehidd;
    OOP_Object *kbdhidd;
    
    JNIEnv     *jni;
    jobject	jobj;

    jmethodID	GetDisplay_mID;
    jfieldID	Width_aID;
    jfieldID	Height_aID;
};

struct AGFXBase
{
    struct Library library;
    struct agfx_staticdata xsd;
    APTR HostLibHandle;
};

#define XSD(cl) (&((struct AGFXBase *)cl->UserData)->xsd)

#undef HiddBitMapAttrBase
#undef HiddSyncAttrBase
#undef HiddPixFmtAttrBase
#undef HiddGfxAttrBase
#undef HiddAttrBase
#define HiddBitMapAttrBase XSD(cl)->AttrBases[0]
#define HiddSyncAttrBase   XSD(cl)->AttrBases[1]
#define HiddPixFmtAttrBase XSD(cl)->AttrBases[2]
#define HiddGfxAttrBase	   XSD(cl)->AttrBases[3]
#define HiddAttrBase	   XSD(cl)->AttrBases[4]

#define HostLibBase XSD(cl)->HostLibBase

#define JNI_GetIntField(obj, id)		(*XSD(cl)->jni)->GetIntField(XSD(cl)->jni, obj, id)
#define JNI_CallObjectMethod(obj, id...)	(*XSD(cl)->jni)->CallObjectMethod(XSD(cl)->jni, obj, id)
#define JNI_FindClass(name)			(*XSD(cl)->jni)->FindClass(XSD(cl)->jni, name)
#define JNI_GetMethodID(cl, name, sig)		(*XSD(cl)->jni)->GetMethodID(XSD(cl)->jni, cl, name, sig)
#define JNI_GetFieldID(cl, name, sig)		(*XSD(cl)->jni)->GetFieldID(XSD(cl)->jni, cl, name, sig)
