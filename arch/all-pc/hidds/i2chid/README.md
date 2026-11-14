# HID-over-I2C touchpad input

`i2chid.hidd` starts automatically from the PC BSP after DOS initialization.
It finds present ACPI `PNP0C50` / `ACPI0C50` devices by hardware or compatible
ID and uses the first supported device's I2C and GPIO resources. No
User-Startup command or manual LoadResource is required. Remove old
`StartI2CTouchpad` / `I2CTouchpad RUN` startup entries when installing it.

The transport currently supports AMD `AMDI0010` DesignWare I2C with `AMDI0030`
GPIO, 7-bit addresses, firmware-configured 100–400 kHz timing, and active-low
level GPIO polling. The ASUS M1607KA ELAN touchpad is the initial target.
Other controller/GPIO types, PSP arbitration, gestures, scrolling and Precision
Touchpad mode negotiation are not supported. Discovery on other laptops does
not imply their controllers are supported. One pointer device is selected;
hot removal and live HIDD restart are not supported.

## Diagnostics

Build targets: `kernel-hidd-i2chid` and `workbench-c-i2ctouchpad`.

- `I2CTouchpad`: automatic discovery only; no controller writes.
- `I2CTouchpad DEVICE "\_SB.I2CA.TPD0"`: explicit diagnostic device path.
- `I2CTouchpad CHECK`: inspect controller state without register writes.
- `I2CTouchpad RUN`: poll pointer input until Ctrl-C, then attempt restoration.
- `I2CTouchpad RUN REPORT RAM:touchpad-report.bin`: explicitly capture the HID
  report descriptor (overwrites the named file). Redirect console output to
  a file if a diagnostic log is wanted.

The HIDD creates no log or descriptor files at boot. HIDD and diagnostic command
share a named, controller-specific public semaphore; CHECK/RUN reject an already
owned controller. Discovery remains available while the HIDD is active. Reboot
without the HIDD to run hardware diagnostics against its controller. Older
RAM-file-lock versions of the command are incompatible and must not be run
alongside this driver.

Transfers and report parsing are bounded. Firmware AML execution itself has no
guaranteed deadline. Polling releases pressed buttons and attempts controller
restoration on exit; recovery cannot be guaranteed if hardware stops responding.
