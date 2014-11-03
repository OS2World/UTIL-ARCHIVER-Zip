/*

 Copyright (C) 1990-1996 Mark Adler, Richard B. Wales, Jean-loup Gailly,
 Kai Uwe Rommel, Onno van der Linden, George Petrov and Igor Mandrichenko.
 Permission is granted to any individual or institution to use, copy, or
 redistribute this software so long as all of the original files are included,
 that it is not sold for profit, and that this copyright notice is retained.

*/

/*
 * routines common to VM/CMS and MVS
 */

#include "zip.h"

#include <time.h>

#define MATCH shmatch


#define PAD 0
#define PATH_END '/'

/* Library functions not in (most) header files */

#ifndef UTIL    /* the companion #endif is a bit of ways down ... */

int utime OF((char *, ztimbuf *));

extern char *label;
local ulg label_time = 0;
local ulg label_mode = 0;
local time_t label_utim = 0;

int stat(const char *path, struct stat *buf)
{
   if ((buf->fp = fopen(path, "r")) != NULL) {
      fldata_t fdata;
      if (fldata( buf->fp, buf->fname, &fdata ) == 0) {
         buf->st_dev  = fdata.__device;
         buf->st_mode = *(short *)(&fdata);
      }
      strcpy( buf->fname, path );
      fclose(buf->fp);
   }
   return (buf->fp != NULL ? 0 : 1);
}

int fstat(int fd, struct stat *buf)
{
   fldata_t fdata;

   if ((fd != -1) && (fldata( (FILE *)fd, buf->fname, &fdata ) == 0)) {
      buf->st_dev  = fdata.__device;
      buf->st_mode = *(short *)(&fdata);
      buf->fp      = (FILE *)fd;
      return 0;
   }
   return -1;
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
  char mem[10] = "";    /* member name */
  char ext[10] = "";     /* extension name */

  dosflag = dosify;  /* default for non-DOS non-OS/2 */

  /* Find starting point in name before doing malloc */
  for (t = x; *t == '/'; t++)
    ;

  /* Make changes, if any, to the copied name (leave original intact) */
  if (!pathput)
    t = last(t, PATH_END);

  /* Malloc space for internal name and copy it */
  if ((n = malloc(strlen(t) + 1)) == NULL)
    return NULL;
  strcpy(n, t);
  if (t = strrchr(n, '(')) {
     *t = '\0';
     strcpy(mem,t+1);
     if (t = strchr(mem, ')')) *t = '\0';
     if (t = strrchr(n, '.')) t++;
     else t = n;
     strcpy(ext,t);
     strcpy(t,mem);
     strcat(t,".");
     strcat(t,ext);
  }
  if (t = strrchr(n, '.')) {
     while (--t > n)
        if (*t == '.')
          *t = '/';
  }
  n = strtoasc(n,t);

  if (isdir == 42) return n;      /* avoid warning on unused variable */

  if (dosify)
    msname(n);

  /* Returned malloc'ed name */
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
  strtoebc(x, n);
  return x;
}

void stamp(f, d)
char *f;                /* name of file to change */
ulg d;                  /* dos-style time to change it to */
/* Set last updated and accessed time of file f to the DOS time d. */
{
  ztimbuf u;            /* argument for utime() */

  /* Convert DOS time to time_t format in u.actime and u.modtime */
  u.actime = u.modtime = dos2unixtime(d);

  utime(f, &u);
}

ulg filetime(f, a, n, t)
char *f;                /* name of file to get info on */
ulg *a;                 /* return value: file attributes */
long *n;                /* return value: file size */
ztimbuf *t;             /* return value: access and modification time */
{
  FILE *stream;
  time_t ltime;

  if (strcmp(f, "-") != 0) {    /* if not compressing stdin */
     if ((stream = fopen(f, "r")) == (FILE *)NULL) {
        return 0;
     } else {
        if (n != NULL) {
           *n = -1L;
/*         fseek(stream,0L,SEEK_END);
           *n = ftell(stream);  - don't work with "r" */
        }
        fclose(stream);
     }
  }
  else {
     if (n != NULL) {
        *n = -1L;
     }
  }

  time(&ltime);
  if (t != NULL)
     t->actime = t->modtime = ltime;

  return unix2dostime(&ltime);
}

