# dos2ansi - a tool to convert MS-DOS/ANSI.SYS text files to modern terminals

This tool converts MS-DOS text files using ANSI.SYS escape sequences to a
format a modern terminal can display. The output will use a Unicode encoding
of characters and only ANSI SGR escape sequences to set basic foreground and
background colors, intensity and blinking attribute. The input is expected to
use CP-437 or one of the other supported DOS codepages.

The builtin translation tables attempt to match the appearance on VGA as close
as possible by default. For example, unused codepoints are mapped to U+25AE
(Black Vertical Rectangle), because scans of old Microsoft documents show a
glyph resembling this. This is also relevant for e.g. arabic letters that have
many different forms; they are translated to unicode codepoints denoting the
"isolated" form, because this was the only way they could be displayed in text
mode.

As a consequence, this default mode isn't well-suited for converting arabic
*text*. There's a flag (`-X`) to disable this behavior and use canonic
mappings instead, which is also a good idea when using a font reproducing the
original IBM character set glyphs.

Results depend on both your terminal (there are interesting issues with e.g.
writing direction) and the font you're using (e.g. on Windows, the "Courier
New" font sometimes gives better results than "Consolas"). Please check with
different terminals and fonts before reporting a bug in dos2ansi.

### Install dos2ansi

Releases will contain a `.tar.xz` archive of the source for building yourself
(which works simply with `make`, see [Building](#building) for details), a
`.deb` containing an amd64 Linux binary and a `.zip` archive containing an
i386 dos2ansi binary for Windows.

Installation is as simple as placing the binary somewhere in your path, it
doesn't need any other files at runtime.

### Usage manual

You can read the [Manpages](https://zirias.github.io/dos2ansi) online, they
will also be installed by default.

## Features

* Renders to a virtual canvas with a fixed width, defaulting to 80 columns
* Supports SGR sequences as well as cursor movement/placement and line/screen
  erasing sequences like ANSI.SYS; setting screen mode is implemented for the
  standard text modes 0-3 and line wrapping on/off, other modes are silently
  ignored
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
and other settings from SAUCE metadata. It's pre-configured to use `xterm` as
the X terminal emulator and `less` as the pager.

The script comes with a sample configuration file
`$(prefix)/etc/dos2ansi/showansirc.sample` you can copy to
`$(prefix)/etc/dos2ansi/showansirc` to activate it. Three fontsets are
installed, one using bitmap fonts from [IBM bitmap
fonts](https://github.com/farsil/ibmfonts), one using scalable fonts from [The
Ultimate Oldschool PC Font Pack](https://int10h.org/oldschool-pc-fonts), and
one using a mix of both. The sample configuration file defaults to the mix,
because scalable fonts are necessary for aspect correction, but have an effect
on sharpness, so bitmap fonts are preferred when no aspect correction is
necessary.

showansi can output debugging information by setting the environment variable
`SHOWANSI_DEBUG` to `1`.

> [!WARNING]
>
> This script just sources configuration files. Be sure to read the comments
> in the configuration file about allowing per-user configuration, which is
> disabled by default, but enabled in the sample configuration.

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
from the [MSYS2](https://msys2.org) distribution, or if you want it truly
minimal, installing `make` and `mingw` from "chocolatey" is also enough,
then you can build directly from the Windows commandline (CMD.EXE).

If your system has a different flavor of `make` by default (e.g. a BSD
system), GNU make will typically be installed as `gmake`, so type this instead
of `make`.

To compile the tool, just type

    make

If you want to build a version with full debugging symbols, you can use

    make BUILDCFG=debug

Installing can be done with

    make install

There are a few build-time configuration options available:

* `STATIC`: When set to a truthy value, the tool is linked statically. This is
  used for the official Windows binaries. Defaults to `0`.
* `WITH_CURSES`: Use `curses` to build a terminfo-based output writer that
  will automatically respect `$TERM`. Only available on non-Windows, defaults
  to `1`.
* `WITH_HTML`: Build and install manpages in HTML format. Defaults to `0`, but
  forced on when building for Windows.
* `WITH_MAN`: Build and install manpages. Only available on non-Windows,
  defaults to `1`.
* `WITH_SHOWANSI`: Also install the `showansi` script, see above. Only
  available on non-Windows, defaults to `1`.
* `FORCE_STDIO`: Always use the standard C `stdio.h` functions for I/O,
  instead of a platform-specific backend (available are POSIX and win32).
  Defaults to `0`.
* `MANFMT`: The format of the manpages, either `man` (classic troff/man) or
  `mdoc` (BSD-style mandoc). Defaults to `mdoc` if the OS name contains `BSD`,
  `man` otherwise.

So, `make FORCE_STDIO=1` would build a stripped version using standard C
I/O, `make WITH_CURSES=no` would build a version not linked to `curses`.

Options to `make` must be given unchanged for building and installing,
otherwise installing will trigger a full rebuild. You can also save options
using the `config` target, e.g.

    make WITH_HTML=1 config

The binary is installed to `$(prefix)/bin`, with `prefix` defaulting to
`/usr/local`. So, to install e.g. to `/opt/dos2ansi/bin`, you would type

    make prefix=/opt/dos2ansi config
    make
    make install

To display all available configuration variables, including generic ones and
target directories, with their current (default or configured) values, type

    make showconfig

This also shows a few detected values you can't change yourself, like the
target platform and architecture.
