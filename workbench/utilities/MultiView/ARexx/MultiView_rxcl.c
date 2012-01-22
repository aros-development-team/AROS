/*
 * Source generated with ARexxBox 1.12 (May 18 1993)
 * which is Copyright (c) 1992,1993 Michael Balzer
 */

#include <exec/types.h>
#include <dos/dos.h>
#include <rexx/storage.h>

#define NO_GLOBALS
#include "MultiView.h"

#define RESINDEX(stype) (((long) &((struct stype *)0)->res) / sizeof(long))

char    RexxPortBaseName[80] = "MULTIVIEW";
char    *rexx_extension = "MULTIVIEW";

struct rxs_command rxs_commandlist[] =
{
        { "ABOUT", NULL, NULL, 0, (void (*)(struct RexxHost *,void **,long,struct RexxMsg *)) rx_about, 1 },
        { "ACTIVATEWINDOW", NULL, NULL, 0, (void (*)(struct RexxHost *,void **,long,struct RexxMsg *)) rx_activatewindow, 1 },
        { "BEEPSCREEN", NULL, NULL, 0, (void (*)(struct RexxHost *,void **,long,struct RexxMsg *)) rx_beepscreen, 1 },
        { "CLEARSELECTED", NULL, NULL, 0, (void (*)(struct RexxHost *,void **,long,struct RexxMsg *)) rx_clearselected, 1 },
        { "COPY", NULL, NULL, 0, (void (*)(struct RexxHost *,void **,long,struct RexxMsg *)) rx_copy, 1 },
        { "DOTRIGGERMETHOD", "METHOD/A", NULL, 0, (void (*)(struct RexxHost *,void **,long,struct RexxMsg *)) rx_dotriggermethod, 1 },
        { "GETCURRENTDIR", NULL, "CURRENTDIR", RESINDEX(rxd_getcurrentdir), (void (*)(struct RexxHost *,void **,long,struct RexxMsg *)) rx_getcurrentdir, 1 },
        { "GETFILEINFO", NULL, "FILEINFO", RESINDEX(rxd_getfileinfo), (void (*)(struct RexxHost *,void **,long,struct RexxMsg *)) rx_getfileinfo, 1 },
        { "GETOBJECTINFO", "MYVAR/S,MYSTEM/K", "RESULT", RESINDEX(rxd_getobjectinfo), (void (*)(struct RexxHost *,void **,long,struct RexxMsg *)) rx_getobjectinfo, 1 },
        { "GETTRIGGERINFO", "MYVAR/S,MYSTEM/K", "RESULT", RESINDEX(rxd_gettriggerinfo), (void (*)(struct RexxHost *,void **,long,struct RexxMsg *)) rx_gettriggerinfo, 1 },
        { "MARK", NULL, NULL, 0, (void (*)(struct RexxHost *,void **,long,struct RexxMsg *)) rx_mark, 1 },
        { "MAXIMUMSIZE", NULL, NULL, 0, (void (*)(struct RexxHost *,void **,long,struct RexxMsg *)) rx_maximumsize, 1 },
        { "MINIMUMSIZE", NULL, NULL, 0, (void (*)(struct RexxHost *,void **,long,struct RexxMsg *)) rx_minimumsize, 1 },
        { "NORMALSIZE", NULL, NULL, 0, (void (*)(struct RexxHost *,void **,long,struct RexxMsg *)) rx_normalsize, 1 },
        { "OPEN", "NAME/K,CLIPBOARD/S,CLIPUNIT/K/N", NULL, 0, (void (*)(struct RexxHost *,void **,long,struct RexxMsg *)) rx_open, 1 },
        { "PRINT", NULL, NULL, 0, (void (*)(struct RexxHost *,void **,long,struct RexxMsg *)) rx_print, 1 },
        { "PUBSCREEN", "NAME/A", NULL, 0, (void (*)(struct RexxHost *,void **,long,struct RexxMsg *)) rx_pubscreen, 1 },
        { "QUIT", NULL, NULL, 0, (void (*)(struct RexxHost *,void **,long,struct RexxMsg *)) rx_quit, 1 },
        { "RELOAD", NULL, NULL, 0, (void (*)(struct RexxHost *,void **,long,struct RexxMsg *)) rx_reload, 1 },
        { "SAVEAS", "NAME/K,IFF/S", NULL, 0, (void (*)(struct RexxHost *,void **,long,struct RexxMsg *)) rx_saveas, 1 },
        { "SCREEN", "TRUE/S,FALSE/S", NULL, 0, (void (*)(struct RexxHost *,void **,long,struct RexxMsg *)) rx_screen, 1 },
        { "SCREENTOBACK", NULL, NULL, 0, (void (*)(struct RexxHost *,void **,long,struct RexxMsg *)) rx_screentoback, 1 },
        { "SCREENTOFRONT", NULL, NULL, 0, (void (*)(struct RexxHost *,void **,long,struct RexxMsg *)) rx_screentofront, 1 },
        { "SNAPSHOT", NULL, NULL, 0, (void (*)(struct RexxHost *,void **,long,struct RexxMsg *)) rx_snapshot, 1 },
        { "WINDOWTOBACK", NULL, NULL, 0, (void (*)(struct RexxHost *,void **,long,struct RexxMsg *)) rx_windowtoback, 1 },
        { "WINDOWTOFRONT", NULL, NULL, 0, (void (*)(struct RexxHost *,void **,long,struct RexxMsg *)) rx_windowtofront, 1 },
        { NULL, NULL, NULL, NULL, NULL }
};

