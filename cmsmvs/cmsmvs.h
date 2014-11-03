/*

 Copyright (C) 1990-1996 Mark Adler, Richard B. Wales, Jean-loup Gailly,
 Kai Uwe Rommel, Onno van der Linden, George Petrov and Igor Mandrichenko.
 Permission is granted to any individual or institution to use, copy, or
 redistribute this software so long as all of the original files are included,
 that it is not sold for profit, and that this copyright notice is retained.

*/

#ifndef __cmsmvs_h   /* prevent multiple inclusions */
#define __cmsmvs_h

#define CMS_MVS
#define NO_UNISTD_H
#define NO_FCNTL_H


/* Include file for VM/CMS and MVS */

#ifndef NULL
#  define NULL 0
#endif

#include <time.h>               /* the usual non-BSD time functions */
#include "stat.h"

#define PASSWD_FROM_STDIN
                  /* Kludge until we know how to open a non-echo tty channel */

/* definition for ZIP */
#define getpid() (int)time(NULL)
#define getch() getc(stdin)
#define native(c)   ebcdic[(c)]
#define MAXPATHLEN 128
#define NO_RMDIR
#define NO_MKTEMP
#define fileno(x) (char *)(x)
#define fdopen fopen
#define unlink remove
#define link rename
#define utime(f,t)
#define isatty(t) 1
/* end defines for ZIP */

#define EBCDIC

#ifdef UNZIP                    /* definitions for UNZIP */

#define INBUFSIZ 8192

#define USE_STRM_INPUT
#define USE_FWRITE

#define REALLY_SHORT_SYMS
#define PATH_MAX 128

#endif /* UNZIP */

#define FOPR "rb,recfm=fb"
#define FOPM "r+"
#define FOPW "wb,recfm=fb"
#define FOPWT "w"

#define CBSZ 0x40000
#define ZBSZ 0x40000

#endif /* !__cmsmvs_h */
