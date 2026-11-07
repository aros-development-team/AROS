/* SPDX-License-Identifier: MIT */
/* unitext.library: the Unicode text layer of zunekit (zk_text: UTF-8,
 * grapheme clusters, HarfBuzz shaping, FreeType drawing; zk_text_aros:
 * fonts, the system's character set, DOS names) as one shared library,
 * for classes that show and edit UTF-8 (BetterString's and TextEditor's
 * UTF-8 modes). Programs and classes that do not use UTF-8 never open it.
 *
 * The layer is opened once when the library is first loaded and closed
 * when it is expunged; its state is shared by every opener and changes
 * only under the layer's semaphore (zk_text_aros.c). A zt_font belongs to
 * the task that opened it. Buffers the library returns are freed with
 * zt_free. */
#include <aros/symbolsets.h>
#include <proto/exec.h>
#include "zk_text_aros.h"

static int InitFunc(struct Library *lh)
{
    (void)lh;
    return zt_layer_open() == ZT_OK;
}

static int ExpungeFunc(struct Library *lh)
{
    (void)lh;
    zt_layer_close();
    return TRUE;
}

ADD2INITLIB(InitFunc, 0);
ADD2EXPUNGELIB(ExpungeFunc, 0);
