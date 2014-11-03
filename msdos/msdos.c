/*

 Copyright (C) 1990-1996 Mark Adler, Richard B. Wales, Jean-loup Gailly,
 Onno van der Linden, Christian Spieler and Kai Uwe Rommel.
 Permission is granted to any individual or institution to use, copy, or
 redistribute this software so long as all of the original files are included,
 that it is not sold for profit, and that this copyright notice is retained.

*/

#ifndef UTIL    /* no material in this file is used by UTIL */

#include "zip.h"
#include <dos.h>
#include <time.h>


#if defined(__GO32__) || defined(__TURBOC__)
#  include <dir.h> /* prototypes of find*() */
   typedef struct ffblk   ff_dir;
#  define FATTR (hidden_files ? FA_HIDDEN+FA_SYSTEM+FA_DIREC : FA_DIREC)
#  define FFIRST(n,d,a)   findfirst(n,(struct ffblk *)d,a)
#  define FNEXT(d)        findnext((struct ffblk *)d)
#  if (!defined(__DJGPP__) || (__DJGPP__ < 2))
#    define GetFileMode(name) bdosptr(0x43, (name), 0)
#  endif
#endif /* __GO32__ || __TURBOC__ */

#if defined(MSC) || defined(__WATCOMC__)
   typedef struct find_t  ff_dir;
#  define FATTR (hidden_files ? _A_HIDDEN+_A_SYSTEM+_A_SUBDIR : _A_SUBDIR)
#  ifndef FA_LABEL
#    define FA_LABEL      _A_VOLID
#  endif
#  define FFIRST(n,d,a)   _dos_findfirst(n,a,(struct find_t *)d)
#  define FNEXT(d)        _dos_findnext((struct find_t *)d)
#  define ff_name         name
#  define ff_fdate        wr_date
#  define ff_ftime        wr_time
#  define ff_attrib       attrib
#endif /* MSC || __WATCOMC__ */

#ifdef __EMX__
#  define size_t xxx_size_t
#  define wchar_t xxx_wchar_t
#  define tm xxx_tm
#  include <sys/emx.h>
#  undef size_t
#  undef wchar_t
#  undef tm
   typedef struct _find   ff_dir;
#  define FATTR (hidden_files ? _A_HIDDEN+_A_SYSTEM+_A_SUBDIR : _A_SUBDIR)
#  define FA_LABEL        _A_VOLID
#  define FFIRST(n,d,a)   __findfirst(n,a,d)
#  define FNEXT(d)        __findnext(d)
#  define ff_name         name
#  define ff_fdate        date
#  define ff_ftime        time
#  define ff_attrib       attr
#  define GetFileMode(name) __chmod(name, 0, 0)
#endif /* __EMX__ */


/* Keep this =>SYNCHRONIZED<= with the corresponding definition in fileio.c */
#if defined(__GO32__) || defined(__EMX__)
#define MATCH shmatch
#else
#define MATCH dosmatch
#endif /* __GO32__ */

#define PAD  0
#define PATH_END '/'

/* Library functions not in (most) header files */
int rmdir OF((const char *));
int utime OF((char *, ztimbuf *));

/* Local functions */
#ifndef GetFileMode
int GetFileMode OF((char *name));
#endif /* !GetFileMode */

local int  initDirSearch OF((char *name, ff_dir *ff_context_p));
local char *getVolumeLabel OF((int, ulg *, ulg *, time_t *));

/* Module level variables */
extern char *label;
local ulg label_time = 0;
local ulg label_mode = 0;
local time_t label_utim = 0;

/* Module level constants */
local const char wild_match_all[] = "*.*";


#ifndef GetFileMode
int GetFileMode(char *name)
{
  unsigned int attr = 0;
  _dos_getfileattr(name, &attr);
  return attr;
}
#endif /* !GetFileMode */

