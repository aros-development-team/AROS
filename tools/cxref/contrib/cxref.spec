Name        : cxref
Version     : 1.5c
Release     : 2mdk
Group       : Development/C
License     : GPL
Packager    : Matthew L Daniel <mdaniel@scdi.com>
URL         : http://www.gedanken.demon.co.uk/cxref/
Source      : http://www.gedanken.demon.co.uk/cxref/%{name}-%{version}.tgz
BuildRoot   : %{_tmppath}/%{name}-%{version}-root
Summary     : C program cross-referencing & documentation tool
%Description 
A program that takes as input a series of C source files
and produces a LaTeX or HTML document containing a cross
reference of the files/functions/variables in the program,
including documentation taken from suitably formatted
source code comments.
The documentation is stored in the C source file in
specially formatted comments, making it simple to maintain.
The cross referencing includes lists of functions called,
callers of each function, usage of global variables, header
file inclusion, macro definitions and type definitions.
Works for ANSI C, including many gcc extensions.

This compiled against gcc-3.0.1.
%prep
%setup -q
%patch -p0
%build
%configure --enable-us-paper
make
%install
%makeinstall
%clean
if test "/" != "${RPM_BUILD_ROOT}"; then rm -rf ${RPM_BUILD_ROOT}; fi
%files
%defattr(-,root,root)
%doc ANNOUNCE COPYING FAQ FAQ.html FAQ-html.pl FILES INSTALL LSM NEWS TODO
%doc README README.c README.c.html README.c.rtf README.c.sgml README.c.src.html
%doc README_c.tex README.html README.man README.sgml README.tex contrib doc
%{_bindir}
%{_mandir}
