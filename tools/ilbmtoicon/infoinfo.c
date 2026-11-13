/*
 * Copyright (C) 2011, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 *
 * This program dumps out a textual description of an icon,
 * suitable for use with ilbmtoicon.
 *
 * We are intentionally using only the C library here, as this
 * program should be able to be run on both Unix and AmigaOS
 * systems.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

#define HAS_DRAWERDATA     (1 << 0)
#define HAS_GADGETRENDER   (1 << 1)
#define HAS_SELECTRENDER   (1 << 2)
#define HAS_USERDATA       (1 << 3)
#define HAS_DEFAULTTOOL    (1 << 4)
#define HAS_TOOLTYPES      (1 << 5)

const char *file_in, *file_iff;

void w8(int fd, uint8_t val)
{
    int err;

    err = write(fd, &val, 1);
    if (err != 1) {
        perror(file_iff);
        exit(EXIT_FAILURE);
    }
}

void w32(int fd, uint32_t val)
{
    int err;

    val = htonl(val);
    err = write(fd, &val, 4);
    if (err != 4) {
        perror(file_iff);
        exit(EXIT_FAILURE);
    }
}

uint32_t r32(int fd)
{
    uint32_t val = 0;
    if (read(fd, &val, 4) != 4) {
        perror(file_in);
        exit(EXIT_FAILURE);
    }

    return ntohl(val);
}

uint16_t r16(int fd)
{
    uint16_t val = 0;
    if (read(fd, &val, 2) != 2) {
        perror(file_in);
        exit(EXIT_FAILURE);
    }

    return ntohs(val);
}

uint8_t r8(int fd)
{
    uint8_t val = 0;
    if (read(fd, &val, 1) != 1) {
        perror(file_in);
        exit(EXIT_FAILURE);
    }

    return val;
}

#define DUMP8(name, fmt, cast) do { t8 = r8(fd); printf("%s = " fmt "\n", name, (cast)t8); } while (0);
#define DUMP16(name, fmt, cast) do { t16 = r16(fd); printf("%s = " fmt "\n", name, (cast)t16); } while (0);
#define DUMP32(name, fmt, cast) do { t32 = r32(fd); printf("%s = " fmt "\n", name, (cast)t32); } while (0);

#define COMMENT_X8(name)        DUMP8("; " name, "0x%x", unsigned int)
#define COMMENT_U8(name)        DUMP8("; " name, "%u", unsigned int)
#define COMMENT_X16(name)       DUMP16("; " name, "0x%x", unsigned int)
#define COMMENT_I16(name)       DUMP16("; " name, "%d", short)
#define COMMENT_U16(name)       DUMP16("; " name, "%u", unsigned int)
#define COMMENT_X32(name)       DUMP32("; " name, "0x%x", unsigned int)

#define FIELD_I16(name)         DUMP16(name, "%d", short)
#define FIELD_I32(name)         DUMP32(name, "%d", int)
#define FIELD_U16(name)         DUMP16(name, "%u", unsigned int)
#define FIELD_U32(name)         DUMP32(name, "%u", unsigned int)

uint32_t dump_diskobject(int fd)
{
    uint32_t flags = 0;
    uint32_t t32;
    uint16_t t16;
    uint8_t t8;
    const char *tstr;

    COMMENT_X16("MAGIC");
    COMMENT_X16("VERSION");
    COMMENT_X32("NEXTGADGET");
    FIELD_I16("ICONLEFTPOS");
    FIELD_I16("ICONTOPPOS");
    COMMENT_U16("ICONWIDTH");
    COMMENT_U16("ICONHEIGHT");
    COMMENT_X16("ICONFLAGS");
    COMMENT_X16("ICONACTIVATION");
    COMMENT_U16("ICONGADGETTYPE");
    COMMENT_X32("ICONGADGETRENDER");
    if (t32) flags |= HAS_GADGETRENDER;
    COMMENT_X32("ICONSELECTRENDER");
    if (t32) flags |= HAS_SELECTRENDER;
    COMMENT_X32("ICONGADGETTEXT");
    COMMENT_X32("ICONMUTUALEXCLUDE");
    COMMENT_X32("ICONSPECIALINFO");
    COMMENT_X16("ICONGADGETID");
    COMMENT_X32("ICONUSERDATA");
    if (t32) flags |= HAS_USERDATA;
    t8 = r8(fd);
    tstr = NULL;
    switch (t8) {
    case 1: tstr = "DISK"; break;
    case 2: tstr = "DRAWER"; break;
    case 3: tstr = "TOOL"; break;
    case 4: tstr = "PROJECT"; break;
    case 5: tstr = "GARBAGE"; break;
    case 6: tstr = "DEVICE"; break;
    case 7: tstr = "KICK"; break;
    case 8: tstr = "APPICON"; break;
    default: break;
    }
    if (tstr)
        printf("TYPE = %s\n", tstr);
    else
        printf("; TYPE = %u\n", (unsigned int)t8);
    COMMENT_X8("ICONPADDING");
    COMMENT_X32("ICONDEFAULTTOOL");
    if (t32) flags |= HAS_DEFAULTTOOL;
    COMMENT_X32("ICONTOOLTYPES");
    if (t32) flags |= HAS_TOOLTYPES;
    COMMENT_X32("ICONCURRENTX");
    COMMENT_X32("ICONCURRENTY");
    COMMENT_X32("ICONDRAWERDATA");
    if (t32) flags |= HAS_DRAWERDATA;
    COMMENT_X32("ICONTOOLWINDOW");
    FIELD_U32("STACK");

    return flags;
}

void dump_drawerdata(int fd)
{
    uint32_t t32;
    uint16_t t16;
    uint32_t t8;

    FIELD_I16("DRAWERLEFTPOS");
    FIELD_I16("DRAWERTOPPOS");
    FIELD_I16("DRAWERWIDTH");
    FIELD_I16("DRAWERHEIGHT");
    COMMENT_U8("DRAWERDETAILPEN");
    COMMENT_U8("DRAWERBLOCKPEN");
    COMMENT_X32("DRAWERIDCMP");
    COMMENT_X32("DRAWERFLAGS");
    COMMENT_X32("DRAWERFIRSTGADGET");
    COMMENT_X32("DRAWERCHECKMARK");
    COMMENT_X32("DRAWERTITLE");
    COMMENT_X32("DRAWERSCREEN");
    COMMENT_X32("DRAWERBITMAP");
    COMMENT_U16("DRAWERMINWIDTH");
    COMMENT_U16("DRAWERMINHEIGHT");
    COMMENT_U16("DRAWERMAXWIDTH");
    COMMENT_U16("DRAWERMAXHEIGHT");
    COMMENT_U16("DRAWERTYPE");
    FIELD_I32("DRAWERVIEWLEFT");
    FIELD_I32("DRAWERVIEWTOP");
}

void dump_bitmap(int fd, int id)
{
    uint16_t t16;
    uint32_t t32;
    uint8_t t8;
    unsigned int depth, rows, bytesperrow;
    unsigned int plane, x, y;

    printf("; BitMap (%sRender)\n", id ? "Select" : "Gadget");
    COMMENT_I16("LeftEdge");
    COMMENT_I16("TopEdge");
    COMMENT_U16("Width");
    bytesperrow = ((((unsigned int)t16) + 15) & ~15) / 8;
    COMMENT_U16("Height");
    rows = (unsigned int)t16;
    COMMENT_U16("Depth");
    depth = (unsigned int)t16;
    COMMENT_X32("ImageData");
    COMMENT_X8("PlanePick");
    COMMENT_X8("PlaneOnOff");
    COMMENT_X32("NextImage");
    
    for (plane = 0; plane < depth; plane++) {
        printf("; Plane[%d] = { \n", plane);
        for (y = 0; y < rows; y++) {
            printf("; /* %-3d */", y);
            for (x = 0; x < bytesperrow; x++) {
                t8 = r8(fd);
                printf(" 0x%2x,", (unsigned int)t8);
            }
            printf("\n");
        }
    }
}