local int initDirSearch(name, ff_context_p)
  char *name;                   /* name of directory to scan */
  ff_dir *ff_context_p;         /* pointer to FFIRST/FNEXT context structure */
{
  int r;                        /* FFIRST return value */
  char *p, *q;                  /* temporary copy of name, and aux pointer */

  if ((p = malloc(strlen(name) + 5)) == NULL)
    return ZE_MEM;

  strcpy(p, name);
  q = p + strlen(p);
  if ((q - p) > 0 && *(q - 1) != '/')
    *q++ = '/';
  strcpy(q, wild_match_all);
  r = FFIRST(p, ff_context_p, FATTR);
  free((zvoid *)p);

  return (r ? ZE_MISS : ZE_OK);
}

local char *getVolumeLabel(drive, vtime, vmode, vutim)
  int drive;    /* drive name: 'A' .. 'Z' or '\0' for current drive */
  ulg *vtime;   /* volume label creation time (DOS format) */
  ulg *vmode;   /* volume label file mode */
  time_t *vutim;/* volume label creationtime (UNIX format) */

/* If a volume label exists for the given drive, return its name and
   set its time and mode. The returned name must be static data. */
{
  static char vol[14];
  ff_dir d;

  if (drive) {
    vol[0] = (char)drive;
    strcpy(vol+1, ":/");
  } else {
    strcpy(vol, "/");
  }
  strcat(vol, wild_match_all);
  if (FFIRST(vol, &d, FA_LABEL) == 0) {
    strncpy(vol, d.ff_name, sizeof(vol)-1);
    *vtime = ((ulg)d.ff_fdate << 16) | ((ulg)d.ff_ftime & 0xffff);
    *vmode = (ulg)d.ff_attrib;
    *vutim = dos2unixtime(*vtime);
    return vol;
  }
  return NULL;
}

int wild(w)
char *w;                /* path/pattern to match */
/* If not in exclude mode, expand the pattern based on the contents of the
   file system.  Return an error code in the ZE_ class. */
{
  ff_dir d;             /* control structure for FFIRST/FNEXT */
  char *e;              /* name found in directory */
  int r;                /* temporary variable */
  int ff_status;        /* return value of FFIRST/FNEXT */
  char *n;              /* constructed name from directory */
  int f;                /* true if there was a match */
  char *a;              /* alloc'ed space for name */
  char *p;              /* path */
  char *q;              /* name */
  char v[5];            /* space for device current directory */

  if (volume_label == 1) {
    volume_label = 2;
    label = getVolumeLabel((w != NULL && w[1] == ':') ? to_up(w[0]) : '\0',
                           &label_time, &label_mode, &label_utim);
    if (label != NULL) {
       (void)newname(label, 0);
    }
    if (w == NULL || (w[1] == ':' && w[2] == '\0')) return ZE_OK;
    /* "zip -$ foo a:" can be used to force drive name */
  }

  if (w == NULL)
    return ZE_OK;

  /* special handling of stdin request */
  if (strcmp(w, "-") == 0)   /* if compressing stdin */
    return newname(w, 0);

  /* Allocate and copy pattern */
  if ((p = a = malloc(strlen(w) + 1)) == NULL)
    return ZE_MEM;
  strcpy(p, w);

  /* Normalize path delimiter as '/'. */
  for (q = p; *q; q++)                  /* use / consistently */
    if (*q == '\\')
      *q = '/';

  /* Only name can have special matching characters */
  if ((q = isshexp(p)) != NULL &&
      (strrchr(q, '/') != NULL || strrchr(q, ':') != NULL))
  {
    free((zvoid *)a);
    return ZE_PARMS;
  }

  /* Separate path and name into p and q */
  if ((q = strrchr(p, '/')) != NULL && (q == p || q[-1] != ':'))
  {
    *q++ = '\0';                        /* path/name -> path, name */
    if (*p == '\0')                     /* path is just / */
      p = strcpy(v, "/.");
  }
  else if ((q = strrchr(p, ':')) != NULL)
  {                                     /* has device and no or root path */
    *q++ = '\0';
    p = strcat(strcpy(v, p), ":");      /* copy device as path */
    if (*q == '/')                      /* -> device:/., name */
    {
      strcat(p, "/");
      q++;
    }
    strcat(p, ".");
  }
  else if (recurse && (strcmp(p, ".") == 0 ||  strcmp(p, "..") == 0))
  {                                    /* current or parent directory */
    /* I can't understand Mark's code so I am adding a hack here to get
     * "zip -r foo ." to work. Allow the dubious "zip -r foo .." but
     * reject "zip -rm foo ..".
     */
    if (dispose && strcmp(p, "..") == 0)
       ziperr(ZE_PARMS, "cannot remove parent directory");
    q = (char *)wild_match_all;
  }
  else                                  /* no path or device */
  {
    q = p;
    p = strcpy(v, ".");
  }
  if (recurse && *q == '\0') {
    q = (char *)wild_match_all;
  }
  /* Search that level for matching names */
  if ((r = initDirSearch(p, &d)) != ZE_OK)
  {
    free((zvoid *)a);
    return r;
  }
  if ((r = strlen(p)) > 1 &&
      (strcmp(p + r - 2, ":.") == 0 || strcmp(p + r - 2, "/.") == 0))
    *(p + r - 1) = '\0';
  f = 0;
  for (e = d.ff_name, ff_status = 0;
       ff_status == 0;
       ff_status = FNEXT(&d))
  {
    if (strcmp(e, ".") && strcmp(e, "..") && MATCH(q, e))
    {
      f = 1;
      if (strcmp(p, ".") == 0) {                /* path is . */
        r = procname(e);                        /* name is name */
        if (r) {
           f = 0;
           break;
        }
      } else
      {
        if ((n = malloc(strlen(p) + strlen(e) + 2)) == NULL)
        {
          free((zvoid *)a);
          return ZE_MEM;
        }
        n = strcpy(n, p);
        if (n[r = strlen(n) - 1] != '/' && n[r] != ':')
          strcat(n, "/");
        r = procname(strcat(n, e));             /* name is path/name */
        free((zvoid *)n);
        if (r) {
          f = 0;
          break;
        }
      }
    }
  }

  /* Done */
  free((zvoid *)a);
  return f ? ZE_OK : ZE_MISS;
}

