/* Makes the macros VERSION, REVISION, DATE and VERS from a RCS ID */

OPTIONS RESULTS
PARSE PULL "$Id: " . " " ver "." rev y "/" m "/" d .

y = RIGHT(y, 2)

SAY    'VERSION		EQU ' || ver
SAY    'REVISION	EQU ' || rev
SAY    'DATE	MACRO'
SAY    '		dc.b	"' || d || '.' || m || '.' || y || '"'
SAY    '	ENDM'
SAY    'VERS	MACRO'
SAY    '		dc.b	"paula.audio ' || ver || '.' || rev || '"'
SAY    '	ENDM'
