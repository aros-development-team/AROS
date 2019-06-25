# BetterString.mcc

[![Build Status](https://travis-ci.org/amiga-mui/betterstring.svg?branch=master)](https://travis-ci.org/amiga-mui/betterstring) [![Code Climate](https://codeclimate.com/github/amiga-mui/betterstring/badges/gpa.svg)](https://codeclimate.com/github/amiga-mui/betterstring) [![License](http://img.shields.io/:license-lgpl2-blue.svg?style=flat)](http://www.gnu.org/licenses/lgpl-2.1.html) [![Github Issues](http://githubbadges.herokuapp.com/amiga-mui/betterstring/issues.svg)](https://github.com/amiga-mui/betterstring/issues)

BetterString is a string gadget replacement for Amiga systems. It is created for MUI,
so using it should eliminate the problems with the original BOOPSI string class.
The class offers the user to do number arithmetic, like increase, decrease and
bin to  hex conversion. It has filename completion, ability to mark, cut, copy
and paste text - Both via mouse and keyboard. The length of the contents
buffer will dynamically be expanded to hold all of what the user type (unless
a maximum length is given)

It is used by a wide range of well-known MUI-based application like [YAM](https://github.com/jens-maus/yam)
or [SimpleMail](https://github.com/sba1/simplemail) as the main string gadget.

BetterString.mcc is available for AmigaOS3, AmigaOS4, MorphOS and AROS.

## Downloads/Releases

All releases up to the most current ones can be downloaded from our
[central releases management](https://github.com/amiga-mui/betterstring/releases).

## Bug Reports / Enhancement Requests

To report bugs use the [bug/issue tracker](https://github.com/amiga-mui/betterstring/issues).

## Manual Installation

1. Extract the archive to a temporary directory.
   ```
   > cd RAM:
   > lha x MCC_BetterString.lha
   ```

2. Go to the `MCC_BetterString/Libs/MUI/<OS>` directory where `<OS>` is the directory
   matching the operating system you want to install TexiEditor.mcc for:
   ```
   > cd MCC_BetterString/Libs/MUI/AmigaOS4
   ```

3. copy all `#?.mcc` and `#?.mcp` files found in that `<OS>` directory to the
   global `MUI:Libs/mui/` directory on your system partition:
   ```
   > copy #?.mcc MUI:Libs/mui/
   > copy #?.mcp MUI:Libs/mui/
   ```

4. reboot and enjoy the new version ;)

## License / Copyright

The gadget was originally written in 1997, and is Copyright (c) 1997-2000 by
Allan Odgaard. As of version 11.7, release in July 2005, the gadget is
maintained and Copyright (c) by the BetterString.mcc Open Source Team.

BetterString is distributed and licensed under the GNU Lesser General Public
License Version 2.1. See [COPYING](COPYING) for more detailed information.

## Authors

* Allan Odgaard
* Ilkka Lehtoranta
* Jens Maus
* Thore Böckelmann
