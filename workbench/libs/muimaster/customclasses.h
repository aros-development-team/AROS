#ifndef ZUNE_CUSTOMCLASSES_H
#define ZUNE_CUSTOMCLASSES_H

#include <proto/muimaster.h>
#include <dos/dos.h>
#include <aros/symbolsets.h>
#include <aros/autoinit.h>
#include <aros/debug.h>

#define __ZUNE_CUSTOMCLASS_START(name)                                \
BOOPSI_DISPATCHER(IPTR, name ## _Dispatcher, __class, __self, __msg); \
BOOPSI_DISPATCHER(IPTR, name ## _Dispatcher, __class, __self, __msg)  \
{                                                                     \
    switch (__msg->MethodID)                                          \
    {                                                                 \

#define __ZUNE_CUSTOMCLASS_END(name, base, parent_name, parent_class) \
        default:                                                      \
            return DoSuperMethodA(__class, __self, __msg);            \
    }                                                                 \
                                                                      \
    return (IPTR) NULL;                                               \
}                                                                     \
                                                                      \
struct MUI_CustomClass * name ## _CLASS;                              \
                                                                      \
int name ## _Initialize(void);                                        \
int name ## _Initialize(void)                                         \
{                                                                     \
    name ## _CLASS = MUI_CreateCustomClass                            \
    (                                                                 \
        base, parent_name, parent_class,                              \
        sizeof(struct name ## _DATA), name ## _Dispatcher             \
    );                                                                \
                                                                      \
    if (!name ## _CLASS)                                              \
    {                                                                 \
        __showerror                                                   \
	(                                                             \
	    "Could not create Zune custom class `" #name "'."         \
	);                                                            \
                                                                      \
	return 0;                                                     \
    }                                                                 \
                                                                      \
    return 1;                                                         \
}                                                                     \
                                                                      \
void name ## _Deinitialize(void);                                     \
void name ## _Deinitialize(void)                                      \
{                                                                     \
    MUI_DeleteCustomClass(name ## _CLASS);                            \
}                                                                     \
                                                                      \
ADD2INIT(name ## _Initialize,   100);                                 \
ADD2EXIT(name ## _Deinitialize, 100);                                 \

#define __ZUNE_CUSTOMCLASS_METHOD(mname, mid, m_msg_type)        \
    case mid:                                                    \
                                                                 \
        D(bug("[ZCC] ENTERING "__AROS_STR(mname)                 \
          "("__AROS_STR(m_msg_type)")\n"));                      \
                                                                 \
        return (IPTR) mname(__class, __self, (m_msg_type)__msg)  \

/******************************************************************/

#define ZUNE_CUSTOMCLASS_1(name, base, parent_name, parent_class,  \
                           m1, m1_msg_type)                        \
    __ZUNE_CUSTOMCLASS_START(name)                                 \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m1, m1, m1_msg_type);  \
    __ZUNE_CUSTOMCLASS_END(name, base, parent_name, parent_class)  \

#define ZUNE_CUSTOMCLASS_2(name, base, parent_name, parent_class,  \
                           m1, m1_msg_type,                        \
                           m2, m2_msg_type)                        \
    __ZUNE_CUSTOMCLASS_START(name)                                 \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m1, m1, m1_msg_type);  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m2, m2, m2_msg_type);  \
    __ZUNE_CUSTOMCLASS_END(name, base, parent_name, parent_class)  \

#define ZUNE_CUSTOMCLASS_3(name, base, parent_name, parent_class,  \
                           m1, m1_msg_type,                        \
                           m2, m2_msg_type,                        \
                           m3, m3_msg_type)                        \
    __ZUNE_CUSTOMCLASS_START(name)                                 \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m1, m1, m1_msg_type);  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m2, m2, m2_msg_type);  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m3, m3, m3_msg_type);  \
    __ZUNE_CUSTOMCLASS_END(name, base, parent_name, parent_class)  \