int procname(n)
char *n;                /* name to process */
/* Process a name or sh expression to operate on (or exclude).  Return
   an error code in the ZE_ class. */
{
  char *a;              /* path and name for recursion */
  ff_dir *d;            /* control structure for FFIRST/FNEXT */
  char *e;              /* pointer to name from readd() */
  int m;                /* matched flag */
  int ff_status;        /* return value of FFIRST/FNEXT */
  char *p;              /* path for recursion */
  struct stat s;        /* result of stat() */
  struct zlist far *z;  /* steps through zfiles list */

  if (n == NULL)        /* volume_label request in freshen|delete mode ?? */
    return ZE_OK;

  if (strcmp(n, "-") == 0)   /* if compressing stdin */
    return newname(n, 0);
  else if (*n == '\0') return ZE_MISS;
  else if (LSSTAT(n, &s)
#if defined(__TURBOC__) || defined(__WATCOMC__)
           /* For these 2 compilers, stat() succeeds on wild card names! */
           || isshexp(n)
#endif
          )
  {
    /* Not a file or directory--search for shell expression in zip file */
    p = ex2in(n, 0, (int *)NULL);       /* shouldn't affect matching chars */
    m = 1;
    for (z = zfiles; z != NULL; z = z->nxt) {
      if (MATCH(p, z->zname))
      {
        z->mark = pcount ? filter(z->zname) : 1;
        if (verbose)
            fprintf(mesg, "zip diagnostic: %scluding %s\n",
               z->mark ? "in" : "ex", z->name);
        m = 0;
      }
    }
    free((zvoid *)p);
    return m ? ZE_MISS : ZE_OK;
  }

  /* Live name--use if file, recurse if directory */
  for (p = n; *p; p++)          /* use / consistently */
    if (*p == '\\')
      *p = '/';
  if ((s.st_mode & S_IFDIR) == 0)
  {
    /* add or remove name of file */
    if ((m = newname(n, 0)) != ZE_OK)
      return m;
  } else {
    /* Add trailing / to the directory name */
    if ((p = malloc(strlen(n)+2)) == NULL)
      return ZE_MEM;
    if (strcmp(n, ".") == 0 || strcmp(n, "/.") == 0) {
      *p = '\0';  /* avoid "./" prefix and do not create zip entry */
    } else {
      strcpy(p, n);
      a = p + strlen(p);
      if (a[-1] != '/')
        strcpy(a, "/");
      if (dirnames && (m = newname(p, 1)) != ZE_OK) {
        free((zvoid *)p);
        return m;
      }
    }
    /* recurse into directory */
    if (recurse)
    {
      if ((d = malloc(sizeof(ff_dir))) == NULL ||
          (m = initDirSearch(n, d)) == ZE_MEM)
      {
        if (d != NULL)
          free((zvoid *)d);
        free((zvoid *)p);
        return ZE_MEM;
      }
      for (e = d->ff_name, ff_status = m;
           ff_status == 0;
           ff_status = FNEXT(d))
      {
        if (strcmp(e, ".") && strcmp(e, ".."))
        {
          if ((a = malloc(strlen(p) + strlen(e) + 1)) == NULL)
          {
            free((zvoid *)d);
            free((zvoid *)p);
            return ZE_MEM;
          }
          strcat(strcpy(a, p), e);
          if ((m = procname(a)) != ZE_OK)   /* recurse on name */
          {
            if (m == ZE_MISS)
              zipwarn("name not matched: ", a);
            else
              ziperr(m, a);
          }
          free((zvoid *)a);
        }
      }
      free((zvoid *)d);
    }
    free((zvoid *)p);
  } /* (s.st_mode & S_IFDIR) == 0) */
  return ZE_OK;
}

