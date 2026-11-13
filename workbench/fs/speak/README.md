# SPEAK: handler

`SPEAK:` is a write-only DOS device for text-to-speech.

```
Copy S:Startup-Sequence SPEAK:
Echo "Hello" >SPEAK:M/R/S150/P110
Echo "DH1 EH0 L OW1" >SPEAK:A1
```

Classic options are `M`/`F`, `R`/`N`, `S<rate>`, `P<pitch>`, `O0`/`O1`,
`A0`/`A1`, and `D0`/`D1`. `O1` recognizes lines beginning with `OPT/` as
option changes. `A1` treats input as narrator phonemes.

Long options select a speech engine:

```
Echo "Hello" >SPEAK:ENGINE=narrator/VOICE=female/STYLE=natural/LANGUAGE=en-US
```

Supported selectors are `ENGINE=`, `VOICE=`, `STYLE=`, and `LANGUAGE=`.
Values are case-sensitive backend identifiers. `ENGINE=DEFAULT` selects the
system default. Narrator-compatible selectors continue through the classic
translator/narrator pair. Other engines use `speech.device`. Direct phonemes
require a narrator-compatible selection.
