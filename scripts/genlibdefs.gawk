/name/ { name=tolower($2); }
/version/ {
    str=$2;
    n=split(str,a,".");
    version=a[1];
    revision=a[2];
}
END {
    Name=toupper(substr(name,1,1)) substr(name,2);

    cmd="date \"+%d.%m.%Y\"";
    cmd | getline date;

    print "#define LIBNAME      \""name".library\""
    print "#define LIBBASE      "Name"Base"
    print "#define LIBBASETYPE  "Name"Base"
    print "#define LIBVERSION   "version
    print "#define LIBREVISION  "revision
    print "#define _LIBnAME     "Name
    print "#define _LIBNAME     "name
    print "#define VERSION      \"$VER: "name" "version"."revision" ("date")\\n\\r\""
    print "#define END          "Name"_end"
    print "#define FUNCTABLE    "Name"_functable"
    print "#define INIT         AROS_SLIB_ENTRY(init,"Name")"
}