char *ex2in(x, isdir, pdosflag)
char *x;                /* external file name */
int isdir;              /* input: x is a directory */
int *pdosflag;          /* output: force MSDOS file attributes? */
/* Convert the external file name to a zip file name, returning the malloc'ed
   string or NULL if not enough memory. */
{
  char *n;              /* internal file name (malloc'ed) */
  char *t;              /* shortened name */
  int dosflag;

  dosflag = 1;

  /* Find starting point in name before doing malloc */
  t = *x && *(x + 1) == ':' ? x + 2 : x;
  while (*t == '/' || *t == '\\')
    t++;

  /* Make changes, if any, to the copied name (leave original intact) */
  for (n = t; *n; n++)
    if (*n == '\\')
      *n = '/';

  if (!pathput)
    t = last(t, PATH_END);

  /* Malloc space for internal name and copy it */
  if ((n = malloc(strlen(t) + 1)) == NULL)
    return NULL;
  strcpy(n, t);

  if (isdir == 42) return n;      /* avoid warning on unused variable */

  if (dosify)
    msname(n);
  else
    strlwr(n);
  if (pdosflag)
    *pdosflag = dosflag;
  return n;
}

char *in2ex(n)
char *n;                /* internal file name */
/* Convert the zip file name to an external file name, returning the malloc'ed
   string or NULL if not enough memory. */
{
  char *x;              /* external file name */

  if ((x = malloc(strlen(n) + 1 + PAD)) == NULL)
      return NULL;
  strcpy(x, n);

  return x;
}

void stamp(f, d)
char *f;                /* name of file to change */
ulg d;                  /* dos-style time to change it to */
/* Set last updated and accessed time of file f to the DOS time d. */
{
#if defined(__TURBOC__) || defined(__GO32__)
  int h;                /* file handle */

  if ((h = open(f, 0)) != -1)
  {
    setftime(h, (struct ftime *)&d);
    close(h);
  }
#else /* !__TURBOC__ && !__GO32__ */
  ztimbuf u;            /* argument for utime() */

  /* Convert DOS time to time_t format in u.actime and u.modtime */
  u.actime = u.modtime = dos2unixtime(d);

  /* Set updated and accessed times of f */
  utime(f, &u);
#endif /* ?(__TURBOC__ || __GO32__) */
}