#define ZUNE_CUSTOMCLASS_4(name, base, parent_name, parent_class,  \
                           m1, m1_msg_type,                        \
                           m2, m2_msg_type,                        \
                           m3, m3_msg_type,                        \
                           m4, m4_msg_type)                        \
    __ZUNE_CUSTOMCLASS_START(name)                                 \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m1, m1, m1_msg_type);  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m2, m2, m2_msg_type);  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m3, m3, m3_msg_type);  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m4, m4, m4_msg_type);  \
    __ZUNE_CUSTOMCLASS_END(name, base, parent_name, parent_class)  \

#define ZUNE_CUSTOMCLASS_5(name, base, parent_name, parent_class,  \
                           m1, m1_msg_type,                        \
                           m2, m2_msg_type,                        \
                           m3, m3_msg_type,                        \
                           m4, m4_msg_type,                        \
                           m5, m5_msg_type)                        \
    __ZUNE_CUSTOMCLASS_START(name)                                 \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m1, m1, m1_msg_type);  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m2, m2, m2_msg_type);  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m3, m3, m3_msg_type);  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m4, m4, m4_msg_type);  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m5, m5, m5_msg_type);  \
    __ZUNE_CUSTOMCLASS_END(name, base, parent_name, parent_class)  \

#define ZUNE_CUSTOMCLASS_6(name, base, parent_name, parent_class,  \
                           m1, m1_msg_type,                        \
                           m2, m2_msg_type,                        \
                           m3, m3_msg_type,                        \
                           m4, m4_msg_type,                        \
                           m5, m5_msg_type,                        \
                           m6, m6_msg_type)                        \
    __ZUNE_CUSTOMCLASS_START(name)                                 \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m1, m1, m1_msg_type);  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m2, m2, m2_msg_type);  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m3, m3, m3_msg_type);  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m4, m4, m4_msg_type);  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m5, m5, m5_msg_type);  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m6, m6, m6_msg_type);  \
    __ZUNE_CUSTOMCLASS_END(name, base, parent_name, parent_class)  \

#define ZUNE_CUSTOMCLASS_7(name, base, parent_name, parent_class,  \
                           m1, m1_msg_type,                        \
                           m2, m2_msg_type,                        \
                           m3, m3_msg_type,                        \
                           m4, m4_msg_type,                        \
                           m5, m5_msg_type,                        \
                           m6, m6_msg_type,                        \
                           m7, m7_msg_type)                        \
    __ZUNE_CUSTOMCLASS_START(name)                                 \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m1, m1, m1_msg_type);  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m2, m2, m2_msg_type);  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m3, m3, m3_msg_type);  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m4, m4, m4_msg_type);  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m5, m5, m5_msg_type);  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m6, m6, m6_msg_type);  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m7, m7, m7_msg_type);  \
    __ZUNE_CUSTOMCLASS_END(name, base, parent_name, parent_class)  \

#define ZUNE_CUSTOMCLASS_8(name, base, parent_name, parent_class,  \
                           m1, m1_msg_type,                        \
                           m2, m2_msg_type,                        \
                           m3, m3_msg_type,                        \
                           m4, m4_msg_type,                        \
                           m5, m5_msg_type,                        \
                           m6, m6_msg_type,                        \
                           m7, m7_msg_type,                        \
                           m8, m8_msg_type)                        \
    __ZUNE_CUSTOMCLASS_START(name)                                 \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m1, m1, m1_msg_type);  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m2, m2, m2_msg_type);  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m3, m3, m3_msg_type);  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m4, m4, m4_msg_type);  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m5, m5, m5_msg_type);  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m6, m6, m6_msg_type);  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m7, m7, m7_msg_type);  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m8, m8, m8_msg_type);  \
    __ZUNE_CUSTOMCLASS_END(name, base, parent_name, parent_class)  \

