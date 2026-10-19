#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint32_t ULONG; typedef int16_t WORD; typedef int8_t BYTE;
typedef int32_t LONG; typedef void *APTR; typedef int64_t Fixed64;
#include "ahi-functions.c"
int main(void) {
    struct sample input[16]; for (int i=0;i<16;i++) { input[i].left=256+i; input[i].right=512+i; }
    typedef void (*capture)(ULONG,ULONG,APTR,ULONG*,void**);
    capture funcs[]={RecM8S,RecS8S,RecM16S,RecS16S,RecM32S,RecS32S};
    const unsigned bytes[]={1,2,2,4,4,8};
    for (unsigned f=0;f<6;f++) {
        unsigned char out[64]={0}; void *dest=out; ULONG offset=0;
        funcs[f](2,65536,input,&offset,&dest);
        assert(offset==8); assert((unsigned char*)dest==out+2*bytes[f]);
        funcs[f](2,65536,input,&offset,&dest);
        assert(offset==16); assert((unsigned char*)dest==out+4*bytes[f]);
        offset=4; dest=out;
        funcs[f](2,131072,input,&offset,&dest); assert(offset==20);
    }
    WORD out[8]={0}; void *dest=out; ULONG offset=0;
    RecS16S(2,65536,input,&offset,&dest); RecS16S(2,65536,input,&offset,&dest);
    for(int i=0;i<4;i++) {assert(out[2*i]==input[i].left); assert(out[2*i+1]==input[i].right);}
    puts("AHI frame offsets and consecutive PCM samples: PASS"); return 0;
}
