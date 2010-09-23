 This module should offer more portable way of asynchronous file access.
It should have been implemented for both UNIX and Windows and would eventually
replace unixio.hidd.
 It was inspired by an attempt to port tap.device to Windows using TUN/TAP
driver from OpenVPN project. However currently other ways of implementing
network support on Windows-hosted AROS are under consideration, and this
component is actually not needed.
 Because of this i disabled copying its includes into the SDK. Two changed places
in the mmakefile.src are marked as "# DISABLED".
 I still keep the source code in case if i ever return to this concept. Perhaps
it will be useful if i ever decide to implement serial and/or parallel port driver
for Windows-hosted AROS. However even in this case such abstraction layer seems
to be overweighted for me.
				Pavel Fedin <pavel_fedin@mail.ru>
