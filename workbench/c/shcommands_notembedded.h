#ifndef SHCOMMANDS_NOTEMBEDDED_H
#define SHCOMMANDS_NOTEMBEDDED_H

#include <dos/dos.h>
#include <proto/dos.h>

#define _stringify(x) #x
#define stringify(x) _stringify(x)

#define SHArg(name) __shargs[SHA_##name]
#define SHReturn(code) {__shretcode = code; goto __shexitpoint;}

#define __SHA_ENUM(abbr, name, modf, def) SHA_##name
#define __SHA_DEF(abbr, name, modf, def) (IPTR)(def)
#define __SHA_OPT(abbr, name, modf, def) \
     stringify(abbr) stringify(name) stringify(modf)


#define __AROS_SH_COMMON(name, version, date)                  \
static const char name##_version[] = "$VER: "                  \
                                     stringify(name) " "       \
			             stringify(version) " ("   \
			             stringify(date)    ")\n"; \
int __nocommandline = 1;                                       \
                                                               \
int main (void)                                                \
{                                                              \
    int __shretcode = RETURN_OK;                               \
    static char __shcommandname[] = stringify(name);           \


#define __AROS_SH_ARGS(numargs, defaults, template) \
    IPTR __shargs[numargs] = defaults;              \
    struct RDArgs *___rda;                          \
                                                    \
    ___rda = ReadArgs(template, __shargs, NULL);    \
					            \
    if (!___rda)                                    \
	SHReturn(RETURN_FAIL)

#define AROS_SHCOMMAND_INIT

#define AROS_SHCOMMAND_EXIT                   \
    }                                         \
__shexitpoint:                                \
    if (___rda)                               \
         FreeArgs(___rda);                    \
    if (IoErr() && __shretcode)               \
        PrintFault(IoErr(), __shcommandname); \
    return __shretcode;                       \
}


#define AROS_SH0(name, version, date)     \
    __AROS_SH_COMMON(name, version, date) \
    const struct RDArgs *___rda = NULL;

#define __DEF(x...) {x}

#define AROS_SH1(name, version, date, a1)                  \
    __AROS_SH_COMMON(name, version, date)                  \
    __AROS_SH_ARGS(1, __DEF(__SHA_DEF(a1)), __SHA_OPT(a1)) \
    {                                                      \
        enum {__SHA_ENUM(a1)};

#define AROS_SH2(name, version, date, a1, a2)              \
    __AROS_SH_COMMON(name, version, date)                  \
    __AROS_SH_ARGS(2, __DEF(__SHA_DEF(a1), __SHA_DEF(a2)), \
                      __SHA_OPT(a1) "," __SHA_OPT(a2))     \
    {                                                      \
        enum {__SHA_ENUM(a1), __SHA_ENUM(a2)};

#define AROS_SH3(name, version, date, a1, a2, a3)         \
    __AROS_SH_COMMON(name, version, date)                 \
    __AROS_SH_ARGS(3, __DEF(__SHA_DEF(a1), __SHA_DEF(a2), \
                       __SHA_DEF(a3)),                    \
                      __SHA_OPT(a1) "," __SHA_OPT(a2) "," \
		      __SHA_OPT(a3))                      \
    {                                                     \
        enum {__SHA_ENUM(a1), __SHA_ENUM(a2)              \
	      __SHA_ENUM(a3)};

#define AROS_SH4(name, version, date, a1, a2, a3, a4)     \
    __AROS_SH_COMMON(name, version, date)                 \
    __AROS_SH_ARGS(4, __DEF(__SHA_DEF(a1), __SHA_DEF(a2)  \
                       __SHA_DEF(a3), __SHA_DEF(a4)),     \
                      __SHA_OPT(a1) "," __SHA_OPT(a2) "," \
		      __SHA_OPT(a3) "," __SHA_OPT(a4))    \
    {                                                     \
        enum {__SHA_ENUM(a1), __SHA_ENUM(a2)              \
	      __SHA_ENUM(a3), __SHA_ENUM(a4)};

#define AROS_SH5(name, version, date, a1, a2, a3, a4, a5)     \
    __AROS_SH_COMMON(name, version, date)                     \
    __AROS_SH_ARGS(5, __DEF(__SHA_DEF(a1), __SHA_DEF(a2),     \
                       __SHA_DEF(a3), __SHA_DEF(a4),          \
		       __SHA_DEF(a5)),                        \
                      __SHA_OPT(a1) "," __SHA_OPT(a2) ","     \
                      __SHA_OPT(a3) "," __SHA_OPT(a4) ","     \
		      __SHA_OPT(a5))                          \
    {                                                         \
        enum {__SHA_ENUM(a1), __SHA_ENUM(a2), __SHA_ENUM(a3), \
	      __SHA_ENUM(a4), __SHA_ENUM(a5)};                \


#define AROS_SHA(abbr, name, modf, def) abbr,name,modf,def

#endif