ulg filetime(f, a, n, t)
char *f;                /* name of file to get info on */
ulg *a;                 /* return value: file attributes */
long *n;                /* return value: file size */
ztimbuf *t;             /* return value: access and modification time */
/* If file *f does not exist, return 0.  Else, return the file's last
   modified date and time as an MSDOS date and time.  The date and
   time is returned in a long with the date most significant to allow
   unsigned integer comparison of absolute times.  Also, if a is not
   a NULL pointer, store the file attributes there, with the high two
   bytes being the Unix attributes, and the low byte being a mapping
   of that to DOS attributes.  If n is not NULL, store the file size
   there.  If t is not NULL, the file's access and modification time
   are stored there as UNIX time_t values.
   If f is "-", use standard input as the file. If f is a device, return
   a file size of -1 */
{
  struct stat s;        /* results of stat() */
  char name[FNMAX];
  int len = strlen(f), isstdin = !strcmp(f, "-");

  if (f == label) {
    if (a != NULL)
      *a = label_mode;
    if (n != NULL)
      *n = -2L; /* convention for a label name */
    if (t != NULL)
      t->actime = t->modtime = label_utim;
    return label_time;
  }
#if defined(__TURBOC__)
  /* Call tzset() to set timezone correctly (buggy older TC runtime libs) */
  tzset();
#endif /* __TURBOC__ */
  strcpy(name, f);
  if (name[len - 1] == '/')
    name[len - 1] = '\0';
  /* not all systems allow stat'ing a file with / appended */

  if (isstdin) {
    if (fstat(fileno(stdin), &s) != 0)
      error("fstat(stdin)");
    time((time_t *)&s.st_mtime);       /* some fstat()s return time zero */
  } else if (LSSTAT(name, &s) != 0)
             /* Accept about any file kind including directories
              * (stored with trailing / with -r option)
              */
    return 0;

  if (a != NULL)
    *a = ((ulg)s.st_mode << 16) | (isstdin ? 0L : (ulg)GetFileMode(name));
  if (n != NULL)
    *n = (s.st_mode & S_IFREG) != 0 ? s.st_size : -1L;
  if (t != NULL) {
    t->actime = s.st_atime;
    t->modtime = s.st_mtime;
  }

  return unix2dostime((time_t *)&s.st_mtime);
}

int deletedir(d)
char *d;                /* directory to delete */
/* Delete the directory *d if it is empty, do nothing otherwise.
   Return the result of rmdir(), delete(), or system().
 */
{
    return rmdir(d);
}

int set_extra_field(z, z_utim)
  struct zlist far *z;
  ztimbuf *z_utim;
  /* create extra field and change z->att if desired */
{
#ifdef USE_EF_UX_TIME
  if ((z->extra = (char *)malloc(EB_HEADSIZE+EB_UX_MINLEN)) == NULL)
    return ZE_MEM;

  z->extra[0]  = 'U';
  z->extra[1]  = 'X';
  z->extra[2]  = EB_UX_MINLEN;          /* length of data part of e.f. */
  z->extra[3]  = 0;
  z->extra[4]  = (char)(z_utim->actime);
  z->extra[5]  = (char)(z_utim->actime >> 8);
  z->extra[6]  = (char)(z_utim->actime >> 16);
  z->extra[7]  = (char)(z_utim->actime >> 24);
  z->extra[8]  = (char)(z_utim->modtime);
  z->extra[9]  = (char)(z_utim->modtime >> 8);
  z->extra[10] = (char)(z_utim->modtime >> 16);
  z->extra[11] = (char)(z_utim->modtime >> 24);

  z->cext = z->ext = (EB_HEADSIZE+EB_UX_MINLEN);
  z->cextra = z->extra;

  return ZE_OK;
#else /* !USE_EF_UX_TIME */
  return (int)(z-z);
#endif /* ?USE_EF_UX_TIME */
}

/******************************/
/*  Function version_local()  */
/******************************/

static const char CompiledWith[] = "Compiled with %s%s for %s%s%s%s.\n\n";
                        /* At module level to keep Turbo C++ 1.0 happy !! */

