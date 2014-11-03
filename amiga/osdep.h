/*

 Copyright (C) 1990-1996 Mark Adler, Richard B. Wales, Jean-loup Gailly,
 Kai Uwe Rommel, Onno van der Linden, Igor Mandrichenko, Paul Kienitz and
 John Bush.
 Permission is granted to any individual or institution to use, copy, or
 redistribute this software so long as all of the original files are included,
 that it is not sold for profit, and that this copyright notice is retained.

*/

#define NO_UNISTD_H
#define NO_MKTIME

/* default to MEDIUM_MEM, but allow makefile override */
#if ( (!defined(BIG_MEM)) && (!defined(SMALL_MEM)))
#  define MEDIUM_MEM
#endif

#if defined(LATTICE) || defined(__SASC)
#  include <sys/types.h>
#  include <sys/stat.h>
   extern int isatty(int);
#endif

#ifdef AZTEC_C
#  include <fcntl.h>
#  include "amiga/z-stat.h"
#  define NO_RMDIR
#  define NO_SYS_TYPES_H
#endif

char *GetComment(char *);


#define FOPR "rb"
#define FOPM "rb+"
#define FOPW "wb"
