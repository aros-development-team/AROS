# Bluetooth audio class

`btaudio.class` binds to the AudioSink service of registered Bluetooth devices.
Each binding supplies a named, stereo AHI output mode. The class creates the
resident `btaudio.audio` sublibrary; no audio driver or generic mode file is
installed in `Devs:AHI` or `Devs:AudioModes`.

The class owns the service endpoints, AVDTP signalling, SBC encoder and media
transport. AHI supplies mixed PCM to the selected binding. Each sink accepts one
AHI allocation at a time and has its own transport task. The connection can stay
open between sounds; an idle transport session closes after five minutes.

Unbinding removes the mode and drains the transport before releasing its
service and endpoints. Open AHI allocations keep their unit and a silent mixer
clock. Rebinding the same Bluetooth address restores that unit's transport,
including playback already owned by ahi.device. A mode ID collision is refused.

Binding negotiates an SBC format before publishing the mode. The mode advertises
that frequency and AllocAudio sets the mixer to it. A reconnect cannot silently
change the rate underneath an existing allocation. A sink-configured open
transport can wait idle without a setup timeout; playback sends START if the
sink has not started it. Recording is not supported.
The mixer runs at priority 50. A bounded packet queue retains audio across slow
media writes; a mixer buffer larger than the PCM FIFO is consumed in pieces.
Audio deadlines use monotonic uptime and advance by sample count, carrying
fractional microseconds. Catch-up after back-pressure is limited to eight packet
periods; older timing debt is discarded. A silent mixer clock also runs
while a bound transport is broken or reconnecting. Idle bindings check their
mode every 30 seconds; an idle session still closes after five minutes.

AddAudioModes REFRESH keeps its existing file-reload behaviour. While bound,
the class periodically checks AHI_NextAudioID and re-adds its missing mode.
A negotiated rate change without an allocation also republishes the mode. The class opens ahi.device when registering or
removing a binding and reports failures through the Bluetooth error log.

A saved AHI mode cannot be opened before its sink has bound and published the
mode. Applications must retry opening that unit once the sink is available;
this class does not install an unconditional mode or change AHI's device-open
semantics.

Host regression checks run with the existing portable stack suite:
`make -C rom/bluetooth/stack -f Makefile.host test` from the source root.
The suite includes SBC, A2DP/AVDTP, paired L2CAP listeners and the class clock
and open-stream policy checks. Native mixing, binding and audible playback
still require an AROS runtime test.