void version_local()
{
#if defined(__DJGPP__) || defined(__WATCOMC__) || \
    (defined(_MSC_VER) && (_MSC_VER != 800))
    char buf[80];
#endif

    printf(CompiledWith,

#ifdef __GNUC__
#  ifdef __DJGPP__
      (sprintf(buf, "djgpp v%d / gcc ", __DJGPP__), buf),
#  else
#  ifdef __GO32__           /* __GO32__ is defined as "1" only (sigh) */
      "djgpp v1.x / gcc ",
#  else
#  ifdef __EMX__            /* ...so is __EMX__ (double sigh) */
      "emx+gcc ",
#  else
      "gcc ",
#  endif
#  endif
#  endif
      __VERSION__,
#else
#ifdef __WATCOMC__
#  if (__WATCOMC__ % 10 > 0)
/* We do this silly test because __WATCOMC__ gives two digits for the  */
/* minor version, but Watcom packaging prefers to show only one digit. */
      (sprintf(buf, "Watcom C/C++ %d.%02d", __WATCOMC__ / 100,
               __WATCOMC__ % 100), buf), "",
#  else
      (sprintf(buf, "Watcom C/C++ %d.%d", __WATCOMC__ / 100,
               (__WATCOMC__ % 100) / 10), buf), "",
#  endif
#else
#ifdef __TURBOC__
#  ifdef __BORLANDC__
      "Borland C++",
#    if (__BORLANDC__ < 0x0200)
        " 1.0",
#    else
#    if (__BORLANDC__ == 0x0200)   /* James:  __TURBOC__ = 0x0297 */
        " 2.0",
#    else
#    if (__BORLANDC__ == 0x0400)
        " 3.0",
#    else
#    if (__BORLANDC__ == 0x0410)   /* __BCPLUSPLUS__ = 0x0310 */
        " 3.1",
#    else
#    if (__BORLANDC__ == 0x0452)   /* __BCPLUSPLUS__ = 0x0320 */
        " 4.0 or 4.02",
#    else
#    if (__BORLANDC__ == 0x0460)   /* __BCPLUSPLUS__ = 0x0340 */
        " 4.5",
#    else
#    if (__BORLANDC__ == 0x0500)   /* __TURBOC__ = 0x0500 */
        " 5.0",
#    else
        " later than 5.0",
#    endif
#    endif
#    endif
#    endif
#    endif
#    endif
#    endif
#  else
      "Turbo C",
#    if (__TURBOC__ >= 0x0400)     /* Kevin:  3.0 -> 0x0401 */
        "++ 3.0 or later",
#    else
#    if (__TURBOC__ == 0x0295)     /* [661] vfy'd by Kevin */
        "++ 1.0",
#    else
#    if ((__TURBOC__ >= 0x018d) && (__TURBOC__ <= 0x0200))  /* James: 0x0200 */
        " 2.0",
#    else
#    if (__TURBOC__ > 0x0100)
        " 1.5",                    /* James:  0x0105? */
#    else
        " 1.0",                    /* James:  0x0100 */
#    endif
#    endif
#    endif
#    endif
#  endif
#else
#ifdef MSC
      "Microsoft C ",
#  ifdef _MSC_VER
#    if (_MSC_VER == 800)
        "8.0/8.0c (Visual C++ 1.0/1.5)",
#    else
        (sprintf(buf, "%d.%02d", _MSC_VER/100, _MSC_VER%100), buf),
#    endif
#  else
      "5.1 or earlier",
#  endif
#else
      "unknown compiler", "",
#endif /* MSC */
#endif /* __TURBOC__ */
#endif /* __WATCOMC__ */
#endif /* __GNUC__ */

      "MS-DOS",

#if (defined(__GNUC__) || (defined(__WATCOMC__) && defined(__386__)))
      " (32-bit)",
#else
#  if defined(M_I86HM) || defined(__HUGE__)
      " (16-bit, huge)",
#  else
#  if defined(M_I86LM) || defined(__LARGE__)
      " (16-bit, large)",
#  else
#  if defined(M_I86MM) || defined(__MEDIUM__)
      " (16-bit, medium)",
#  else
#  if defined(M_I86CM) || defined(__COMPACT__)
      " (16-bit, compact)",
#  else
#  if defined(M_I86SM) || defined(__SMALL__)
      " (16-bit, small)",
#  else
#  if defined(M_I86TM) || defined(__TINY__)
      " (16-bit, tiny)",
#  else
      " (16-bit)",
#  endif
#  endif
#  endif
#  endif
#  endif
#  endif
#endif

#ifdef __DATE__
      " on ", __DATE__
#else
      "", ""
#endif
    );

    /* temporary debugging code for Borland compilers only */
#ifdef __TURBOC__
    printf("\tdebug(__TURBOC__ = 0x%04x = %d)\n", __TURBOC__, __TURBOC__);
#ifdef __BORLANDC__
    printf("\tdebug(__BORLANDC__ = 0x%04x)\n", __BORLANDC__);
#else
    printf("\tdebug(__BORLANDC__ not defined)\n");
#endif
#ifdef __TCPLUSPLUS__
    printf("\tdebug(__TCPLUSPLUS__ = 0x%04x)\n", __TCPLUSPLUS__);
#else
    printf("\tdebug(__TCPLUSPLUS__ not defined)\n");
#endif
#ifdef __BCPLUSPLUS__
    printf("\tdebug(__BCPLUSPLUS__ = 0x%04x)\n\n", __BCPLUSPLUS__);
#else
    printf("\tdebug(__BCPLUSPLUS__ not defined)\n\n");
#endif
#endif

} /* end function version_local() */


