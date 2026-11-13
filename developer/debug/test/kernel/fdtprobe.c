/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: What the firmware's device tree actually says, which is not
          what the vendor's Linux sources say.

          With no arguments it reports the root, the pin controller and
          every i2c controller. PINS dumps the pin groups. NODE prints
          the properties of every node whose name or compatible contains
          that text.
*/

#include <exec/types.h>
#include <dos/dos.h>
#include <aros/kernel.h>
#include <utility/tagitem.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/kernel.h>
#include <proto/utility.h>

#include <fdt.h>

#include <stdio.h>
#include <string.h>

static const char version[] __attribute__((used)) =
    "$VER: FDTProbe 1.0 (13.10.2026)";

#define TEMPLATE "NODE/K,PINS/S,ALL/S"

enum { ARG_NODE, ARG_PINS, ARG_ALL, ARG_COUNT };

static void showreg(fdt_node_t n)
{
    UQUAD addr = 0, size = 0;

    if (FDT_GetReg(n, 0, &addr, &size))
        printf(" reg %p+%p", (APTR)(IPTR)addr, (APTR)(IPTR)size);
}

/* The properties a pinctrl client needs */
static void showpinctrl(fdt_node_t n)
{
    ULONG len = 0;
    const ULONG *p = FDT_GetProp(n, "pinctrl-0", &len);

    if (!p)
    {
        printf("   pinctrl-0: ABSENT\n");
        return;
    }

    printf("   pinctrl-0: %u phandle(s):", (unsigned)(len / 4));
    {
        ULONG i;
        for (i = 0; i < len / 4; i++)
            printf(" %08lx", (unsigned long)FDT_ReadCells(&p[i], 1));
    }
    printf("\n");
}

/* Every property of a node, by enumeration rather than by guessing */
static void showprops(fdt_node_t n)
{
    fdt_prop_t p;
    int count = 0;

    printf("%s:\n", (char *)FDT_NodeName(n));

    for (p = FDT_FirstProp(n); p != FDT_PROPNONE; p = FDT_NextProp(p))
    {
        CONST_STRPTR name = FDT_PropName(p);
        ULONG len = 0;
        const UBYTE *d = FDT_PropData(p, &len);
        ULONG i;
        BOOL printable = (len > 1) && (d[len - 1] == 0);

        printf("    %-20s %3lu byte(s)", name ? (char *)name : "?",
               (unsigned long)len);

        if (!d || !len)
        {
            printf("  (empty - a flag)\n");
            count++;
            continue;
        }

        /* A string if every byte is one, otherwise cells */
        for (i = 0; i < len - 1 && printable; i++)
            if (d[i] && (d[i] < 0x20 || d[i] > 0x7e))
                printable = FALSE;

        if (printable)
        {
            /* A string property is often a list of them */
            ULONG o = 0;
            int n = 0;

            printf(" ");
            while (o < len && d[o])
            {
                printf("%s\"%s\"", n++ ? ", " : "", (char *)&d[o]);
                o += strlen((char *)&d[o]) + 1;
            }
            if (n > 1)
                printf("  (%d strings)", n);
        }
        else if ((len & 3) == 0)
        {
            const ULONG *c = (const ULONG *)d;

            printf("  <");
            for (i = 0; i < len / 4 && i < 24; i++)
                printf("%s%lx", i ? " " : "",
                       (unsigned long)FDT_ReadCells(&c[i], 1));
            printf("%s>", (len / 4 > 24) ? " ..." : "");
        }
        else
        {
            printf("  [");
            for (i = 0; i < len && i < 24; i++)
                printf("%s%02x", i ? " " : "", d[i]);
            printf("%s]", (len > 24) ? " ..." : "");
        }

        printf("\n");
        count++;
    }

    if (!count)
        printf("    (no properties)\n");
}

