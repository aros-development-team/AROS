boot/amiga/AROSBootstrap ROM boot/amiga/aros.hunk.gz

MakeDir RAM:ENV
MakeDir RAM:T
MakeDir RAM:Clipboards
Assign  ENV:     RAM:ENV
Assign  T:       RAM:T
Assign  CLIPS:   RAM:Clipboards

Mount >NIL: DEVS:DOSDrivers/~(#?.info)

If EXISTS "AROS Live CD:"
  Assign SYS: "AROS Live CD:"
  Assign LIBS: SYS:Libs
  Assign LIBS: SYS:Classes ADD
  Assign IMAGES: SYS:System/Images DEFER
  Assign LOCALE: SYS:Locale
  Assign Fonts:  SYS:Fonts
  Assign HELP:   LOCALE:Help DEFER
  Assign C: SYS:C
EndIf

LoadWB
EndCLI
