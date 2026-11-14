/*
 * Copyright (C) 2026, The AROS Development Team.  All rights reserved.
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 *
 * Desc: Turn a PDF printer driver job file into a PDF document
 */

#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <proto/dos.h>

#include <hpdf.h>

#include "pdfjob.h"

const char version[] = "$VER: PDFRender 1.1 (10.11.2026)";

/* libHaru keeps every object until the document is saved, so count its memory */
typedef union {
    size_t size;
    long double align;
    void *ptr;
} Allocation;

static size_t live, peak;
static jmp_buf failed;
static HPDF_Doc doc;
static FILE *input;
static unsigned char *pixels;
static const char *problem;

static void fail(const char *why)
{
    problem = why;
    longjmp(failed, 1);
}

static void *allocate(HPDF_UINT n)
{
    Allocation *p;

    if (n > PDFJOB_MAX_MEMORY - live)
        return NULL;
    p = malloc(sizeof(*p) + n);
    if (!p)
        return NULL;
    p->size = n;
    live += n;
    if (live > peak)
        peak = live;

    return p + 1;
}

static void release(void *v)
{
    if (v) {
        Allocation *p = (Allocation *)v - 1;

        live -= p->size;
        free(p);
    }
}

static void error(HPDF_STATUS code, HPDF_STATUS detail, void *data)
{
    fprintf(stderr, "PDFRender: libHaru error %lx/%lx\n",
            (unsigned long)code, (unsigned long)detail);
    fail("PDF generation failed");
}

static unsigned get_byte(void)
{
    int c = fgetc(input);

    if (c == EOF)
        fail("truncated job");

    return c;
}

static uint32_t get_word(void)
{
    uint32_t n = 0;
    int i;

    for (i = 0; i < 4; i++)
        n = (n << 8) | get_byte();

    return n;
}

static float get_point(void)
{
    uint32_t n = get_word();

    if (n > 1440000)
        fail("coordinate out of range");

    return n / 100.0f;
}

/* A run of characters on one line in one size and style, drawn as one string */
static struct {
    uint32_t x, y, size, style, step;
    unsigned count;
    char text[256];
} run;

static void flush_run(HPDF_Page page, float ph)
{
    static const char *fonts[] = {
        "Courier", "Courier-Bold", "Courier-Oblique", "Courier-BoldOblique"
    };
    float x = run.x / 100.0f, y = run.y / 100.0f, size = run.size / 100.0f;
    float step = run.count > 1 ? run.step / 100.0f : size * 0.6f;

    if (!run.count)
        return;
    run.text[run.count] = 0;
    HPDF_Page_BeginText(page);
    HPDF_Page_SetFontAndSize(page,
        HPDF_GetFont(doc, fonts[run.style & 3], "WinAnsiEncoding"), size);
    /* Courier advances 0.6 em; the driver's pitch may differ by a fraction */
    HPDF_Page_SetCharSpace(page, step - size * 0.6f);
    HPDF_Page_TextOut(page, x, ph - y, run.text);
    HPDF_Page_EndText(page);
    if (run.style & 4) {
        HPDF_Page_SetLineWidth(page, 0.5f);
        HPDF_Page_MoveTo(page, x, ph - y - 1);
        HPDF_Page_LineTo(page, x + step * run.count, ph - y - 1);
        HPDF_Page_Stroke(page);
    }
    run.count = 0;
}

