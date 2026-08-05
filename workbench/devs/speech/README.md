# Speech

The speech stack provides:

- `translator.library` preserves the classic `Translate()` entry point.
- `narrator.device` preserves the classic phoneme and `mouth_rb` interfaces.
- `speech.device` is the versioned UTF-8 text API for new applications.

The redistributable NRL rules and free voice are compiled in. At module load,
the libraries and devices read an optional narrator-ts resource from:

```text
DEVS:Speech/speech.iff
```

Generate a version 1 `FORM NARR` resource with
[narrator-ts](https://github.com/bitplane/narrator-ts):

```sh
npm run export:aros -- -o speech.iff
npm run export:aros -- -o speech.iff \
  --translator data/translator-33.2.json \
  --voice data/narrator-33.2.json
```

Authentic tables are not distributed with AROS.

## `speech.device` v1

Open unit 0 with `struct IOSpeech`, set `ios_APIVersion` to
`SPEECH_API_VERSION`, put UTF-8 text in the standard `io_Data`/`io_Length`
fields, and issue `CMD_WRITE`.

`SDCMD_QUERY` returns the available engine metadata. Engine, voice, style,
language, rate, pitch, volume, and sample frequency are selected through the
`SPEECHA_*` tag list.

The built-in backend is `narrator`, with `male` and `female` voices and
`natural` and `robotic` styles.