int set_extra_field(z, z_utim)
struct zlist far *z;
ztimbuf *z_utim;
/* create extra field and change z->att if desired */
{
   fldata_t fdata;
   FILE *stream;
   char fname[65];
   char type[60];
   char *eb_ptr;
#ifdef USE_EF_UX_TIME
   extent ef_l_len = (EB_HEADSIZE+EB_UX_MINLEN);
#else /* !USE_EF_UX_TIME */
   extent ef_l_len = 0;
#endif /* ?USE_EF_UX_TIME */
   int set_cmsmvs_eb = 0;

/*translate_eol = 0;*/
  if (aflag == ASCII) {
     z->att = ASCII;
  } else {
    if (bflag)
      z->att = BINARY;
    else
      z->att = __EBCDIC;
    ef_l_len += sizeof(fdata)+EB_HEADSIZE;
    set_cmsmvs_eb = 1;
  }

  if (ef_l_len > 0) {
    z->extra = (char *)malloc(ef_l_len);
    if (z->extra == NULL) {
       printf("FLDATA : Unable to allocate memory !\n");
       return ZE_MEM;
    }
    z->cext = z->ext = ef_l_len;
    eb_ptr = z->cextra = z->extra;

    if (set_cmsmvs_eb) {
      strtoebc(fname,z->zname);
      if (bflag)
        stream = fopen(fname,"rb,type=record");
      else
        stream = fopen(fname,"r");
      if (stream == NULL) {
        printf("FLDATA : Could not open file : %s !\n",fname);
        return ZE_NONE;
      }

      fldata(stream,fname,&fdata);
      /*put the system ID */
#ifdef VM_CMS
      *(eb_ptr) = EF_VMCMS & 0xFF;
      *(eb_ptr+1) = EF_VMCMS >> 8;
#else
      *(eb_ptr) = EF_MVS & 0xFF;
      *(eb_ptr+1) = EF_MVS >> 8;
#endif
      *(eb_ptr+2) = sizeof(fdata) & 0xFF;
      *(eb_ptr+3) = sizeof(fdata) >> 8;

      memcpy(eb_ptr+EB_HEADSIZE,&fdata,sizeof(fdata));
      fclose(stream);
#ifdef USE_EF_UX_TIME
      eb_ptr += (sizeof(fdata)+EB_HEADSIZE);
#endif /* USE_EF_UX_TIME */
    }
#ifdef USE_EF_UX_TIME
    eb_ptr[0]  = 'U';
    eb_ptr[1]  = 'X';
    eb_ptr[2]  = EB_UX_MINLEN;          /* length of data part of e.f. */
    eb_ptr[3]  = 0;
    eb_ptr[4]  = (char)(z_utim->actime);
    eb_ptr[5]  = (char)(z_utim->actime >> 8);
    eb_ptr[6]  = (char)(z_utim->actime >> 16);
    eb_ptr[7]  = (char)(z_utim->actime >> 24);
    eb_ptr[8]  = (char)(z_utim->modtime);
    eb_ptr[9]  = (char)(z_utim->modtime >> 8);
    eb_ptr[10] = (char)(z_utim->modtime >> 16);
    eb_ptr[11] = (char)(z_utim->modtime >> 24);
#endif /* USE_EF_UX_TIME */
  }

  return ZE_OK;
}

int deletedir(d)
char *d;                /* directory to delete */
/* Delete the directory *d if it is empty, do nothing otherwise.
   Return the result of rmdir(), delete(), or system().
   For VMS, d must be in format [x.y]z.dir;1  (not [x.y.z]).
 */
{
#ifdef VM_CMS
    return 0;
#else
    return rmdir(d);
#endif /* VM_CMS */
}

void version_local()
{
    printf("Compiled with %s under %s.\n", "C/370 2.1",
#ifdef VM_CMS
      "VM/CMS"
#else
      "MVS"
#endif
    );
}

#endif /* !UTIL */
