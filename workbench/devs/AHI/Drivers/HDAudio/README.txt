HDAudio driver for AROS
======================

Thanks to
---------

This driver was originally by Davy Wentzler and paid for by Stephen Jones
from Cluster UK Software Ltd (http://www.clusteruk.com).
In order to open-source it, many people donated to the HDaudio bounty.

Installation
------------
Please copy the file devs/AHI/hdaudio.audio to devs:AHI and
devs/AudioModes/HDAUDIO to devs:AudioModes.
Then copy the hdaudio.config file to ENVARC: and reboot.

Start AHI prefs and look for an entry called 'hdaudio'. If present,
select this one for all units and make sure the volume is at 0 dB.
Press the 'Play a test sound' button to see if the driver works.

Don't forget to donate to ClusterUK if you haven't done so already!


6.14: * Improved detection of speaker node
      * Added headphone detection: when a speaker and headphone are found,
        the speaker will be muted when the headphone is connected.
      * Thanks to donations from Gero Birkenfeld (aka gerograph)!


Troubleshooting
---------------
If the entry 'hdaudio' does not show up in AHI prefs, start PCITool from
AROS:Tools and look up the device which has a description of 'Multimedia' or
'High Definition Audio' in the Product Name. Note the VendorID and ProductID.
Make sure these 2 ID's are present in ENVARC:hdaudio.config. If not, add this
entry.
For example, PCITool says that VendorID = 0x10de and ProductID = 0x044a, add
a new line in hdaudio.config with
0x10de, 0x044a
Pay attention to the comma and space, it must be present.
Reboot and try again.


If the 'hdaudio' entry still does not show up, or you don't hear sound, please
add an entry called 'QUERY' to hdaudio.config, for example:

QUERY
0x8086, 0x2668
0x8086, 0x269A
0x8086, 0x27D8
etc...
Reboot and try again.


If this all fails, please do the following:
Change QUERY to QUERYD in the config file and save it.
Reboot
Start sashimi from AROS:Tools/Debug
Make the sashimi window as tall as possible
Start AHI prefs
Select the 'hdaudio' entry.
If present, press the 'Play a test sound' button
Grab the sashimi output and send it to drivers@audio-evolution.com.

