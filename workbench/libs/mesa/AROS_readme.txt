1. Function prototypes

The prototypes in aros_libapi.c and respective entries in
arosmesa.conf/mangle_undef.h are generated AUTOMATIC from gl.h/glext.h
found in release 7.8.1 of MESA.

The codes of the autogeneration tool (C#) are attached.

When updating to newer version please do the following:

a. generate new aros_libapi.c/arosmesa.conf/mangle_undef.files
b. replace the existing files with generated files copying the headers from
   existing files. Be sure to update the version of mesa.library.

NOTE: The tool that generetes the above mentioned files is implemented so that
      it keeps the previous order of functions in the files. Be sure however
      to check (with diff) that all new functions have been added at the end
      and no functions have been removed/reordered. Changing the order of
      existing functions will change the LVOs and break compatibility

2. Solution for global context

Mesa uses a global variable to store current rendering context. This
leads to a problem when Mesa is a shared library as this context should
be different for each opener or Mesa.

The implemented solution uses peropener Mesa library libbase. This libbase
contains a pointer to current opener Mesa rendering context.

For each glXXX public function a stub is generated in aros_libapi.c. This
stub takes a form of AROS library function which gets the Mesa libbase
passed as the last parameter. Once this stub is called, it puts the Mesa
libbase into the EBX register and calls the real glXXX function - which due
to name mangling is now named mglXXX. The EBX register is reserved via a
global variable REGMesaBase (glapi.h) and via -ffixed-ebx compile option.
The Mesa libbase is put into EBX via PUT_MESABASE_IN_REG macro (glapi.h)
while the original EBX value is preserved using SAVE_REG/RESTORE_REG macro
pair. The real function reads the Mesa 'global' rendering context via
GET_CURRENT_CONTEXT macro which is redefined in glapi.h.

The call sequence is as follows:

Client calls glA() functions. This is a stub in linklib.
The glA() function calls Mesa_glA() functions. This is a shared function
   in mesa.library which now receives opener Mesa libbase.
The Mesa_glA() function saves current EBX value
The Mesa_glA() function puts received Mesa libbase into EBX
The Mesa_glA() function calls mglA() function.
The Mesa_glA() function restores the previous EBX value

Note: this is temporary solution until ABI V1 is available. Once this
happens, ABI V1 mechanisms should be used for handling the global context

3. AROSMesaGetProcAddress

This function can return a pointer to gl function by specifying name as
string. This function is compiled direclty into the mesa shared library
linklib and not found in the shared library itself. This is due to a fact
that the returned pointer must point to a stub gl function not its
implementation in shared library, because in the later case, the Mesa 
libbase would not be filled correclty. Using the example above, calling

AROSMesaGetProcAddress("glA") must return the pointer to glA, and not
to Mesa_glA or mglA function.

4. Upgrading to new version of Mesa3D

When upgrading to new version of Mesa3D several tasks need to be performed.

a) regenerate library interface (see point 1.)
b) regenerate files in /mesa/main - the following files
   need to be regenerated:
    - api_exec_es1.c
    - api_exec_es2.c
   To perform this action, check the end of mmakefile.src for mesa. Suitable
   commands are available there.
c) "implement" the missing dispatch stubs
   Go to /mesa/src/mesa/glapi/glprocs.h and find block of dispatch stubs.
   Example stub:
   
   void GLAPIENTRY gl_dispatch_stub_343(GLenum target, GLenum format, GLenum type, GLvoid * table);
   
   Copy these stubs to /mesa/src/mesa/arosmesa_getprocaddress.c overwritting
   the existing ones and "implement" them by replacing the ";" at the end of
   the line with "{};"

   
