1. Function prototypes

The prototypes in arosmesa_library_api.c and respective entries in
arosmesa.conf/mangle_undef.h are generated AUTOMATIC from gl.h/glext.h
found in release of MESA.

The codes of the autogeneration tool (C#) are attached.

When updating to newer version please do the following:

a. generate new files:
    - arosmesaapim.h
    - arosmesa_mangle.h
    - arosmesa.conf
    - arosmesa_library_api.c
    - vgapim.h
    - vg_mangle.h
    - vega.conf
    - vega_library_api.c
    - egl_mangle.h
    - eglapim.h
    - egl.conf
    - egl_library_api.c
    - glu_mangle.h
    - gluapim.h
    - glu.conf
    - glu_libraru_api.c
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
be different for each task which uses Mesa.

The implemented solution uses a simple TLS implementation inside Mesa library
to store the context in a "per task" way. The TLS implementation can be found 
in aros/tls.[ch]

The public gl/egl functions are exported from library using call wrapper which
have the proper "library" definitions (for example contains library base).

For each glXXX public function a stub is generated in arosmesa_library_api.c. 
This stub takes a form of AROS library function which gets the Mesa libbase
passed as the last parameter. 

Once this stub is called, it calls the real glXXX function - which due
to name mangling is now named mglXXX. The real function reads the Mesa 'global' 
rendering context via GET_CURRENT_CONTEXT macro which is redefined in glapi.h.
Code behind this macro eventually calls the TLS to get "per task" context (see
mapi/mapi/u_current.c)

The call sequence is as follows:

Client calls glA() functions. This is a stub in linklib.
The glA() function calls Mesa_glA() functions. This is a shared function in 
mesa.library. The Mesa_glA() function calls mglA() function.

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
   
   Copy these stubs to /mesa/src/mesa/aros/arosmesa_getprocaddress.c 
   overwritting the existing ones and "implement" them by replacing the ";" at 
   the end of the line with "{};"
