# AROS themes

A theme is a directory in `THEMES:` (`SYS:Prefs/Presets/Themes`). The theme
in use is assigned to `THEME:`. This document describes what a theme directory
contains and how its images are named and found. It applies to the themes in
`images/Themes` and to themes written for AROS in general.

The image layout follows the conventions of MUI image sets, so that images can
be shared between AROS themes and MUI.

## Layout

    THEMES:<theme>/
        Description             name and author, shown by the Appearance prefs
        Images/
            <size>pt/           an image set for one size
                <Name>.<ext>
        System/Config           window and screen decoration settings
        Menu/Config             menu decoration settings
        Env-Archive/            preferences applied with the theme
            Zune/global.prefs
            SYS/Wanderer/global.prefs
            SYS/palette.prefs
            SYS/font.prefs
            SYS/pointer.prefs

Only the parts a theme wants to provide need to exist, apart from
`System/Config`, which identifies the directory as a theme.

### Theme version

A theme states the version of this layout it is written for in its
`System/Config`:

    DecorVers = 2

The current version is 2. A theme that states another version, or none, is not
usable: the decoration (`decortheme.library`) does not load it, and the
Appearance preferences do not list it. Themes written before the layout was
versioned have to be converted (see below).

A directory without `System/Config` is not a theme, but its image sets can
still be used as images in Zune (see "In image specifications").

### Styles

A theme that comes in several styles (for instance light and dark) keeps each
style in a sub-directory of its own. Every style is a complete theme:

    THEMES:<theme>/<style>/Images/...
    THEMES:<theme>/<style>/System/Config

The Appearance preferences offer the styles of such a theme for selection
beside the theme, when the theme directory is not a theme by itself. The
selected theme and style are stored as `THEMES:<theme>/<style>` and assigned
to `THEME:` like any other theme.

## Image sets

Images are stored in size sets. The directory name is the font size the images
were drawn for, followed by `pt`, as in MUI:

| Set    | Intended for                           |
|--------|----------------------------------------|
| `11pt` | displays below 640x480                 |
| `16pt` | displays from 640x480 up to 1920x1200  |
| `32pt` | displays up to 3840x2400 (optional)    |
| `64pt` | larger displays (optional)             |

A theme should provide at least `16pt`. The button images of the themes in
this tree are 20x20 pixels in `16pt` and 14x14 in `11pt`.

### Selecting a size

The size to use is selected by the user in the Appearance preferences, which
offer the sizes of the selected theme when it has more than one. The selection
starts out as the theme's default size: the one stated with

    DefaultSize = 16

in `System/Config`, or 16 if the theme does not state one, or the closest size
the theme provides. The selected size is stored in the `SYS/themesize.var`
environment variable.

A set does not have to be complete. An image the selected set lacks is taken
from one of the theme's other sets.

### The `THEMEIMAGES:` assign

The images of the theme in use are found through the `THEMEIMAGES:` assign.
It is made at startup and lists the theme's image set of the selected size
followed by its other sets, so a file name there finds the image in the
selected size, or in another size if that set lacks it:

    THEMEIMAGES:Cancel.mbr

Code that shows theme images uses this assign rather than a path into a
particular theme or size.

### File types

The extension states what a file holds. They are the ones MUI uses:

| Extension            | Contents                                              |
|----------------------|-------------------------------------------------------|
| `.mbr`               | an image with a single state                          |
| `.mf0`, `.mf1`, ...  | one state of an image that draws its own frame        |
| `.mb0`, `.mb1`, ...  | one state of an image without a frame                 |
| `.mim`               | all states of an image, side by side in one file      |
| `.mi1`               | an image with a single state                          |
| `.image`             | a BOOPSI image class                                  |

The files are loaded through datatypes, so any picture format with a datatype
can be used. The themes in this tree use PNG with an alpha channel; images
written for MUI are IFF ILBM brushes.

### States

The number in `.mf<n>` and `.mb<n>` is the state:

| State | Meaning                                   |
|-------|-------------------------------------------|
| 0     | normal                                    |
| 1     | selected                                  |
| 2     | inactive, or disabled                     |
| 3     | inactive and selected                     |

MUI and Zune use states 0 and 1. The window decoration uses all of them for
the gadgets in window borders.

A state does not need a file of its own. A missing state 3 uses state 1, and
any other missing state uses state 0. A theme can therefore provide only
`.mf0`, the `.mf0`/`.mf1` pair MUI expects, or all four. All the states of an
image must have the same dimensions.