static void render(void)
{
    HPDF_Page page = NULL;
    unsigned pages = 0;
    float ph = 0;

    for (;;) {
        unsigned op = get_byte();

        if (op != 'T' && page)
            flush_run(page, ph);

        if (op == 'P') {
            float pw = get_point();

            ph = get_point();
            if (page || pw < 3 || ph < 3)
                fail("invalid page");
            pages++;
            page = HPDF_AddPage(doc);
            HPDF_Page_SetWidth(page, pw);
            HPDF_Page_SetHeight(page, ph);
        } else if (op == 'T') {
            uint32_t x, y, size, style;
            unsigned c;

            x = get_word();
            y = get_word();
            size = get_word();
            style = get_word();
            c = get_byte();
            if (!page || !c || x > 1440000 || y > 1440000 || size < 100 || size > 20000 || style > 7)
                fail("invalid text");
            /* the next character of the run: same line, size and style, one step on */
            if (run.count && (y != run.y || size != run.size || style != run.style ||
                run.count == sizeof(run.text) - 1 || x <= run.x ||
                (run.count > 1 && x != run.x + run.step * run.count) ||
                (run.count == 1 && (x - run.x) * 5 > size * 4)))
                flush_run(page, ph);
            if (!run.count) {
                run.x = x;
                run.y = y;
                run.size = size;
                run.style = style;
            } else if (run.count == 1)
                run.step = x - run.x;
            run.text[run.count++] = c;
        } else if (op == 'I') {
            float x = get_point();
            float y = get_point();
            float w = get_point();
            float h = get_point();
            uint32_t iw = get_word();
            uint32_t ih = get_word();
            uint32_t channels = get_word();
            HPDF_Image image;
            size_t count;

            if (!page || !iw || !ih || (channels != 1 && channels != 3) ||
                iw > PDFJOB_MAX_IMAGE / channels ||
                ih > PDFJOB_MAX_IMAGE / (iw * channels) || w <= 0 || h <= 0)
                fail("invalid image");
            count = (size_t)iw * ih * channels;
            pixels = malloc(count);
            if (!pixels)
                fail("image allocation failed");
            if (fread(pixels, 1, count, input) != count)
                fail("truncated image");
            image = HPDF_LoadRawImageFromMem(doc, pixels, iw, ih,
                channels == 3 ? HPDF_CS_DEVICE_RGB : HPDF_CS_DEVICE_GRAY, 8);
            free(pixels);
            pixels = NULL;
            HPDF_Page_DrawImage(page, image, x, ph - y - h, w, h);
        } else if (op == 'E') {
            if (!page)
                fail("page end outside page");
            page = NULL;
        } else if (op == 'Z') {
            if (page || !pages || fgetc(input) != EOF || ferror(input))
                fail("invalid document end");
            break;
        } else
            fail("unknown record");
    }
}

int main(void)
{
    enum { ARG_JOB, ARG_PDF, ARG_STATS, ARG_COUNT };
    IPTR args[ARG_COUNT] = { 0 };
    struct RDArgs *rda;
    volatile int rc = 20;
    const char *job, *pdf;
    char magic[8];
    long size;

    rda = ReadArgs("JOB/A,PDF/A,STATS/S", args, NULL);
    if (!rda) {
        PrintFault(IoErr(), "PDFRender");
        return 20;
    }
    job = (const char *)args[ARG_JOB];
    pdf = (const char *)args[ARG_PDF];
    if (!strcmp(job, pdf)) {
        FreeArgs(rda);
        return 20;
    }
    input = fopen(job, "rb");
    if (!input) {
        perror("PDFRender");
        FreeArgs(rda);
        return 20;
    }

    if (setjmp(failed))
        goto done;
    if (fseek(input, 0, SEEK_END) || (size = ftell(input)) < 0 ||
        (unsigned long)size > PDFJOB_MAX_SPOOL || fseek(input, 0, SEEK_SET))
        fail("invalid job size");
    if (fread(magic, 1, 8, input) != 8 || memcmp(magic, PDFJOB_MAGIC, 8))
        fail("invalid job header");

    doc = HPDF_NewEx(error, allocate, release, 0, NULL);
    if (!doc)
        fail("out of memory");
    HPDF_SetCompressionMode(doc, HPDF_COMP_ALL);
    HPDF_SetInfoAttr(doc, HPDF_INFO_CREATOR, "AROS PDF printer driver");
    render();
    if (HPDF_SaveToFile(doc, pdf) != HPDF_OK)
        fail("cannot save PDF");
    rc = 0;

done:
    free(pixels);
    if (doc)
        HPDF_Free(doc);
    fclose(input);
    if (rc) {
        remove(pdf);
        fprintf(stderr, "PDFRender: %s\n", problem ? problem : "failure");
    }
    if (args[ARG_STATS])
        fprintf(stderr, "PDFRender: peak libHaru allocation %lu bytes\n", (unsigned long)peak);
    FreeArgs(rda);

    return rc;
}
