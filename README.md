# dos2ansi - a tool to convert MS-DOS/ANSI.SYS text files to modern terminals

This tool converts MS-DOS text files using ANSI.SYS escape sequences to a
format a modern terminal can display. The output will use UTF-8 encoding of
characters and only ANSI SGR escape sequences to set basic foreground and
background colors, intensity and blinking attribute. The input is expected to
use CP-437 encoding.

## Features:

* Renders to a virtual canvas with a fixed width, defaulting to 80 columns
* Supports SGR sequences as well as cursor movement sequences in the input
* Autodetecs whether any visible color is used, if not, the output won't
  use any escape sequences
* Supports all simple control characters MS-DOS interpreted when outputting
  text: NUL (ignored), BEL (ignored), BS, TAB, LF, CR and ESC (ignored when
  it doesn't start some ANSI.SYS sequence)
* Interprets control characters not supported by MS-DOS as symbols from CP437
* Stops reading at the MS-DOS EOL character (0x1a), this can also be disabled

## Maybe planned:

* Support alternative MS-DOS codepages for input
* Support UTF-16 for output