#define ZUNE_CUSTOMCLASS_9(name, base, parent_name, parent_class,  \
                           m1, m1_msg_type,                        \
                           m2, m2_msg_type,                        \
                           m3, m3_msg_type,                        \
                           m4, m4_msg_type,                        \
                           m5, m5_msg_type,                        \
                           m6, m6_msg_type,                        \
                           m7, m7_msg_type,                        \
                           m8, m8_msg_type,                        \
                           m9, m9_msg_type)                        \
    __ZUNE_CUSTOMCLASS_START(name)                                 \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m1, m1, m1_msg_type);  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m2, m2, m2_msg_type);  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m3, m3, m3_msg_type);  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m4, m4, m4_msg_type);  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m5, m5, m5_msg_type);  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m6, m6, m6_msg_type);  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m7, m7, m7_msg_type);  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m8, m8, m8_msg_type);  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m9, m9, m9_msg_type);  \
    __ZUNE_CUSTOMCLASS_END(name, base, parent_name, parent_class)  \

/*************************************************************************/

#define __ZUNE_CUSTOMCLASS_INLINEMETHOD(cname, mname, m_msg_type, m_code) \
IPTR mname(Class *CLASS, Object *self, m_msg_type message);               \
IPTR mname(Class *CLASS, Object *self, m_msg_type message)                \
{                                                                         \
    struct cname ## _DATA *data __unused = INST_DATA(CLASS, self);        \
                                                                          \
    (void)m_code;                                                         \
}

#define __ZUNE_CUSTOMCLASS_INSTDATA(cname, inst_data) \
    struct cname ## _DATA inst_data

#define ZUNE_CUSTOMCLASS_INLINE_1(name, base, parent_name, parent_class,           \
                                  inst_data,                                       \
                                  m1, m1_msg_type, m1_code)                        \
    __ZUNE_CUSTOMCLASS_INSTDATA(name, inst_data);                                  \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m1, m1_msg_type, m1_code); \
    __ZUNE_CUSTOMCLASS_START(name)                                                 \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m1, m1, m1_msg_type);                  \
    __ZUNE_CUSTOMCLASS_END(name, base, parent_name, parent_class)

#define ZUNE_CUSTOMCLASS_INLINE_2(name, base, parent_name, parent_class,           \
                                  inst_data,                                       \
                                  m1, m1_msg_type, m1_code,                        \
                                  m2, m2_msg_type, m2_code)                        \
    __ZUNE_CUSTOMCLASS_INSTDATA(name, inst_data);                                  \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m1, m1_msg_type, m1_code); \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m2, m2_msg_type, m2_code); \
    __ZUNE_CUSTOMCLASS_START(name)                                                 \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m1, m1, m1_msg_type);                  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m2, m2, m2_msg_type);                  \
    __ZUNE_CUSTOMCLASS_END(name, base, parent_name, parent_class)

#define ZUNE_CUSTOMCLASS_INLINE_3(name, base, parent_name, parent_class,           \
                                  inst_data,                                       \
                                  m1, m1_msg_type, m1_code,                        \
                                  m2, m2_msg_type, m2_code,                        \
                                  m3, m3_msg_type, m3_code)                        \
    __ZUNE_CUSTOMCLASS_INSTDATA(name, inst_data);                                  \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m1, m1_msg_type, m1_code); \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m2, m2_msg_type, m2_code); \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m3, m3_msg_type, m3_code); \
    __ZUNE_CUSTOMCLASS_START(name)                                                 \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m1, m1, m1_msg_type);                  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m2, m2, m2_msg_type);                  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m3, m3, m3_msg_type);                  \
    __ZUNE_CUSTOMCLASS_END(name, base, parent_name, parent_class)

#define ZUNE_CUSTOMCLASS_INLINE_4(name, base, parent_name, parent_class,           \
                                  inst_data,                                       \
                                  m1, m1_msg_type, m1_code,                        \
                                  m2, m2_msg_type, m2_code,                        \
                                  m3, m3_msg_type, m3_code,                        \
                                  m4, m4_msg_type, m4_code)                        \
    __ZUNE_CUSTOMCLASS_INSTDATA(name, inst_data);                                  \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m1, m1_msg_type, m1_code); \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m2, m2_msg_type, m2_code); \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m3, m3_msg_type, m3_code); \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m4, m4_msg_type, m4_code); \
    __ZUNE_CUSTOMCLASS_START(name)                                                 \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m1, m1, m1_msg_type);                  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m2, m2, m2_msg_type);                  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m3, m3, m3_msg_type);                  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m4, m4, m4_msg_type);                  \
    __ZUNE_CUSTOMCLASS_END(name, base, parent_name, parent_class)