int main(void)
{
    IPTR args[ARG_COUNT] = { 0 };
    struct RDArgs *rda;
    APTR KernelBase;
    struct TagItem *bootinfo;
    APTR dtb;
    fdt_node_t n;
    int rc = RETURN_FAIL, count;

    rda = ReadArgs(TEMPLATE, args, NULL);
    if (!rda)
    {
        PrintFault(IoErr(), "FDTProbe");
        return RETURN_FAIL;
    }

    KernelBase = OpenResource("kernel.resource");
    if (!KernelBase)
    {
        printf("no kernel.resource\n");
        goto out;
    }

    bootinfo = KrnGetBootInfo();
    dtb = (APTR)GetTagData(KRN_FlattenedDeviceTree, 0, bootinfo);

    if (!dtb || !FDT_Open(dtb))
    {
        printf("the firmware handed over no device tree\n");
        goto out;
    }

    printf("device tree at %p\n\n", dtb);

    {
        CONST_STRPTR c = FDT_GetPropStr(FDT_Root(), "compatible");
        CONST_STRPTR m = FDT_GetPropStr(FDT_Root(), "model");

        printf("root model      : %s\n", m ? (char *)m : "(none)");
        printf("root compatible : %s\n\n", c ? (char *)c : "(none)");
    }

    if (args[ARG_PINS])
    {
        for (n = FDT_Root(), count = 0; n != FDT_NONE; n = FDT_NextNode(n))
        {
            CONST_STRPTR nm = FDT_NodeName(n);

            if (nm && (strstr((char *)nm, "-pins") ||
                       strstr((char *)nm, "pinmux")))
            {
                showprops(n);
                count++;
            }
        }
        printf("\n%d pin node(s)\n", count);
        rc = RETURN_OK;
        goto out;
    }

    if (args[ARG_NODE])
    {
        const char *want = (const char *)args[ARG_NODE];

        for (n = FDT_Root(), count = 0; n != FDT_NONE; n = FDT_NextNode(n))
        {
            CONST_STRPTR nm = FDT_NodeName(n);

            if ((nm && strstr((char *)nm, want)) || FDT_IsCompatible(n, want))
            {
                CONST_STRPTR c = FDT_GetPropStr(n, "compatible");

                (void)c;
                showreg(n);
                showprops(n);
                count++;
            }
        }
        printf("\n%d node(s) matching \"%s\"\n", count, want);
        rc = RETURN_OK;
        goto out;
    }

    printf("--- pin controller ---\n");
    n = FDT_FindCompatible(FDT_NONE, "ultrarisc,dp1000-pinctrl");
    if (n == FDT_NONE)
    {
        printf("NOT DESCRIBED - nothing can learn the pin mux from this tree\n");
    }
    else
    {
        printf("%s", (char *)FDT_NodeName(n));
        showreg(n);
        printf("\n");
    }

    printf("\n--- i2c controllers ---\n");
    for (n = FDT_FindCompatible(FDT_NONE, "snps,designware-i2c"), count = 0;
         n != FDT_NONE; n = FDT_FindCompatible(n, "snps,designware-i2c"))
    {
        printf("unit %d  %s", count, (char *)FDT_NodeName(n));
        showreg(n);
        printf("\n");
        showpinctrl(n);

        /* Children - the chips the tree says hang off this bus */
        {
            fdt_node_t c = FDT_NextNode(n);

            if (c != FDT_NONE)
            {
                CONST_STRPTR cc = FDT_GetPropStr(c, "compatible");
                UQUAD a = 0;

                if (cc)
                {
                    FDT_GetReg(c, 0, &a, NULL);
                    printf("   next node: %s, compatible %s, reg %02x\n",
                           (char *)FDT_NodeName(c), (char *)cc,
                           (unsigned)a);
                }
            }
        }
        count++;
    }
    printf("\n%d i2c controller(s)\n", count);

    rc = RETURN_OK;

out:
    FreeArgs(rda);
    return rc;
}
