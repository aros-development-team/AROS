#pragma once
#define AROS_BE2LONG(x) __builtin_bswap32((unsigned int)(x))
#define AROS_LONG2BE(x) __builtin_bswap32((unsigned int)(x))
#define AROS_BE2WORD(x) __builtin_bswap16((unsigned short)(x))
#define AROS_WORD2BE(x) __builtin_bswap16((unsigned short)(x))
