#ifndef ZUNE_CUSTOMCLASSES_H
#define ZUNE_CUSTOMCLASSES_H

#include <proto/muimaster.h>
#include <aros/symbolsets.h>

#define __ZUNE_CUSTOMCLASS_START(name)                               \
BOOPSI_DISPATCHER(IPTR, name ## _Dispatcher, __class, __self, __msg) \
{                                                                    \
    switch (__msg->MethodID)                                         \
    {                                                                \

#define __ZUNE_CUSTOMCLASS_END(name, base, parent_name, parent_class) \
        default:                                                      \
            return DoSuperMethodA(__class, __self, __msg);            \
    }                                                                 \
                                                                      \
    return NULL;                                                      \
}                                                                     \
                                                                      \
struct MUI_CustomClass * name ## _CLASS;                              \
                                                                      \
int name ## _Initialize(void)                                         \
{                                                                     \
    name ## _CLASS = MUI_CreateCustomClass                            \
    (                                                                 \
        base, parent_name, parent_class,                              \
        sizeof(struct name ## _DATA), name ## _Dispatcher             \
    );                                                                \
                                                                      \
    return name ## _CLASS != NULL ? 0 : 20;                           \
}                                                                     \
                                                                      \
void name ## _Deinitialize(void)                                      \
{                                                                     \
    MUI_DeleteCustomClass(name ## _CLASS);                            \
}                                                                     \
                                                                      \
ADD2INIT(name ## _Initialize,   100);                                 \
ADD2EXIT(name ## _Deinitialize, 100);                                 \

#define __ZUNE_CUSTOMCLASS_METHOD(mname, mid, m_msg_type) \
    case mid:                                             \
        return mname(__class, __self, (m_msg_type)__msg)

/******************************************************************/

#define ZUNE_CUSTOMCLASS1(name, base, parent_name, parent_class,  \
                          m1, m1_msg_type)                        \
    __ZUNE_CUSTOMCLASS_START(name)                                \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m1, m1, m1_msg_type); \
    __ZUNE_CUSTOMCLASS_END(name, base, parent_name, parent_class) \

#define ZUNE_CUSTOMCLASS2(name, base, parent_name, parent_class,  \
                          m1, m1_msg_type,                        \
                          m2, m2_msg_type)                        \
    __ZUNE_CUSTOMCLASS_START(name)                                \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m1, m1, m1_msg_type); \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m2, m2, m2_msg_type); \
    __ZUNE_CUSTOMCLASS_END(name, base, parent_name, parent_class) \

#define ZUNE_CUSTOMCLASS3(name, base, parent_name, parent_class,  \
                          m1, m1_msg_type,                        \
                          m2, m2_msg_type,                        \
                          m3, m3_msg_type)                        \
    __ZUNE_CUSTOMCLASS_START(name)                                \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m1, m1, m1_msg_type); \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m2, m2, m2_msg_type); \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m3, m3, m3_msg_type); \
    __ZUNE_CUSTOMCLASS_END(name, base, parent_name, parent_class) \

#define ZUNE_CUSTOMCLASS4(name, base, parent_name, parent_class,  \
                          m1, m1_msg_type,                        \
                          m2, m2_msg_type,                        \
                          m3, m3_msg_type,                        \
                          m4, m4_msg_type)                        \
    __ZUNE_CUSTOMCLASS_START(name)                                \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m1, m1, m1_msg_type); \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m2, m2, m2_msg_type); \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m3, m3, m3_msg_type); \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m4, m4, m4_msg_type); \
    __ZUNE_CUSTOMCLASS_END(name, base, parent_name, parent_class) \

#define ZUNE_CUSTOMCLASS5(name, base, parent_name, parent_class,  \
                          m1, m1_msg_type,                        \
                          m2, m2_msg_type,                        \
                          m3, m3_msg_type,                        \
                          m4, m4_msg_type,                        \
                          m5, m5_msg_type)                        \
    __ZUNE_CUSTOMCLASS_START(name)                                \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m1, m1, m1_msg_type); \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m2, m2, m2_msg_type); \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m3, m3, m3_msg_type); \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m4, m4, m4_msg_type); \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m5, m5, m5_msg_type); \
    __ZUNE_CUSTOMCLASS_END(name, base, parent_name, parent_class) \

#define ZUNE_CUSTOMCLASS6(name, base, parent_name, parent_class,  \
                          m1, m1_msg_type,                        \
                          m2, m2_msg_type,                        \
                          m3, m3_msg_type,                        \
                          m4, m4_msg_type,                        \
                          m5, m5_msg_type,                        \
                          m6, m6_msg_type)                        \
    __ZUNE_CUSTOMCLASS_START(name)                                \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m1, m1, m1_msg_type); \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m2, m2, m2_msg_type); \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m3, m3, m3_msg_type); \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m4, m4, m4_msg_type); \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m5, m5, m5_msg_type); \
    __ZUNE_CUSTOMCLASS_METHOD(name ## __ ## m6, m6, m6_msg_type); \
    __ZUNE_CUSTOMCLASS_END(name, base, parent_name, parent_class) \

#endif /* !ZUNE_CUSTOMCLASSES_H */
