/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */

#include <aros/libcall.h>
#include <aros/symbolsets.h>
#include <exec/libraries.h>
#include <exec/types.h>
#include <proto/exec.h>

#include <string.h>

#include <libraries/aros_resource.h>
#include <libraries/speechcore.h>
#include "translator_intern.h"

#include LC_LIBDEFS_FILE

#define RESOURCE_PATH "DEVS:Speech/speech.iff"

/*
 * Initialize the library's translation resources.
 *
 * Attempt to open dos.library, clear the resource descriptor, and load the
 * optional speech resource from DEVS:Speech/speech.iff into the library base.
 * Translate() uses the compiled rules if no resource translator is available.
 *
 * Resource loading may allocate memory and perform file I/O. This initializer
 * returns TRUE regardless of whether the DOS library or speech resource was
 * opened successfully; it does not report initialization errors.
 */
static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR base)
{
    base->tb_DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 0);
    memset(&base->tb_Resource, 0, sizeof(base->tb_Resource));
    base->tb_ResourceMemory = LoadSpeechResource(base->tb_DOSBase,
                                                    RESOURCE_PATH,
                                                    &base->tb_Resource);
    return TRUE;
}

ADD2INITLIB(GM_UNIQUENAME(Init), 0)

/*****************************************************************************

    NAME */
        AROS_LH4(LONG, Translate,

/*  SYNOPSIS */
            AROS_LHA(CONST_STRPTR, inputString, A0),
            AROS_LHA(LONG, inputLength, D0),
            AROS_LHA(STRPTR, outputBuffer, A1),
            AROS_LHA(LONG, outputLength, D1),

/*  LOCATION */
            struct TranslatorBase *, TranslatorBase, 5, Translator)
/*  FUNCTION
        Convert a bounded byte string to phoneme text using the translator's
        letter-to-sound rules. This call produces text, not audio. The default
        tables implement English letter-to-sound conversion; a loaded speech
        resource may supply different translation tables.

    INPUTS
        inputString - Readable input bytes. May be NULL only when inputLength
            is zero. Input and output storage must not overlap.
        inputLength - Nonnegative maximum number of input bytes to examine.
            A NUL byte ends translation early; no terminator is required when
            the supplied length bounds the input. This is a byte count, not a
            Unicode character count.
        outputBuffer - Non-NULL writable output storage owned by the caller.
        outputLength - Positive capacity in bytes, including the terminating
            NUL. Supply the actual accessible capacity.

    RESULT
        Zero on reported completion. A negative value reports rejected
        arguments, an overlong internal word or detected output exhaustion.
        Invalid arguments return -1 without modifying the output buffer.

        After valid arguments reach the translation core, the output is NUL-
        terminated, including on a reported translation failure. Failure may
        leave partial phoneme text. A core failure returns the negative count
        of input bytes not yet read, or -1 if none remain. This is not a count
        of untranslated characters or a reliable offset for resuming input.

        No required output size, bytes-written count or distinct error code
        for each failure class is returned. errno and IoErr() are not set by
        this call. See BUGS: zero does not guarantee complete stress marking.

    NOTES
        Include <proto/translator.h> and keep translator.library open while
        calling. The library attempts to load DEVS:Speech/speech.iff during
        initialization. If no translator table is available, it uses compiled
        defaults. Translate() performs no resource reload or file I/O itself.

        ASCII lowercase is converted to uppercase before rule matching.
        Bytes below 0x20, bytes at or above 0x80, DEL and ']' become spaces;
        NUL additionally ends input. This is not UTF-8 decoding or a locale-
        dependent text conversion. Two consecutive '#' bytes participate in
        the core's end-marker handling and can stop translation before the
        end of the supplied input. Do not use this interface to preserve
        arbitrary binary data or non-ASCII spelling.

        Rule output can include punctuation, spaces and stress digits. The
        default conversion of "hello world" with ample output capacity is
        "/HEH4LOW WER4LD ", including its trailing space. Output length need
        not match input length. An empty input produces an empty string.

        The internal word-fill limit is 100 bytes, including the scanner's
        boundary/look-ahead behavior. Long unbroken input can fail even with
        ample output capacity. Increasing outputLength cannot repair that
        condition. Split long text at suitable text boundaries, not at an
        assumed position derived from a negative return value.

        The call uses per-call state and caller-owned output; it retains
        neither input nor output. Do not modify either buffer during the
        call. The translation core performs no dynamic allocation, exposes
        no cancellation callback and supplies no timeout. Resource loading
        during OpenLibrary() has a separate allocation and I/O lifetime.

    BUGS
        Output exhaustion during final stress insertion is not always
        propagated to the result. With the compiled default tables, "hello"
        and an outputLength of 9 or 10 can return zero with "/HEHLOW "; an
        outputLength of 11 produces "/HEH4LOW ". Merely fitting the final
        visible text plus NUL is not a reliable completeness test. The core
        must check the final overflow state before promising complete output.

        Read-ahead means a negative result cannot safely identify a source
        suffix whose translation can be appended to the partial output.
        Retry from the original input with adequate capacity after a capacity
        failure; distinguish long-word failures separately in the caller.

    SEE ALSO
        OpenLibrary(), CloseLibrary()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (inputLength < 0 || outputLength <= 0 || outputBuffer == NULL ||
        (inputString == NULL && inputLength != 0))
        return -1;

    return SCTranslateWith(TranslatorBase->tb_Resource.translator != NULL
            ? TranslatorBase->tb_Resource.translator : &SCDefaultTranslator,
        inputString, (size_t)inputLength,
        outputBuffer, (size_t)outputLength);

    AROS_LIBFUNC_EXIT
}
