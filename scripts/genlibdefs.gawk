#
# Create libdefs.h from a file lib.conf. lib.conf may contain these options:
#
# name <string> - Init the various fields with reasonable defaults. If
#		<string> is XXX, then this is the result:
#
#		    libname	xxx
#		    basename	Xxx
#		    libbase	XxxBase
#		    libbasetype XxxBase
#
#		Variables will only be changed if they have not yet been
#		specified.
#
# libname <string> - Set libname to <string>. This is the name of the
#		library (ie. you can open it with <string>.library).
#		It will show up in the version string, too.
# basename <string> - Set basename to <string>. The basename is used in
#		the AROS-LHx macros in the location part (last parameter)
#		and to specify defaults for libbase and libbasetype
#		in case they have no value yet. If <string> is xXx, then
#		libbase will become xXxBase and libbasetype will become
#		xXxBase.
# libbase <string> - Defines the name of the library base (ie. SysBase,
#		DOSBase, IconBase, etc). If libbasetype is not set, then
#		it is set to <string>, too.
# libbasetype <string> - The type of libbase (without struct), ie.
#		ExecBase, DosLibrary, IconBase, etc).
# version <version>.<revision> - Specifies the version and revision of the
#		library. 41.0103 means version 41 and revision 103.
#
BEGIN {
    libbase="";
    libbasetype="";
    basename="";
}
/name/ {
    libname=tolower($2);
    if (basename=="")
	basename=toupper(substr(libname,1,1)) substr(libname,2);
    if (libbase=="")
	libbase=basename"Base";
    if (libbasetype=="")
	libbasetype=basename"Base";
}
/libname/ { libname=$2; }
/basename/ {
    basename=$2;
    if (libbase=="")
	libbase=basename"Base";
    if (libbasetype=="")
	libbasetype=basename"Base";
}
/libbase[ \t]+/ {
    libbase=$2;
    if (libbasetype=="")
	libbasetype=libbase;
}
/libbasetype/ {
    libbasetype=$2;
}
/version/ {
    str=$2;
    n=split(str,a,".");
    version=int(a[1]);
    revision=int(a[2]);
}
END {
    cmd="date \"+%d.%m.%Y\"";
    cmd | getline date;

    if (libbase=="")
	libbase=basename"Base";
    if (libbasetype=="")
	libbasetype=basename"Base";

    print "#define LIBNAME      \""libname".library\""
    print "#define LIBBASE      "libbase
    print "#define LIBBASETYPE  "libbasetype
    print "#define LIBVERSION   "version
    print "#define LIBREVISION  "revision
    print "#define BASENAME     "basename
    print "#define VERSION      \"$VER: "libname" "version"."revision" ("date")\\n\\r\""
    print "#define END          "basename"_end"
    print "#define FUNCTABLE    "basename"_functable"
    print "#define INIT         AROS_SLIB_ENTRY(init,"basename")"
}