void dump_string(int fd, const char *name)
{
    uint32_t len;

    printf("%s = \"", name);
    for (len = r32(fd); len > 0; len--) {
        uint8_t t8 = r8(fd);
        printf("%c", t8);
    }
    printf("\"\n");
}

void dump_strings(int fd, const char *name)
{
    uint32_t strings;

    for (strings = (r32(fd)-4)/4; strings > 0; strings--) {
        dump_string(fd, name);
    }
}

void dump_newdrawerdata(int fd)
{
    uint32_t t32;
    uint16_t t16;
    const char *tstr;

    t32 = r32(fd);
    tstr = NULL;
    switch (t32) {
    case 0: tstr = "DEFAULT"; break;
    case 1: tstr = "ICONS"; break;
    case 2: tstr = "ALL"; break;
    default: break;
    }
    if (tstr == NULL)
        printf("; DRAWERSHOW = 0x%x\n", (unsigned int)t32);
    else
        printf("DRAWERSHOW = %s\n", tstr);

    t16 = r16(fd);
    tstr = NULL;
    switch (t16) {
    case 0: tstr = "DEFAULT"; break;
    case 1: tstr = "ICON"; break;
    case 2: tstr = "TEXT_NAME"; break;
    case 3: tstr = "TEXT_DATE"; break;
    case 4: tstr = "TEXT_SIZE"; break;
    case 5: tstr = "TEXT_TYPE"; break;
    default: break;
    }
    if (tstr == NULL)
        printf("; DRAWERSHOWAS = %u\n", (unsigned int)t16);
    else
        printf("DRAWERSHOWAS = %s\n", tstr);
}

