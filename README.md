# dos2ansi - a tool to convert MS-DOS/ANSI.SYS text files to modern terminals

This is work in progress, the goal is to convert old MS-DOS "text" files using
8bit encodings like CP-437 and escape sequences supported by ANSI.SYS to a
modern representation easily viewable on today's terminals.

For this, it should

* translate the encoding to a standard one, most likely UTF-8 which should be
  the default
* insert "missing" newlines assuming a fixed output width (by default 80 cols)
* translate the DOS newlines to the target system format
* eliminate cursor positioning escape sequences, so the result only contains
  color setting sequences supported by almost every terminal in the standard
  mode

