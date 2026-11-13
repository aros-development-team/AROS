#ifndef _LEVELMETER_PRIVATE_H_
#define _LEVELMETER_PRIVATE_H_

struct Levelmeter_DATA
{
    LONG levelbgpen;
    LONG normalpen;
    LONG warningpen;
    LONG criticalpen;
    DOUBLE prevangle;
    ULONG style;
    ULONG segments;
    ULONG warningSegments;
    ULONG criticalSegments;
    BOOL horiz;
};

#endif /* _LEVELMETER_PRIVATE_H_ */
