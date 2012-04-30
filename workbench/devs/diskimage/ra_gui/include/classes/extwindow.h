/* Copyright 2007-2012 Fredrik Wikstrom. All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
**
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS `AS IS'
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
** LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
** INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
** CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
** ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
** POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef CLASSES_EXTWINDOW_H
#define CLASSES_EXTWINDOW_H

#ifndef CLASSES_WINDOW_H
#include <classes/window.h>
#endif

#define EXTWINDOW_Dummy          (0x80025000L)
#define EXTWINDOW_SnapshotGadget (EXTWINDOW_Dummy + 1)
#define EXTWINDOW_VertProp       (EXTWINDOW_Dummy + 2)
#define EXTWINDOW_HorizProp      (EXTWINDOW_Dummy + 3)
#define EXTWINDOW_VertObject     (EXTWINDOW_Dummy + 4)
#define EXTWINDOW_HorizObject    (EXTWINDOW_Dummy + 5)

#define WMHI_SNAPSHOT            (256UL << 16)

extern Class *ExtWindowClass;

#if defined(__amigaos4__) && !defined(__USE_INLINE__)
#define ExtWindowObject IIntuition->NewObject(ExtWindowClass, NULL
#else
#define ExtWindowObject NewObject(ExtWindowClass, NULL
#endif

Class *InitExtWindowClass (void);

#endif
