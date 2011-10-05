1. Solution for global context

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

2. AROSMesaGetProcAddress

This function can return a pointer to gl function by specifying name as
string. This function is compiled direclty into the mesa shared library
linklib and not found in the shared library itself. This is due to a fact
that the returned pointer must point to a stub gl function not its
implementation in shared library, because in the later case, the Mesa 
libbase would not be filled correclty. Using the example above, calling

AROSMesaGetProcAddress("glA") must return the pointer to glA, and not
to Mesa_glA or mglA function.