int             command_cnt = 26;

static struct arb_p_link link0[] = {
        {"WINDOWTO", 1}, {"S", 4}, {"RELOAD", 11}, {"QUIT", 12}, {"P", 13}, {"OPEN", 16},
        {"NORMALSIZE", 17}, {"M", 18}, {"GET", 23}, {"DOTRIGGERMETHOD", 28}, {"C", 29}, {"BEEPSCREEN", 32},
        {"A", 33}, {NULL, 0} };

static struct arb_p_link link1[] = {
        {"FRONT", 2}, {"BACK", 3}, {NULL, 0} };

static struct arb_p_link link4[] = {
        {"NAPSHOT", 5}, {"CREEN", 6}, {"AVEAS", 10}, {NULL, 0} };

static struct arb_p_link link6[] = {
        {"TO", 7}, {NULL, 0} };

static struct arb_p_link link7[] = {
        {"FRONT", 8}, {"BACK", 9}, {NULL, 0} };

static struct arb_p_link link13[] = {
        {"UBSCREEN", 14}, {"RINT", 15}, {NULL, 0} };

static struct arb_p_link link18[] = {
        {"INIMUMSIZE", 19}, {"A", 20}, {NULL, 0} };

static struct arb_p_link link20[] = {
        {"XIMUMSIZE", 21}, {"RK", 22}, {NULL, 0} };

static struct arb_p_link link23[] = {
        {"TRIGGERINFO", 24}, {"OBJECTINFO", 25}, {"FILEINFO", 26}, {"CURRENTDIR", 27}, {NULL, 0} };

static struct arb_p_link link29[] = {
        {"OPY", 30}, {"LEARSELECTED", 31}, {NULL, 0} };

static struct arb_p_link link33[] = {
        {"CTIVATEWINDOW", 34}, {"BOUT", 35}, {NULL, 0} };

struct arb_p_state arb_p_state[] = {
        {-1, link0}, {24, link1}, {25, NULL}, {24, NULL}, {19, link4},
        {23, NULL}, {20, link6}, {21, link7}, {22, NULL}, {21, NULL},
        {19, NULL}, {18, NULL}, {17, NULL}, {15, link13}, {16, NULL},
        {15, NULL}, {14, NULL}, {13, NULL}, {10, link18}, {12, NULL},
        {10, link20}, {11, NULL}, {10, NULL}, {6, link23}, {9, NULL},
        {8, NULL}, {7, NULL}, {6, NULL}, {5, NULL}, {3, link29},
        {4, NULL}, {3, NULL}, {2, NULL}, {0, link33}, {1, NULL},
        {0, NULL}  };

