#ifndef DOS2ANSI_STRCICMP_H
#define DOS2ANSI_STRCICMP_H

#ifdef _WIN32
#  include <windows.h>
#  define strcicmp _stricmp
#else
#  define _POSIX_C_SOURCE 200112L
#  include <strings.h>
#  define strcicmp strcasecmp
#endif

#endif