#define ZUNE_CUSTOMCLASS_INLINE_5(name, base, parent_name, parent_class,           \
                                  inst_data,                                       \
                                  m1, m1_msg_type, m1_code,                        \
                                  m2, m2_msg_type, m2_code,                        \
                                  m3, m3_msg_type, m3_code,                        \
                                  m4, m4_msg_type, m4_code,                        \
                                  m5, m5_msg_type, m5_code)                        \
    __ZUNE_CUSTOMCLASS_INSTDATA(name, inst_data);                                  \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m1, m1_msg_type, m1_code); \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m2, m2_msg_type, m2_code); \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m3, m3_msg_type, m3_code); \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m4, m4_msg_type, m4_code); \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m5, m5_msg_type, m5_code); \
    __ZUNE_CUSTOMCLASS_START(name)                                                 \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m1, m1, m1_msg_type);                  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m2, m2, m2_msg_type);                  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m3, m3, m3_msg_type);                  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m4, m4, m4_msg_type);                  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m5, m5, m5_msg_type);                  \
    __ZUNE_CUSTOMCLASS_END(name, base, parent_name, parent_class)

#define ZUNE_CUSTOMCLASS_INLINE_6(name, base, parent_name, parent_class,           \
                                  inst_data,                                       \
                                  m1, m1_msg_type, m1_code,                        \
                                  m2, m2_msg_type, m2_code,                        \
                                  m3, m3_msg_type, m3_code,                        \
                                  m4, m4_msg_type, m4_code,                        \
                                  m5, m5_msg_type, m5_code,                        \
                                  m6, m6_msg_type, m6_code)                        \
    __ZUNE_CUSTOMCLASS_INSTDATA(name, inst_data);                                  \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m1, m1_msg_type, m1_code); \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m2, m2_msg_type, m2_code); \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m3, m3_msg_type, m3_code); \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m4, m4_msg_type, m4_code); \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m5, m5_msg_type, m5_code); \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m6, m6_msg_type, m6_code); \
    __ZUNE_CUSTOMCLASS_START(name)                                                 \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m1, m1, m1_msg_type);                  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m2, m2, m2_msg_type);                  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m3, m3, m3_msg_type);                  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m4, m4, m4_msg_type);                  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m5, m5, m5_msg_type);                  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m6, m6, m6_msg_type);                  \
    __ZUNE_CUSTOMCLASS_END(name, base, parent_name, parent_class)

#define ZUNE_CUSTOMCLASS_INLINE_7(name, base, parent_name, parent_class,           \
                                  inst_data,                                       \
                                  m1, m1_msg_type, m1_code,                        \
                                  m2, m2_msg_type, m2_code,                        \
                                  m3, m3_msg_type, m3_code,                        \
                                  m4, m4_msg_type, m4_code,                        \
                                  m5, m5_msg_type, m5_code,                        \
                                  m6, m6_msg_type, m6_code,                        \
                                  m7, m7_msg_type, m7_code)                        \
    __ZUNE_CUSTOMCLASS_INSTDATA(name, inst_data);                                  \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m1, m1_msg_type, m1_code); \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m2, m2_msg_type, m2_code); \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m3, m3_msg_type, m3_code); \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m4, m4_msg_type, m4_code); \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m5, m5_msg_type, m5_code); \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m6, m6_msg_type, m6_code); \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m7, m7_msg_type, m7_code); \
    __ZUNE_CUSTOMCLASS_START(name)                                                 \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m1, m1, m1_msg_type);                  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m2, m2, m2_msg_type);                  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m3, m3, m3_msg_type);                  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m4, m4, m4_msg_type);                  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m5, m5, m5_msg_type);                  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m6, m6, m6_msg_type);                  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m7, m7, m7_msg_type);                  \
    __ZUNE_CUSTOMCLASS_END(name, base, parent_name, parent_class)

