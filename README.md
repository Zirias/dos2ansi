# dos2ansi - a tool to convert MS-DOS/ANSI.SYS text files to modern terminals

This tool converts MS-DOS text files using ANSI.SYS escape sequences to a
format a modern terminal can display. The output will use a Unicode encoding
of characters and only ANSI SGR escape sequences to set basic foreground and
background colors, intensity and blinking attribute. The input is expected to
use CP-437 or one of the other supported DOS codepages.

## Features

* Renders to a virtual canvas with a fixed width, defaulting to 80 columns
* Supports SGR sequences as well as cursor movement sequences in the input
* Autodetecs whether any visible color is used, if not, the output won't
  use any escape sequences
* Supports all simple control characters MS-DOS interpreted when outputting
  text: NUL (ignored), BEL (ignored), BS, TAB, LF, CR and ESC (ignored when
  it doesn't start some ANSI.SYS sequence)
* Interprets control characters not supported by MS-DOS as symbols from CP437
* Stops reading at the MS-DOS EOL character (0x1a), this can also be disabled
* Input codepage can be selected, currently supported: 437, 850 and 858
* Output format can be UTF-8, UTF-16 or UTF-16LE, with or without a BOM, with
  LF (Unix style) or CRLF (DOS/Windows style) line endings
* Optionally translates the pipe bar symbol to a broken bar, matching the
  appearance with most hardware fonts
* Includes a test mode rendering an encoding table with the selected settings
  to verify appearance on your terminal

## Test mode

![Test mode on Windows](.github/screenshots/dos2ansi_test_win32.png?raw=true)

## Maybe planned

* Output to terminal using native APIs (Windows) or termcap/terminfo
  (non-Windows) instead of fixed ANSI sequences
* Support more MS-DOS codepages

