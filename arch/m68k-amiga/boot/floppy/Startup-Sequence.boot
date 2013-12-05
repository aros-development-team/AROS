Arch/amiga/AROSBootstrap ROM Arch/amiga/aros.hunk.gz

MakeDir RAM:ENV
MakeDir RAM:T
MakeDir RAM:Clipboards
Assign  ENV:     RAM:ENV
Assign  T:       RAM:T
Assign  CLIPS:   RAM:Clipboards

Mount >NIL: DEVS:DOSDrivers/~(#?.info)

If EXISTS "AROS Live CD:"
  Assign LIBS: "AROS Live CD:Libs" ADD
  Assign LIBS: "AROS Live CD:Classes" ADD
  Assign C: "AROS Live CD:C" ADD
  Assign IMAGES: "AROS Live CD:System/Images" DEFER
  Assign LOCALE: "AROS Live CD:Locale"
  Assign Fonts:  "AROS Live CD:Fonts"
  Assign HELP:   LOCALE:Help DEFER
EndIf

LoadWB
EndCLI
