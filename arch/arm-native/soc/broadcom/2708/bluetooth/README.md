# Raspberry Pi on-board Bluetooth

AROS has a Bluetooth stack (`rom/bluetooth`), two firmware loaders and one
HCI device -- and that device is `vbthci`, which is virtual. On real hardware
the stack has nothing to talk to. These three modules are the missing half for
the Raspberry Pi 3, 3B+ and Zero 2 W, whose radio hangs off the SoC's PL011.

| module | where it lands | what it owns |
|---|---|---|
| `pl011bt.resource` | in the ROM package | the PL011: pin mux, the 32.768 kHz LPO on GPCLK2, `BT_REG_EN`, the baud divisor, the receive interrupt |
| `h4bthci.device` | `DEVS:Bluetooth/` | H:4 framing, and the AROS device the stack opens |
| `brcmbt.fwl` | `DEVS:Bluetooth/FWLoaders/` | the Broadcom patchram upload |

The split is the one the stack already assumes. `BTStackLoader` opens every
`*.fwl` it finds, `AddBTHardware` opens the device, and the device reaches the
wire through the resource. Nothing above H:4 is here.

## The three names

Each says what is specific about it, because all three sit in one namespace
once they are built:

- **`pl011bt`** names the block. The BCM283x has two UARTs and the other one,
  the AUX mini-UART, is where the serial console goes when the firmware hands
  the PL011 to the radio. A resource called `btuart` would not say which.
- **`h4bthci`** names the protocol, not the wire -- H:4 is what it speaks, and
  it is the same shape as upstream's `vbthci`, where `v` is virtual.
- **`brcmbt`** is vendor plus Bluetooth, the shape `rtlbtv1` and `rtlbtv2`
  already use. Not `brcmfw`: on a Pi the SoC *and* the radio are Broadcom, so
  "Broadcom firmware" does not distinguish this from the WiFi blob that
  `bwfm` loads.

## What the resource does and does not touch

Resident initialisation is side-effect free: it finds the peripheral window,
asks the firmware for the UART clock, and stops. The PL011 is not written
until a client claims the resource and calls `PL011BTConfigure()`, so a
machine that never opens Bluetooth is left exactly as the firmware set it up.

It binds by naming the SoCs it has been run on rather than by excluding the
others. The arrangement it drives -- PL011 on GPIO 30-33, console pushed onto
the mini-UART, LPO on GPCLK2, `BT_REG_EN` on the firmware's GPIO expander --
is the BCM2835/6/7 one, and `IRQ_VC_UART` is the legacy interrupt
controller's numbering. A BCM2711 wires its radio the same way but presents
GPU interrupts through the GIC at `BCM2711_GPUIRQ_OFFSET`, and a BCM2712 does
not use this UART for Bluetooth at all. Neither has been tested, so the
resource stands down there: absent is a state `h4bthci.device` already
handles, present and wrong is not.

## Two things that are easy to get wrong here

**The receive path cannot be polled.** The FIFO holds sixteen bytes, which at
115200 baud is 1.4 ms. Polling at 10 ms lost data on every burst and polling
at 1 ms still reported OVERRUN on hardware during an LE scan. The interrupt
owns the FIFO and fills an 8 KB ring; `PL011BTRead()` drains it. The ring is
sized against the traffic, not against a tick: a BCM43430A1 batches every
advert it heard into one HCI event of up to 1220 bytes, and an overflow costs
the framer its synchronisation exactly as a FIFO overrun does.

**Stopping GPCLK2 means clearing `ENAB` and nothing else.** Writing the
password alone clears it, but in the same store drives `SRC` to GND and
`MASH` to 0 -- and the datasheet forbids changing the source or the divisor
while the generator runs. The result is `BUSY` stuck set. It passes wherever
the register is not modelled and `BUSY` reads 0; on a Pi the firmware has
already started the generator for the radio, so the bad write always lands on
a running one.

## Firmware

`brcmbt.fwl` reads its patchram from `DEVS:Firmware/brcm/` at runtime, the
same directory `bwfm` takes the WiFi blob from, so changing controllers is
changing a file. Without a patchram a BCM43438 answers `HCI_Reset` from ROM
and reports the address `AA:AA:AA:AA:AA:AA`, which is not an identity -- the
firmware is what gives it one.

`BTStackLoader` has to run *before* `AddBTHardware`, so the loader is bound
before the radio is brought up and the patchram is applied at the bring-up's
own firmware step. Registering the controller first sends it through the
late-bind path instead, which uploads the patchram, restarts the controller
and then re-runs the bring-up against a chip that is still restarting.
