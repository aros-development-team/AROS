
SB128 AHI driver
----------------


This is the open source release of the SB128 AHI driver written by Ross Vumbaca.

The source to this driver is released under the terms of the Aros Public License (APL).

Thank you to Paolo Besser for sponsoring the release of this driver source.

For licensing information, please read the "LICENSE.txt" file.


Background:
-----------


"SB128" is a generic name sometimes used by Creative Labs for their low cost
PCI Sound Cards.


Sometimes they are known by different names, such as:

Sound Blaster AudioPCI97
Sound Blaster PCI
Sound Blaster Vibra
Sound Blaster 128
Sound Blaster 64(V)

These cards are all based around the same "Ensoniq" audio chip
(Creative bought them out a number of years ago).

Creative likes to change the names of their cards.


The original cards were based on the "Ensoniq ES1370" chip.

The next generations of chips (name changing over time) were:

Ensoniq ES1371
Ensoniq ES1373
Creative CT5880
Ectiva EV1938

They are all basically the same thing, and are all supported by this driver.

Creative likes to change the names of their chips (do we see a trend here?)


The driver supports playback and recording from 8000Hz to 48000Hz at 16 Bit, Stereo.


Thank you to Davy Wentlzer for extensive help and guidance during the development of this driver,
and also for sending me an ES1370 card - http://www.audio-evolution.com

Additionally, thank you to all the AmigaOS4 Beta Testers that provided feedback.


Instructions for use:
---------------------

1) You must install AHI on your system

2) Place "sb128.audio" into Devs:AHI/

3) Place "SB128" into Devs:AudioModes/

4) Make sure to set up Unit 0 using AHI Prefs
   Use a frequency of 44100Hz, and Channels = 32.


Note Regarding ES1370:
----------------------

When you are recording with the ES1370, the Monitor Volume level you hear,
is the level which will be used for recording. That is, if you adjust the Monitor
Volume, you are actually adjusting the Input Gain. It is not possible to adjust
the Input Gain separately (as it is with the other SB128 AC97 based cards).

The exception to this rule, is the Microphone and Speaker-Phone/TAD input
(if present).


Useless Trivia
--------------

Useless Trivia #1274 - Ensoniq was founded by an ex-Commodore employee.
                       He was the inventor of the SID chip.


History
-------

5.10 - Initial Release

5.15 - Implemented proper MicroDelays, for better performance in particular areas.
       Adjusted AC97 read code for the buggy revision 8, ES1373 chip.

5.16 - Got rid of MicroDelays, changed timing approach.

5.17 - Experimental DMA fix added

5.18 - Adjustments made to experimental DMA fix

5.19 - DMA fix finalised

5.20 - Reset handler added.
       Support for ES1370 added!

5.21 - Code clean up

5.22 - Fixed issue with simultaneous playback and
       record

5.23 - Now supports the New Memory System (TM)

5.24 - Open source release, thank you to Paolo Besser


Contacting the author
---------------------

Please send bug reports to rossv@potduck.com

Ross Vumbaca, 18/8/11