#ifdef MY_ZCALLOC       /* Special zcalloc function for MEMORY16 (MSDOS/OS2) */

#if defined(__TURBOC__) && !defined(OS2)
/* Small and medium model are for now limited to near allocation with
 * reduced MAX_WBITS and MAX_MEM_LEVEL
 */

/* Turbo C malloc() does not allow dynamic allocation of 64K bytes
 * and farmalloc(64K) returns a pointer with an offset of 8, so we
 * must fix the pointer. Warning: the pointer must be put back to its
 * original form in order to free it, use zcfree().
 */

#define MAX_PTR 10
/* 10*64K = 640K */

local int next_ptr = 0;

typedef struct ptr_table_s {
    zvoid far *org_ptr;
    zvoid far *new_ptr;
} ptr_table;

local ptr_table table[MAX_PTR];
/* This table is used to remember the original form of pointers
 * to large buffers (64K). Such pointers are normalized with a zero offset.
 * Since MSDOS is not a preemptive multitasking OS, this table is not
 * protected from concurrent access. This hack doesn't work anyway on
 * a protected system like OS/2. Use Microsoft C instead.
 */

zvoid far *zcalloc (unsigned items, unsigned size)
{
    zvoid far *buf;
    ulg bsize = (ulg)items*size;

    if (bsize < (65536L-16L)) {
        buf = farmalloc(bsize);
        if (*(ush*)&buf != 0) return buf;
    } else {
        buf = farmalloc(bsize + 16L);
    }
    if (buf == NULL || next_ptr >= MAX_PTR) return NULL;
    table[next_ptr].org_ptr = buf;

    /* Normalize the pointer to seg:0 */
    *((ush*)&buf+1) += ((ush)((uch*)buf-NULL) + 15) >> 4;
    *(ush*)&buf = 0;
    table[next_ptr++].new_ptr = buf;
    return buf;
}

zvoid zcfree (zvoid far *ptr)
{
    int n;
    if (*(ush*)&ptr != 0) { /* object < 64K */
        farfree(ptr);
        return;
    }
    /* Find the original pointer */
    for (n = next_ptr - 1; n >= 0; n--) {
        if (ptr != table[n].new_ptr) continue;

        farfree(table[n].org_ptr);
        while (++n < next_ptr) {
            table[n-1] = table[n];
        }
        next_ptr--;
        return;
    }
    ziperr(ZE_MEM, "zcfree: ptr not found");
}
#endif /* __TURBOC__ */

#if defined(MSC) || defined(__WATCOMC__)
#if (!defined(_MSC_VER) || (_MSC_VER < 600))
#  define _halloc  halloc
#  define _hfree   hfree
#endif

zvoid far *zcalloc (unsigned items, unsigned size)
{
    return (zvoid far *)_halloc((long)items, size);
}

zvoid zcfree (zvoid far *ptr)
{
    _hfree((void huge *)ptr);
}
#endif /* MSC || __WATCOMC__ */

#endif /* MY_ZCALLOC */

#if (defined(__WATCOMC__) && defined(ASMV) && !defined(__386__))
/* This is a hack to connect "call _exit" in match.asm to exit() */
#pragma aux xit "_exit" parm caller []
void xit(void)
{
    exit(20);
}
#endif

#endif /* !UTIL */
