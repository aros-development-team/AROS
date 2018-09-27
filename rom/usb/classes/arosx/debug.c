
#include "debug.h"

#include "xinput.class.h"

void nDebugMem(struct Library *ps, UBYTE *rptr, ULONG rptlen) {
    char  fmtstr[108];
    STRPTR fmtptr;
    UWORD cnt;
    UWORD pos = 0;

    while(rptlen)
    {
        fmtptr = fmtstr;
        cnt = 16;
        *fmtptr++ = '%';
        *fmtptr++ = '0';
        *fmtptr++ = '4';
        *fmtptr++ = 'l';
        *fmtptr++ = 'x';
        *fmtptr++ = ':';
        *fmtptr++ = ' ';
        do
        {
            *fmtptr++ = '%';
            *fmtptr++ = '0';
            *fmtptr++ = '2';
            *fmtptr++ = 'l';
            *fmtptr++ = 'x';
            //*fmtptr++ = 'd';
            if(--cnt)
            {
                *fmtptr++ = ' ';
            }
            --rptlen;
        } while(cnt && rptlen);
        *fmtptr = 0;
        psdAddErrorMsg(RETURN_WARN, (STRPTR)libname, fmtstr, pos,
                       rptr[0], rptr[1], rptr[2], rptr[3], rptr[4], rptr[5], rptr[6], rptr[7],
                       rptr[8], rptr[9], rptr[10], rptr[11], rptr[12], rptr[13], rptr[14], rptr[15]);
        rptr += 16;
        pos += 16;
    }
}