#define ZUNE_CUSTOMCLASS_INLINE_8(name, base, parent_name, parent_class,           \
                                  inst_data,                                       \
                                  m1, m1_msg_type, m1_code,                        \
                                  m2, m2_msg_type, m2_code,                        \
                                  m3, m3_msg_type, m3_code,                        \
                                  m4, m4_msg_type, m4_code,                        \
                                  m5, m5_msg_type, m5_code,                        \
                                  m6, m6_msg_type, m6_code,                        \
                                  m7, m7_msg_type, m7_code,                        \
                                  m8, m8_msg_type, m8_code)                        \
    __ZUNE_CUSTOMCLASS_INSTDATA(name, inst_data);                                  \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m1, m1_msg_type, m1_code); \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m2, m2_msg_type, m2_code); \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m3, m3_msg_type, m3_code); \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m4, m4_msg_type, m4_code); \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m5, m5_msg_type, m5_code); \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m6, m6_msg_type, m6_code); \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m7, m7_msg_type, m7_code); \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m8, m8_msg_type, m8_code); \
    __ZUNE_CUSTOMCLASS_START(name)                                                 \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m1, m1, m1_msg_type);                  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m2, m2, m2_msg_type);                  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m3, m3, m3_msg_type);                  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m4, m4, m4_msg_type);                  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m5, m5, m5_msg_type);                  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m6, m6, m6_msg_type);                  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m7, m7, m7_msg_type);                  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m8, m8, m8_msg_type);                  \
    __ZUNE_CUSTOMCLASS_END(name, base, parent_name, parent_class)

#define ZUNE_CUSTOMCLASS_INLINE_9(name, base, parent_name, parent_class,           \
                                  inst_data,                                       \
                                  m1, m1_msg_type, m1_code,                        \
                                  m2, m2_msg_type, m2_code,                        \
                                  m3, m3_msg_type, m3_code,                        \
                                  m4, m4_msg_type, m4_code,                        \
                                  m5, m5_msg_type, m5_code,                        \
                                  m6, m6_msg_type, m6_code,                        \
                                  m7, m7_msg_type, m7_code,                        \
                                  m8, m8_msg_type, m8_code,                        \
                                  m9, m9_msg_type, m9_code)                        \
    __ZUNE_CUSTOMCLASS_INSTDATA(name, inst_data);                                  \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m1, m1_msg_type, m1_code); \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m2, m2_msg_type, m2_code); \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m3, m3_msg_type, m3_code); \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m4, m4_msg_type, m4_code); \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m5, m5_msg_type, m5_code); \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m6, m6_msg_type, m6_code); \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m7, m7_msg_type, m7_code); \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m8, m8_msg_type, m8_code); \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m9, m9_msg_type, m9_code); \
    __ZUNE_CUSTOMCLASS_START(name)                                                 \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m1, m1, m1_msg_type);                  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m2, m2, m2_msg_type);                  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m3, m3, m3_msg_type);                  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m4, m4, m4_msg_type);                  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m5, m5, m5_msg_type);                  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m6, m6, m6_msg_type);                  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m7, m7, m7_msg_type);                  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m8, m8, m8_msg_type);                  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m9, m9, m9_msg_type);                  \
    __ZUNE_CUSTOMCLASS_END(name, base, parent_name, parent_class)