Some decoration images use the states differently; see the table below.

## Image names

### Buttons

These are shown on buttons by applications, through `ImageButton()`:

`Cancel`, `Help`, `OK`, `Revert`, `Save`, `Search`, `Test`, `Use`, `DirUp`

Buttons that go back, forward or expand use the MUI arrow images described
below (`ArrowLeft`, `ArrowRight`, `ArrowDown`).

and, for small buttons such as the ones beside a list, the smaller versions
`AddSmall`, `DeleteSmall`, `UpSmall` and `DownSmall` (12x12 pixels in `16pt`),
with `Add`, `Delete`, `Up` and `Down` as the versions of normal size.

A theme can add images of its own; they are available to applications and in
the Zune preferences under their file name.

### MUI images

A theme can provide images for Zune's own gadgets under the names MUI gives
them:

`ArrowUp`, `ArrowDown`, `ArrowLeft`, `ArrowRight`, `CheckMark`,
`RadioButton`, `Cycle`, `PopUp`, `PopFile`, `PopDrawer`, `Drawer`,
`HardDisk`, `Disk`, `Chip`, `Volume`, `Network`, `Assign`, `TapePlay`,
`TapePlayBack`, `TapePause`, `TapeStop`, `TapeRecord`, `TapeUp`, `TapeDown`

These are the names of MUI's `MUII_` image numbers. The arrow images are also
the arrows of the window borders, and are used on buttons.

Zune does not use these images by itself: it draws the gadgets with the images
that are configured. To use a theme's image, select it on the External page of
the image's setting in the Zune preferences, either from the entries of the
theme in use (`THEMEIMAGES:`, which follow the theme and the size selected for
it) or from the images of a particular theme and size.

A theme author who wants the theme's images used provides a Zune configuration
with the theme (`Env-Archive/Zune/global.prefs`) in which they are selected.
It is applied when the user chooses to use the theme's Zune configuration in
the Appearance preferences.

For `Drawer`, `HardDisk`, `Disk` and `Volume` the system provides image
classes that show the default icons of icon.library (`Drawer.image`,
`HardDisk.image`, `Disk.image`, `Volume.image`, and also `Ram.image`). When
they are installed they are what Zune uses for these images until another one
is configured, so that applications show the same imagery as the desktop.

### Decoration

The screen, window and menu decoration (`C:Decoration`) uses these images.
The settings that refer to them (offsets, sizes, layout) are in
`System/Config` and `Menu/Config`.

| Name            | States | Use                                                |
|-----------------|--------|----------------------------------------------------|
| `WinClose`      | 4      | window close gadget                                |
| `WinDepth`      | 4      | window depth gadget                                |
| `WinZoom`       | 4      | window zoom gadget                                 |
| `WinSize`       | 4      | window size gadget                                 |
| `WinIconify`    | 4      | window iconify gadget                              |
| `WinLock`       | 4      | window lock gadget                                 |
| `WinMUI`        | 4      | MUI settings gadget                                |
| `WinPopUp`      | 4      | window popup gadget                                |
| `WinSnapshot`   | 4      | window snapshot gadget                             |
| `ArrowUp`, `ArrowDown`, `ArrowLeft`, `ArrowRight` | 4 | scroll arrows in window borders |
| `WinBar`        | 2      | title bar: 0 active window, 1 inactive window      |
| `WinBorder`     | 2      | border fill: 0 active window, 1 inactive window    |
| `PropBackVert`  | 2      | vertical scroller container: 0 active, 1 inactive  |
| `PropBackHoriz` | 2      | horizontal scroller container: 0 active, 1 inactive |
| `PropKnobVert`  | 3      | vertical scroller knob: 0 normal, 1 hit, 2 inactive |
| `PropKnobHoriz` | 3      | horizontal scroller knob: 0 normal, 1 hit, 2 inactive |
| `ScrDepth`      | 2      | screen depth gadget                                |
| `ScrBar`        | 1      | screen title bar                                   |
| `ScrBarLogo`    | 1      | logo in the screen title bar                       |
| `MenuBack`      | 1      | menu background                                    |
| `MenuAmigaKey`  | 1      | menu shortcut symbol                               |
| `MenuCheck`     | 1      | menu checkmark                                     |
| `MenuSubMenu`   | 1      | sub-menu indicator                                 |

A theme with `NoInactiveSelected = Yes` in `System/Config` uses three states
for the window gadgets.

