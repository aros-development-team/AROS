* define one of the following in your asm command line
* to produce the desired startup

	IFD ASTARTUP
*------ astartup.i
ASTART    SET   1
WINDOW    SET   0
XNIL      SET   0
NARGS     SET   0
DEBUG     SET   0
QARG      SET   0
	ENDC

	IFD AWSTARTUP
*------ awstartup.i
ASTART    SET   1
WINDOW    SET   1
XNIL      SET   0
NARGS     SET   0
DEBUG     SET   0
QARG      SET   0
	ENDC

	IFD RSTARTUP
*------ rstartup.i
ASTART    SET   0
WINDOW    SET   0
XNIL      SET   0
NARGS     SET   0
DEBUG     SET   0
QARG      SET   0
	ENDC

	IFD RWSTARTUP
*------ rwstartup.i
ASTART    SET   0
WINDOW    SET   1
XNIL      SET   0
NARGS     SET   0
DEBUG     SET   0
QARG      SET   0
	ENDC

	IFD	RXSTARTUP
*------ rxstartup.i

ASTART    SET   0
WINDOW    SET   0
XNIL      SET   1
NARGS     SET   0
DEBUG     SET   0
QARG      SET   0
	ENDC

	IFD QSTARTUP
*------ qstartup.i
ASTART    SET   0
WINDOW    SET   0
XNIL      SET   1
NARGS     SET   0
DEBUG     SET   0
QARG      SET   1
	ENDC