#define ZUNE_CUSTOMCLASS_INLINE_10(name, base, parent_name, parent_class,             \
                                   inst_data,                                         \
                                   m1,  m1_msg_type,  m1_code,                        \
                                   m2,  m2_msg_type,  m2_code,                        \
                                   m3,  m3_msg_type,  m3_code,                        \
                                   m4,  m4_msg_type,  m4_code,                        \
                                   m5,  m5_msg_type,  m5_code,                        \
                                   m6,  m6_msg_type,  m6_code,                        \
                                   m7,  m7_msg_type,  m7_code,                        \
                                   m8,  m8_msg_type,  m8_code,                        \
                                   m9,  m9_msg_type,  m9_code,                        \
                                   m10, m10_msg_type, m10_code)                       \
    __ZUNE_CUSTOMCLASS_INSTDATA(name, inst_data);                                     \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m1,  m1_msg_type,  m1_code);  \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m2,  m2_msg_type,  m2_code);  \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m3,  m3_msg_type,  m3_code);  \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m4,  m4_msg_type,  m4_code);  \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m5,  m5_msg_type,  m5_code);  \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m6,  m6_msg_type,  m6_code);  \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m7,  m7_msg_type,  m7_code);  \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m8,  m8_msg_type,  m8_code);  \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m9,  m9_msg_type,  m9_code);  \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m10, m10_msg_type, m10_code); \
    __ZUNE_CUSTOMCLASS_START(name)                                                    \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m1,  m1,  m1_msg_type);                   \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m2,  m2,  m2_msg_type);                   \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m3,  m3,  m3_msg_type);                   \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m4,  m4,  m4_msg_type);                   \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m5,  m5,  m5_msg_type);                   \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m6,  m6,  m6_msg_type);                   \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m7,  m7,  m7_msg_type);                   \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m8,  m8,  m8_msg_type);                   \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m9,  m9,  m9_msg_type);                   \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m10, m10, m10_msg_type);                  \
    __ZUNE_CUSTOMCLASS_END(name, base, parent_name, parent_class)

#define ZUNE_CUSTOMCLASS_INLINE_11(name, base, parent_name, parent_class,             \
                                   inst_data,                                         \
                                   m1,  m1_msg_type,  m1_code,                        \
                                   m2,  m2_msg_type,  m2_code,                        \
                                   m3,  m3_msg_type,  m3_code,                        \
                                   m4,  m4_msg_type,  m4_code,                        \
                                   m5,  m5_msg_type,  m5_code,                        \
                                   m6,  m6_msg_type,  m6_code,                        \
                                   m7,  m7_msg_type,  m7_code,                        \
                                   m8,  m8_msg_type,  m8_code,                        \
                                   m9,  m9_msg_type,  m9_code,                        \
                                   m10, m10_msg_type, m10_code,                       \
                                   m11, m11_msg_type, m11_code)                       \
    __ZUNE_CUSTOMCLASS_INSTDATA(name, inst_data);                                     \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m1,  m1_msg_type,  m1_code);  \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m2,  m2_msg_type,  m2_code);  \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m3,  m3_msg_type,  m3_code);  \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m4,  m4_msg_type,  m4_code);  \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m5,  m5_msg_type,  m5_code);  \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m6,  m6_msg_type,  m6_code);  \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m7,  m7_msg_type,  m7_code);  \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m8,  m8_msg_type,  m8_code);  \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m9,  m9_msg_type,  m9_code);  \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m10, m10_msg_type, m10_code); \
    __ZUNE_CUSTOMCLASS_INLINEMETHOD(name, name ## __ ## m11, m11_msg_type, m11_code); \
    __ZUNE_CUSTOMCLASS_START(name)                                                    \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m1,  m1,  m1_msg_type);                   \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m2,  m2,  m2_msg_type);                   \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m3,  m3,  m3_msg_type);                   \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m4,  m4,  m4_msg_type);                   \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m5,  m5,  m5_msg_type);                   \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m6,  m6,  m6_msg_type);                   \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m7,  m7,  m7_msg_type);                   \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m8,  m8,  m8_msg_type);                   \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m9,  m9,  m9_msg_type);                   \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m10, m10, m10_msg_type);                  \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m11, m11, m11_msg_type);                  \
    __ZUNE_CUSTOMCLASS_END(name, base, parent_name, parent_class)

#endif /* !ZUNE_CUSTOMCLASSES_H */
