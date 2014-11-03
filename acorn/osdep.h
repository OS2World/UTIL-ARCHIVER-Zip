/*

 Copyright (C) 1990-1996 Mark Adler, Richard B. Wales, Jean-loup Gailly,
 Kai Uwe Rommel, Onno van der Linden, Sergio Monesi, Karl Davis and
 Igor Mandrichenko.
 Permission is granted to any individual or institution to use, copy, or
 redistribute this software so long as all of the original files are included,
 that it is not sold for profit, and that this copyright notice is retained.

*/

#include "acorn/riscos.h"

#define RISCOS
#define NO_SYMLINK
#define NO_FCNTL_H
#define NO_UNISTD_H
#define NO_MKTEMP

#define isatty(a) 1
