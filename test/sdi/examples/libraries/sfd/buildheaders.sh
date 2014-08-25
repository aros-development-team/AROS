mkdir -p ../include/proto
mkdir -p ../include/clib
mkdir -p ../include/defines

sfdc example --mode proto  --target x-aros --output ../include/proto/example.h
sfdc example --mode clib   --target x-aros --output ../include/clib/example_protos.h
sfdc example --mode macros --target x-aros --output ../include/defines/example.h
