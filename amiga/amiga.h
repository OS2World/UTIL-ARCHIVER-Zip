/*

 Copyright (C) 1990-1996 Mark Adler, Richard B. Wales, Jean-loup Gailly,
 Kai Uwe Rommel, Onno van der Linden, Igor Mandrichenko, Paul Kienitz and
 John Bush.
 Permission is granted to any individual or institution to use, copy, or
 redistribute this software so long as all of the original files are included,
 that it is not sold for profit, and that this copyright notice is retained.

*/

/* amiga.h
 *
 * Globular definitions that affect all of AmigaDom.
 *
 * Originally included in unzip.h, extracted for simplicity and eeze of
 * maintenance by John Bush.
 *
 * This version is for use with Zip.  It is not globally included, but used
 * only by functions in amiga/amigazip.c.  Much material that was needed for
 * UnZip is absent here.
 *
 */

#include <time.h>
#include <fcntl.h>     /* O_BINARY for open() w/o CR/LF translation */

#ifdef AZTEC_C                   /* Manx Aztec C, 5.0 or newer only */
#  include <clib/dos_protos.h>
#  include <pragmas/dos_lib.h>       /* do inline dos.library calls */
#  define MODERN
#  define O_BINARY 0
#  include "amiga/z-stat.h"              /* substitute for <stat.h> */
#  define direct dirent
#endif /* AZTEC_C */


#if defined(LATTICE) || defined(__SASC) || defined(__SASC_60)
#  include <sys/types.h>         
#  include <sys/stat.h>
/* Is this define necessary ?? */
#  define direct dirent
#  include <sys/dir.h>
#  include <dos.h>
#  include <proto/dos.h>  /* needed? */
#  define disk_not_mounted 0  
#  if ( (!defined(O_BINARY)) && defined(O_RAW))
#    define O_BINARY O_RAW
#  endif
#endif /* LATTICE */

/* Funkshine Prough Toe Taipes */

LONG FileDate (char *, time_t[]);
