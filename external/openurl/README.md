# OpenURL.library

[![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=RAQSDY9YNZVCL)
[![Build Status](https://travis-ci.org/jens-maus/libopenurl.svg?branch=master)](https://travis-ci.org/jens-maus/libopenurl) [![Code Climate](https://codeclimate.com/github/jens-maus/libopenurl/badges/gpa.svg)](https://codeclimate.com/github/jens-maus/libopenurl) [![License](http://img.shields.io/:license-public_domain-blue.svg?style=flat)](http://www.gnu.org/licenses/license-list.html#PublicDomain) [![Github Issues](http://githubbadges.herokuapp.com/jens-maus/libopenurl/issues.svg)](https://github.com/jens-maus/libopenurl/issues)

This Amiga-based shared library was created to make it easier for application
programmers to include clickable URLs in their applications, about windows, etc.
Current solutions to this problem typically are to launch an ARexx script
or just support a few webbrowsers, with no room for configuration. This
leads to countless reinventions of the wheel (how many `SendURLToBrowser.rexx`
scripts do you have on your harddisk?).

This library solves the problem by giving application programmers a very
simple API to handle (one function) and the user gets configurability with
the included preferences program.

OpenURL is available for AmigaOS3, AmigaOS4, MorphOS and AROS.

## Downloads/Releases

All releases up to the most current ones can be downloaded from our
[central releases management](https://github.com/jens-maus/libopenurl/releases).

## Bug Reports / Enhancement Requests

To report bugs use the [bug/issue tracker](https://github.com/jens-maus/libopenurl/issues).

## Manual Installation

1. Extract the archive to a temporary directory.
   ```
   > cd RAM:
   > lha x OpenURL.lha
   ```

2. Go to the `Binaries/<OS>` directory where `<OS>` is the directory
   matching the operating system you want to install TexiEditor.mcc for:
   ```
   > cd OpenURL/Binaries/AmigaOS4
   ```

3. copy all files found in that `<OS>` directory to their respective
   directory.
   ```
   > copy ALL #? SYS:
   ```

4. reboot and enjoy the new version ;)

## License / Copyright

```
Copyright (C) 1998-2005 by Troels Walsted Hansen, et al.
Copyright (C) 2005-2018 openurl.library Open Source Team
```
OpenURL was originally developed by Troels Walsted Hansen, et al. Since 2005
it is developed by an independent open source developer group. It is released
as a public domain software.

TextEditor is distributed and licensed under Public Domain.
See [COPYING](COPYING) for more detailed information.

## Authors

* Alexandre Balaban
* Alfonso Ranieri
* Dave Norris
* Jeff Gilpin
* Jens Maus
* Matthias Rustler
* Stefan Kost
* Thomas Aglassinger
* Thore Böckelmann
* Troels Walsted Hansen
