/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: 64-bit division for the Kickstart ROM
*/

/*
 * libgcc's __divdi3, __udivdi3 and __umoddi3 are about 1.4 KB each on the
 * 68000. The ROM links these instead: one shift and subtract core with fast
 * paths for the common cases, 32-bit operands and 16-bit divisors. The
 * results are the same, including the divide-by-zero trap. Anything else
 * still comes from libgcc.
 */

typedef unsigned long long UDItype;
typedef long long DItype;
typedef unsigned long USItype;

UDItype __udivdi3(UDItype n, UDItype d);
UDItype __umoddi3(UDItype n, UDItype d);
DItype __divdi3(DItype n, DItype d);

static UDItype udivmoddi4(UDItype n, UDItype d, UDItype *rp)
{
    USItype nh = n >> 32, dh = d >> 32;
    UDItype q;

    if (dh == 0)
    {
        USItype dl = d, r;

        /* Also the divide-by-zero case: let the 32-bit division trap */
        if (nh == 0 || dl == 0)
        {
            q = (USItype)n / dl;
            r = (USItype)n % dl;
        }
        else if (dl <= 0xffff)
        {
            /* Four 32/16 steps; the remainder always stays below 16 bits */
            int i;

            q = 0;
            r = 0;
            for (i = 48; i >= 0; i -= 16)
            {
                r = (r << 16) | (USItype)((n >> i) & 0xffff);
                q = (q << 16) | (r / dl);
                r %= dl;
            }
        }
        else
            goto slow;

        *rp = r;
        return q;
    }

slow:
    {
        UDItype r = 0;
        int i;

        q = n;
        for (i = 0; i < 64; i++)
        {
            r = (r << 1) | (q >> 63);
            q <<= 1;
            if (r >= d)
            {
                r -= d;
                q |= 1;
            }
        }
        *rp = r;
        return q;
    }
}

UDItype __udivdi3(UDItype n, UDItype d)
{
    UDItype r;

    return udivmoddi4(n, d, &r);
}

UDItype __umoddi3(UDItype n, UDItype d)
{
    UDItype r;

    udivmoddi4(n, d, &r);
    return r;
}

DItype __divdi3(DItype n, DItype d)
{
    UDItype r, q;
    int neg = 0;

    if (n < 0)
    {
        n = -(UDItype)n;
        neg = 1;
    }
    if (d < 0)
    {
        d = -(UDItype)d;
        neg ^= 1;
    }
    q = udivmoddi4(n, d, &r);
    return neg ? -q : q;
}