The decoration uses the set of the selected size. Each image is looked for as
`.mf<n>`, `.mb<n>`, `.mim`, `.mbr` and `.mi1`, in that order. An image with a
single file (`.mbr`, `.mi1`) is used for every state.

The settings in `System/Config` and `Menu/Config` describe the images of the
theme's default size. A theme that provides decoration images in another size
as well puts the settings that differ for that size in a file named `Config`
in the image set (`Images/<size>pt/Config`); it is read after the other two
and can contain any of their settings.

## How images are found

### By applications

An application refers to an image of the theme in use without stating a size
or a file type:

    THEMEIMAGES:<Name>

for example `ImageButton(label, "THEMEIMAGES:Cancel")`. Zune finds the file
type the theme provides, and the assign the size. A name with a file type
(`THEMEIMAGES:Add.mbr`) is an ordinary file name, which a program can also use
to check that the image exists.

### In image specifications

External images are named in MUI image specifications of type 3
(`3:<name>`). A name can be:

- `THEMEIMAGES:<Name>`: an image of the theme in use, as described above. A
  specification of this kind follows the theme, and the size selected for it,
  when they are changed.
- a path relative to an image set directory, as in MUI. Zune looks in
  `IMAGES:Zune/` (its equivalent of `MUI:Images/`) and then in `THEMES:`, so
  the images of any installed theme can be named in a particular size, for
  example `3:AROSDefault/Images/16pt/CheckMark.mf0`. An image set written for MUI
  (`<set>/[<style>/]<size>pt/<Name>.<ext>`) can be copied to `IMAGES:Zune/`
  and used unchanged.
- the full path of a file.

When the name ends in `0` (`.mf0`, `.mb0`), the file with `1` in its place is
used for the selected state. Specifications never name the selected image.

The External page of the Zune image settings lists the images of the theme in
use first (`THEMEIMAGES:`), then every image in `IMAGES:Zune/` and `THEMES:`,
the latter with the theme and size each one belongs to. As in MUI, only
files with the extensions `.mbr`, `.mb0`, `.mf0`, `.mim`, `.mi1` and `.image`
are listed.

## Themes in the original layout

Themes used to keep their images in other places:

| Original file                          | Now                              |
|----------------------------------------|----------------------------------|
| `Images/Gadgets/<Name>`                | `Images/16pt/<Name>.mbr`         |
| `Images/Gadgets/Previous`, `Next`, `Below` | the `ArrowLeft`, `ArrowRight` and `ArrowDown` images |
| `Images/Gadgets/small/<Name>`          | `Images/16pt/<Name>Small.mbr`    |
| `System/<Name>/Default`                | `Images/16pt/<NewName>.mf<n>`    |
| `System/Borders/Default`               | `Images/16pt/WinBorder.mf0`      |
| `System/Borders/Default_Deactivated`   | `Images/16pt/WinBorder.mf1`      |
| `Menu/<Name>/Default`                  | `Images/16pt/<NewName>.mbr`      |

with every state of a decoration image stored side by side in one file.

Such themes are no longer used: they do not state a theme version, so the
decoration does not load them and the Appearance preferences do not list them.
Image names are not translated either: code that names images by their
original path (`THEME:Images/Gadgets/<Name>`) has to be changed to the current
names; note that `Previous`, `Next` and `Below` are now `ArrowLeft`,
`ArrowRight` and `ArrowDown`.

### Converting a theme

`scripts/theme-convert.py` converts a theme in the original layout:

    scripts/theme-convert.py [options] <theme directory> ...

It writes the `16pt` set from the theme's images, splitting the multi state
decoration images into one file for each state, creates the `11pt` button
images by scaling, and states the theme version in `System/Config`. A state that is identical to the one used in its place is
not written. Options:

| Option            | Effect                                                    |
|-------------------|-----------------------------------------------------------|
| `--size N`        | the size the theme's images were drawn for (default 16)   |
| `--scaled N`      | also create scaled button images for size N (default 11)  |
| `--no-scaled`     | do not create scaled button images                        |
| `--all-states`    | write every state, including ones that match their fallback |
| `--remove`        | remove the original files once converted                  |
| `-n`, `--dry-run` | show what would be done without writing anything          |

The script needs Python 3 only, and reads PNG images. Apart from the theme
version, `System/Config` and `Menu/Config` are not changed: the offsets in
them refer to the same pixels as before, since the states keep their
dimensions.
