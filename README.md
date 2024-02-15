# dos2ansi - a tool to convert MS-DOS/ANSI.SYS text files to modern terminals

This tool converts MS-DOS text files using ANSI.SYS escape sequences to a
format a modern terminal can display. The output will use a Unicode encoding
of characters and only ANSI SGR escape sequences to set basic foreground and
background colors, intensity and blinking attribute. The input is expected to
use CP-437 or one of the other supported DOS codepages.

The builtin translation tables attempt to match the appearance on VGA as close
as possible. For example, unused codepoints are mapped to U+25AE (Black
Vertical Rectangle), because scans of old Microsoft documents show a glyph
resembling this. This is also relevant for e.g. arabic letters that have many
different forms; they are translated to unicode codepoints denoting the
"isolated" form, because this was the only way they could be displayed in text
mode. As a consequence, this tool isn't well-suited for converting arabic
*text*.

Results depend on both your terminal (there are interesting issues with e.g.
writing direction) and the font you're using (e.g. on Windows, the "Courier
New" font sometimes gives better results than "Consolas"). Please check with
different terminals and fonts before reporting a bug in dos2ansi.

### Install dos2ansi

Releases will contain a `.tar.xz` archive of the source for building yourself
(which works simply with `make` or `make strip`, see [Building](#building) for
details) and a `.zip` archive containing dos2ansi compiled for Windows.

Installation is as simple as placing the binary somewhere in your path, it
doesn't need any other files at runtime.

## Features

* Renders to a virtual canvas with a fixed width, defaulting to 80 columns
* Supports SGR sequences as well as cursor movement sequences in the input
* Autodetecs whether any visible color is used, if not, the output won't
  use any escape sequences
* Supports flexible modes for color output, from simple 8-color mode with
  attributes for "bright" colors up to exact CGA/VGA colors using 256-color
  mode and more, including a terminfo (curses) based writer for unix-like
  systems and one using the legacy Console API for older Windows versions
  (pre Windows 10).
* Supports all simple control characters MS-DOS interpreted when outputting
  text: NUL (ignored), BEL (ignored), BS, TAB, LF, CR and ESC (ignored when
  it doesn't start some ANSI.SYS sequence)
* Interprets control characters not supported by MS-DOS as symbols from CP437
* Stops reading at the MS-DOS EOL character (0x1a), this can also be disabled
* Input codepage can be selected, currently supported: 437, 708, 720, 737,
  775, 819, 850, 852, 855, 857, 860, 861, 862, 863, 864, 865, 869, KAM, MAZ
  and MIK
* Supports codepage aliases and versions with Euro sign
* Output format can be UTF-8, UTF-16 or UTF-16LE, with or without a BOM, with
  LF (Unix style) or CRLF (DOS/Windows style) line endings
* Optionally translates the pipe bar symbol to a broken bar, matching the
  appearance with most hardware fonts
* Includes a test mode rendering an encoding table with the selected settings
  and the color palette to verify appearance on your terminal
* Parses a subset of [SAUCE](https://github.com/radman1/sauce) metadata useful
  for MS-DOS text files, which is used to set some defaults (screen width,
  blinking mode and codepage) and can also be displayed or individual fields
  queried in a machine-readable format

## Test mode

![Test mode on Windows](.github/screenshots/dos2ansi_test_win32.png?raw=true)

## SAUCE display

![SAUCE in KDE's konsole](.github/screenshots/dos2ansi_sauce.png?raw=true)

## The showansi script

dos2ansi can optionally install a POSIX shell script (only on non-Windows) to
directly display some ANSI art file in some X terminal emulator, using fonts
and other settings from SAUCE metadata. The script is pre-configured to use
`xterm`, `less` as the pager and
[IBM bitmap fonts](https://github.com/farsil/ibmfonts). Unfortunately, these
fonts currently need a patch to play well with `xterm`, you can find it in
[my fork](https://github.com/Zirias/ibmfonts/tree/novt100).

The behavior of the script can be adjusted to your needs using environment
variables:

* `SHOWANSI_TERM`: The X terminal emulator to run.
  Default: `xterm`
* `SHOWANSI_TERM_ARGS`: Arguments to always pass to the X terminal emulator.
  Default: `-tn xterm-256color -fa "" -bg black -fg lightgray` (disables
  freetype usage and sets default colors matching VGA)
* `SHOWANSI_SETTITLE`: Flag used to set a custom window title, no title will
  be set if empty.
  Default: `-title`
* `SHOWANSI_SETGEOM`: Flag used to set the window geometry, won't be set if
  empty.
  Default: `-geometry`
* `SHOWANSI_SETFONT`: Flag used to set the terminal font, no font will be set
  if empty.
  Default: `-fn`
* `SHOWANSI_EXECUTE`: Flag needed to tell the X terminal emulator what to
  execute, can be empty.
  Default: `-e`
* `SHOWANSI_DOS2ANSI`: Command to invoke dos2ansi.
  Default: `dos2ansi`
* `SHOWANSI_D2A_ARGS`: Arguments to always pass to dos2ansi.
  Default: `-X` (disables attempting to match visuals, leave it to the font)
* `SHOWANSI_MAXWIDTH`: Maximum width for the terminal window when setting
  geometry.
  Default: `200`
* `SHOWANSI_MAXHEIGHT`: Maximum height for the terminal window when setting
  geometry.
  Default: `60`
* `SHOWANSI_ADDLINES`: Number of lines to add to the height from SAUCE
  metadata to make room for the pager.
  Default: `1`
* `SHOWANSI_PAGER`: The pager to pipe dos2ansi output to.
  Default: `less -r` (raw escape sequences mode)
* `SHOWANSI_FONT_<type>`: The font to use in the terminal emulator for font
  `<type>` wanted according to SAUCE metadata. Type can be one of `VGA9X16`,
  `VGA8X16`, `VGA9X8`, `VGA8X8`, `EGA9X14`, `EGA8X14`, `EGA9X8` or `EGA8X8`.
  Default: A font name for the ibmfonts mentioned above, e.g.
  `-ibm-vga-normal-r-normal--16-120-96-96-c-90-iso10646-1` for `VGA9X16`.
* `SHOWANSI_FONT_R_<type>`: Same as above, but used when aspect ratio should
  be corrected for traditional slightly rectangular pixels.
  Default: The same fonts as above, because "ibmfonts" doesn't have
  aspect-corrected variants.

Usage: `showansi ansifile [dos2ansiargs ...]`

Here is what it looks like in default configuration:

![showansi](.github/screenshots/showansi.png?raw=true)

## Building

To build dos2ansi from source, either grab and extract a release tarball or
clone the repository directly from git to get the latest changes:

    git clone --recurse-submodules https://github.com/Zirias/dos2ansi.git

The following is required for building:

* GNU make
* A C compiler supporting C11, either GCC or one compatible with GCC's
  commandline like llvm/clang.
* On "unixy" systems, a `curses` library (typically `ncurses`). This is
  optional, but enabled by default.

For Windows, GNU make and appropriate compilers are conveniently available
from the [MSYS2](https://msys2.org) distribution. If your system has a
different flavor of `make` by default (e.g. a BSD system), GNU make will
typically be installed as `gmake`, so type this instead of `make`.

To compile the tool, just type

    make

To get a stripped version, type

    make strip

If you want to build a version with full debugging symbols, you can use

    make BUILDCFG=debug

Installing can be done with

    make install

The binary is installed to `$(prefix)/bin`, with `prefix` defaulting to
`/usr/local`. So, to install e.g. to `/opt/dos2ansi/bin`, you would type

    make prefix=/opt/dos2ansi install

There are a few build-time configuration options available:

* `STATIC`: When set to a truthy value, the tool is linked statically. This is
  used for the official Windows binaries. Defaults to `0`.
* `WITH_CURSES`: Use `curses` to build a terminfo-based output writer that
  will automatically respect `$TERM`. Only available on non-Windows, defaults
  to `1`.
* `FORCE_STDIO`: Always use the standard C `stdio.h` functions for I/O,
  instead of a platform-specific backend (available are POSIX and win32).
  Defaults to `0`.

So, `make FORCE_STDIO=1 strip` would build a stripped version using standard C
I/O, `make WITH_CURSES=no` would build a version not linked to `curses`.