#define MAKE_ID(a,b,c,d)        (((a) << 24) | ((b) << 16) | ((c) << 8) | (d))
#define IFF_FORM        MAKE_ID('F', 'O', 'R', 'M')
#define IFF_CAT         MAKE_ID('C', 'A', 'T', ' ')
#define IFF_LIST        MAKE_ID('L', 'I', 'S', 'T')

static const char *type2str(uint32_t type)
{
    static char str[5] = {};

    str[0] = (type >> 24) & 0xff;
    str[1] = (type >> 16) & 0xff;
    str[2] = (type >>  8) & 0xff;
    str[3] = (type >>  0) & 0xff;

    return str;
}

uint32_t iff_copy_chunk(int fd, int ofd, int depth)
{
    uint32_t type;
    uint32_t len, total;
    int err;

    type = r32(fd);
    len = total = r32(fd);

    printf("; %*sBegin %s (%d)\n", depth, "", type2str(type), (int)len);
    w32(ofd, type);
    w32(ofd, len);
    if (type == IFF_FORM) {
        uint32_t subtype;
        subtype = r32(fd);
        len -= 4;
        printf("; %*s  FORM type %s\n", depth, "", type2str(subtype));
        while (len > 0)
            len -= iff_copy_chunk(fd, ofd, depth+1);
    } else if (type == IFF_CAT) {
        while (len > 0)
            len -= iff_copy_chunk(fd, ofd, depth+1);
    } else {
        /* Just copy the bytes */
        while (len > 0) {
            uint8_t buff[256];
            err = read(fd, buff, (len >= sizeof(buff)) ? sizeof(buff) : len);
            if (err == 0) {
                printf("ERROR: Still had %d bytes to read!\n", (int)len);
                exit(EXIT_FAILURE);
            }
            if (err < 0) {
                perror(file_in);
                exit(EXIT_FAILURE);
            }
            if (write(ofd, buff, err) != err) {
                perror(file_iff);
                exit(EXIT_FAILURE);
            }
            len -= err;
        }
    }
    if (total & 1) {
        w8(ofd,r8(fd));
        total++;
    }
    printf("; %*sEnd %s\n", depth, "", type2str(type));

    return total + 8;
}

void dump_iff(int fd, const char *file_iff)
{
    int ofd;
#if (0)
    int len;
    uint8_t buff[256];
#endif

    ofd = open(file_iff, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (ofd < 0) {
        perror(file_iff);
        exit(EXIT_FAILURE);
    }

    /* Read a single IFF tag, which may contain nested data */
    iff_copy_chunk(fd, ofd, 0);

    close(ofd);
}

int main(int argc, char **argv)
{
    int fd;
    uint32_t flags;

    if (argc != 2 && argc != 3) {
        printf("Usage:\n%s file.info [file.iff] >file.info.src\n", argv[0]);
    }

    if (argc == 3)
        file_iff = argv[2];

    file_in = argv[1];
    fd = open(file_in, O_RDONLY);
    if (fd < 0) {
        perror(file_in);
        return EXIT_FAILURE;
    }

    flags = dump_diskobject(fd);
    if (flags & HAS_DRAWERDATA) {
        dump_drawerdata(fd);
    }
    if (flags & HAS_GADGETRENDER) {
        dump_bitmap(fd, 0);
    }
    if (flags & HAS_SELECTRENDER) {
        dump_bitmap(fd, 1);
    }
    if (flags & HAS_DEFAULTTOOL) {
        dump_string(fd, "DEFAULTTOOL");
    }
    if (flags & HAS_TOOLTYPES) {
        dump_strings(fd, "TOOLTYPE");
    }
    if ((flags & HAS_DRAWERDATA) && (flags & HAS_USERDATA)) {
        dump_newdrawerdata(fd);
    }
    if (flags & HAS_USERDATA) {
        if (file_iff == NULL) {
            printf("; NewIcon v3.5 IFF data was in the file\n");
        } else {
            printf("; NewIcon v3.5 IFF data dumped to %s\n", file_iff);
            dump_iff(fd, file_iff);
        }
    }

    close(fd);

    return 0;
}
