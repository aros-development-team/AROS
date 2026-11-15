/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */
#include "i2chid_core.h"
#include <string.h>
#include <limits.h>
static uint16_t le16(const uint8_t *p)
{
    return p[0] | (uint16_t)p[1] << 8;
}
int ih_descriptor(const uint8_t *p, size_t n, struct ih_descriptor *d)
{
    if (n < 30 || le16(p) != 30 || le16(p + 2) != 0x100)
    {
        return 0;
    }
    d->report_length = le16(p + 4);
    d->report_register = le16(p + 6);
    d->input_register = le16(p + 8);
    d->input_length = le16(p + 10);
    d->command_register = le16(p + 16);
    d->data_register = le16(p + 18);
    d->vendor = le16(p + 20);
    d->product = le16(p + 22);
    return d->report_length && d->report_length <= IH_MAX_DESCRIPTOR && d->input_length >= 3 &&
           d->input_length <= IH_MAX_REPORT;
}
struct globals
{
    uint32_t page, size, count, id;
    int32_t minimum, maximum;
};
static int32_t signvalue(uint32_t v, unsigned bits)
{
    if (!bits)
    {
        return 0;
    }
    if (bits < 32 && (v & (1u << (bits - 1))))
    {
        v |= ~((1u << bits) - 1);
    }
    return (int32_t)v;
}
int ih_parse(const uint8_t *p, size_t n, struct ih_layout *l)
{
    struct globals g = {0}, stack[8];
    uint32_t usages[32], umin = 0, umax = 0;
    unsigned nu = 0, sp = 0, depth = 0, next_collection = 0;
    uint16_t collections[16] = {0};
    int range = 0;
    size_t pos = 0;
    memset(l, 0, sizeof(*l));
    if (!n || n > IH_MAX_DESCRIPTOR)
    {
        return 0;
    }
    while (pos < n)
    {
        unsigned prefix = p[pos++], size = prefix & 3, type = (prefix >> 2) & 3, tag = prefix >> 4,
                 i;
        uint32_t value = 0;
        if (prefix == 0xfe)
        {
            return 0; /* Long items are not defined for this parser. */
        }
        if (size == 3)
        {
            size = 4;
        }
        if (size > n - pos)
        {
            return 0;
        }
        for (i = 0; i < size; i++)
        {
            value |= (uint32_t)p[pos++] << (8 * i);
        }
        if (type == 1)
        {
            switch (tag)
            {
            case 0:
                g.page = value;
                break;
            case 1:
                g.minimum = signvalue(value, size * 8);
                break;
            case 2:
                g.maximum = g.minimum < 0 ? signvalue(value, size * 8) : (int32_t)value;
                break;
            case 7:
                g.size = value;
                break;
            case 8:
                if (!value || value > 255)
                {
                    return 0;
                }
                g.id = value;
                l->has_ids = 1;
                break;
            case 9:
                g.count = value;
                break;
            case 10:
                if (sp == 8)
                {
                    return 0;
                }
                stack[sp++] = g;
                break;
            case 11:
                if (!sp)
                {
                    return 0;
                }
                g = stack[--sp];
                break;
            }
        }
        else if (type == 2)
        {
            if (tag == 0)
            {
                if (nu == 32)
                {
                    return 0;
                }
                usages[nu++] = value;
            }
            if (tag == 1)
            {
                umin = value;
                range |= 1;
            }
            if (tag == 2)
            {
                umax = value;
                range |= 2;
            }
        }
        else if (type == 0)
        {
            if (tag == 10)
            {
                if (depth == 16 || next_collection == 65535)
                {
                    return 0;
                }
                collections[depth++] = ++next_collection;
            }
            else if (tag == 12)
            {
                if (!depth)
                {
                    return 0;
                }
                depth--;
            }
            else if (tag == 8)
            {
                unsigned bit = l->report_bits[g.id];
                if (!g.size || g.size > 32 || !g.count || g.count > 4096 ||
                    g.size * g.count > IH_MAX_REPORT * 8 - bit)
                {
                    return 0;
                }
                if (!(value & 1) && (value & 2))
                {
                    for (i = 0; i < g.count; i++)
                    {
                        uint32_t u = 0, page = g.page;
                        struct ih_field *f;
                        if (nu)
                        {
                            u = usages[i < nu ? i : nu - 1];
                        }
                        else if (range == 3 && umax >= umin && i <= umax - umin)
                        {
                            u = umin + i;
                        }
                        if (u > 65535)
                        {
                            page = u >> 16;
                            u &= 65535;
                        }
                        if (!((page == 1 && (u == 0x30 || u == 0x31)) ||
                              (page == 9 && u >= 1 && u <= 3) ||
                              (page == 13 && (u == 0x42 || u == 0x51))))
                        {
                            continue;
                        }
                        if (l->count == IH_MAX_FIELDS)
                        {
                            return 0;
                        }
                        f = &l->fields[l->count++];
                        f->bit = bit + i * g.size;
                        f->size = g.size;
                        f->report = g.id;
                        f->page = page;
                        f->usage = u;
                        f->relative = !!(value & 4);
                        f->minimum = g.minimum;
                        f->maximum = g.maximum;
                        f->collection = depth ? collections[depth - 1] : 0;
                    }
                }
                l->report_bits[g.id] = bit + g.size * g.count;
            }
            nu = 0;
            range = 0;
            umin = umax = 0;
        }
    }
    if (depth || sp || (l->has_ids && l->report_bits[0]))
    {
        return 0;
    }
    for (unsigned a = 0; a < l->count; a++)
    {
        for (unsigned b = a + 1; b < l->count; b++)
        {
            const struct ih_field *x = &l->fields[a], *y = &l->fields[b];
            if (x->page == 1 && y->page == 1 && x->usage == 0x30 && y->usage == 0x31 &&
                x->report == y->report && x->collection == y->collection &&
                x->relative == y->relative)
            {
                return 1;
            }
        }
    }
    return 0;
}
static int32_t fieldvalue(const struct ih_field *f, const uint8_t *p)
{
    uint32_t v = 0;
    for (unsigned i = 0; i < f->size; i++)
    {
        v |= (uint32_t)((p[(f->bit + i) / 8] >> ((f->bit + i) % 8)) & 1) << i;
    }
    return f->minimum < 0 ? signvalue(v, f->size) : (int32_t)v;
}
int ih_decode(const struct ih_layout *l, const uint8_t *p, size_t n, struct ih_sample *s)
{
    unsigned id = 0;
    uint16_t length;
    int x = -1, y = -1;
    memset(s, 0, sizeof(*s));
    s->contact = 1;
    s->id = -1;
    if (n < 2)
    {
        return -1;
    }
    length = le16(p);
    if (!length)
    {
        return 0;
    }
    if (length < 2 || length > n || length > IH_MAX_REPORT)
    {
        return -1;
    }
    p += 2;
    length -= 2;
    if (l->has_ids)
    {
        if (!length)
        {
            return -1;
        }
        id = *p++;
        length--;
    }
    if (!l->report_bits[id])
    {
        return 0;
    }
    if (length < (l->report_bits[id] + 7) / 8)
    {
        return -1;
    }
    /* Select one contact collection; never combine axes from different contacts. */
    for (unsigned a = 0; a < l->count; a++)
    {
        const struct ih_field *f = &l->fields[a];
        if (f->report != id || f->page != 1 || f->usage != 0x30)
        {
            continue;
        }
        for (unsigned b = 0; b < l->count; b++)
        {
            const struct ih_field *g = &l->fields[b];
            if (g->report == id && g->page == 1 && g->usage == 0x31 &&
                g->collection == f->collection && g->relative == f->relative)
            {
                int active = f->relative;
                for (unsigned c = 0; c < l->count && !active; c++)
                {
                    const struct ih_field *tip = &l->fields[c];
                    if (tip->report == id && tip->collection == f->collection && tip->page == 13 &&
                        tip->usage == 0x42)
                    {
                        active = !!fieldvalue(tip, p);
                    }
                }
                if (x < 0 || active)
                {
                    x = a;
                    y = b;
                }
                if (active)
                {
                    goto selected;
                }
                break;
            }
        }
    }
selected:
    if (x < 0)
    {
        return 0;
    }
    const struct ih_field *fx = &l->fields[x], *fy = &l->fields[y];
    s->x = fieldvalue(fx, p);
    s->y = fieldvalue(fy, p);
    s->relative = fx->relative;
    if (!s->relative)
    {
        if (fx->maximum <= fx->minimum || fy->maximum <= fy->minimum)
        {
            return -1;
        }
        s->xrange = (uint32_t)((int64_t)fx->maximum - fx->minimum);
        s->yrange = (uint32_t)((int64_t)fy->maximum - fy->minimum);
        s->contact = 0; /* Absolute coordinates require a Tip Switch. */
    }
    for (unsigned i = 0; i < l->count; i++)
    {
        const struct ih_field *f = &l->fields[i];
        if (f->report != id)
        {
            continue;
        }
        int32_t v = fieldvalue(f, p);
        if (f->page == 9 && f->usage <= 3 && v)
        {
            s->buttons |= 1u << (f->usage - 1);
        }
        if (f->collection != fx->collection)
        {
            continue;
        }
        if (f->page == 13 && f->usage == 0x42)
        {
            s->contact = !!v;
        }
        if (f->page == 13 && f->usage == 0x51)
        {
            s->id = v;
        }
    }
    /* Released contacts may carry stale or unspecified coordinates. */
    if (!s->relative && s->contact &&
        (s->x < fx->minimum || s->x > fx->maximum || s->y < fy->minimum || s->y > fy->maximum))
    {
        return -1;
    }
    return 1;
}
/* DesignWare register offsets and protocol bits. */
#define RD(r) b->read(b->context, (r))
#define WR(r, v) b->write(b->context, (r), (v))
int ih_transfer(struct ih_bus *b, const uint8_t *w, size_t wn, uint8_t *r, size_t rn)
{
    size_t sent = 0, issued = 0, received = 0;
    unsigned spin;
    int stop = 0, ok = 0;
    b->failure = 0;
    if ((!wn && !rn) || wn > IH_MAX_DESCRIPTOR || rn > IH_MAX_DESCRIPTOR || (wn && !w) ||
        (rn && !r) || !b->rx_depth || !b->tx_depth || b->address < 8 || b->address > 0x77)
    {
        return 0;
    }
    /* Do not disable a controller with an in-flight transaction. */
    if (RD(0x70) & 0x21)
    {
        b->failure = 1;
        return 0;
    }
    WR(0x6c, 0);
    for (spin = 0; spin < 1000 && (RD(0x9c) & 1); spin++)
    {
        if (b->pause(b->context))
        {
            break;
        }
    }
    if (RD(0x9c) & 1)
    {
        b->failure = 2;
        return 0;
    }
    WR(0x04, b->address);
    WR(0x30, 0);
    (void)RD(0x40);
    WR(0x6c, 1);
    for (spin = 0; spin < 1000 && !(RD(0x9c) & 1); spin++)
    {
        if (b->pause(b->context))
        {
            break;
        }
    }
    if (!(RD(0x9c) & 1))
    {
        b->failure = 3;
        goto done;
    }
    for (spin = 0; spin < 20000; spin++)
    {
        uint32_t intr = RD(0x34);
        if (intr & 0x4b)
        {
            b->failure = (intr & 0x40) ? RD(0x80) : 0x80000000u;
            goto done;
        }
        if (intr & 0x200)
        {
            stop = 1;
        }
        while (received < issued && RD(0x78))
        {
            r[received++] = (uint8_t)RD(0x10);
        }
        while (sent < wn && RD(0x74) < b->tx_depth)
        {
            uint32_t cmd = w[sent++];
            if (sent == wn && !rn)
            {
                cmd |= 0x200;
            }
            WR(0x10, cmd);
        }
        while (sent == wn && issued < rn && issued - received < b->rx_depth &&
               RD(0x74) < b->tx_depth)
        {
            uint32_t cmd = 0x100;
            if (!issued && wn)
            {
                cmd |= 0x400;
            }
            if (++issued == rn)
            {
                cmd |= 0x200;
            }
            WR(0x10, cmd);
        }
        if (sent == wn && received == rn && stop && !(RD(0x70) & 0x21))
        {
            ok = 1;
            break;
        }
        if (b->pause(b->context))
        {
            b->failure = 4;
            goto done;
        }
    }
    if (!ok)
    {
        b->failure = 5;
    }
done:
    WR(0x6c, 0);
    for (spin = 0; spin < 1000 && (RD(0x9c) & 1); spin++)
    {
        (void)b->pause(b->context);
    }
    if (RD(0x9c) & 1)
    {
        b->failure = 6;
        ok = 0;
    }
    (void)RD(0x40);
    return ok;
}
