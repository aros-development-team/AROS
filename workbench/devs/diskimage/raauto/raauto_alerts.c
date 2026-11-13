#include <proto/dos.h>
#include <proto/intuition.h>
#include <stdio.h>
#include "raauto_alerts.h"

void RAAutoClassNotFound (CONST_STRPTR class_name) {
	TEXT alert_message[256];
	snprintf(alert_message, sizeof(alert_message), "%c%c%c" "Failed to open class '%s'." "%c%c"
		"%c%c%c" "Make sure that you have a new enough version of ClassAct or ReAction" "%c%c"
		"%c%c%c" "installed on your system." "%c%c",
		0, 16, 16, FilePart(class_name), 0, 1,
		0, 16, 28, 0, 1,
		0, 16, 40, 0, 0);
	DisplayAlert(RECOVERY_ALERT, alert_message, 52);
}
