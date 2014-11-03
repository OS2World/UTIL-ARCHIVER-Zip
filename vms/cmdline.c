/*

 Copyright (C) 1990-1996 Mark Adler, Richard B. Wales, Jean-loup Gailly,
 Kai Uwe Rommel, Christian Spieler, Onno van der Linden and Igor Mandrichenko.
 Permission is granted to any individual or institution to use, copy, or
 redistribute this software so long as all of the original files are included,
 that it is not sold for profit, and that this copyright notice is retained.

*/

#define module_name VMS_ZIP_CMDLINE
#define module_ident "02-004"
/*
**
**  Facility:   ZIP
**
**  Module:     VMS_ZIP_CMDLINE
**
**  Author:     Hunter Goatley <goathunter@WKUVX1.WKU.EDU>
**
**  Date:       July 30, 1993
**
**  Abstract:   Routines to handle a VMS CLI interface for Zip.  The CLI
**              command line is parsed and a new argc/argv are built and
**              returned to zip.
**
**  Modified by:
**
**      02-004          Onno van der Linden,
**                      Christian Spieler       13-APR-1996 20:05
**              Removed /ENCRYPT=VERIFY ("-ee" option).
**      02-003          Christian Spieler       11-FEB-1996 23:05
**              Added handling of /EXTRAFIELDS qualifier ("-X" option).
**      02-002          Christian Spieler       09-JAN-1996 22:25
**              Added "#include crypt.h", corrected typo.
**      02-001          Christian Spieler       04-DEC-1995 16:00
**              Fixed compilation in DEC CC's ANSI mode.
**      02-000          Christian Spieler       10-OCT-1995 17:54
**              Modified for Zip v2.1, added several new options.
**      01-000          Hunter Goatley          30-JUL-1993 07:54
**              Original version (for Zip v1.9p1).
**
*/


#ifdef __DECC
#pragma module module_name module_ident
#else
#module module_name module_ident
#endif

#include "zip.h"
#include "crypt.h"
#ifndef NOCPYRT
# define NOCPYRT
#endif
#include "revision.h"   /* for help() */

#include <ssdef.h>
#include <descrip.h>
#include <climsgdef.h>
#include <clidef.h>
#include <lib$routines.h>
#include <ots$routines.h>
#include <str$routines.h>

#ifndef CLI$_COMMA
globalvalue CLI$_COMMA;
#endif

/*
**  "Macro" to initialize a dynamic string descriptor.
*/
#define init_dyndesc(dsc) {\
        dsc.dsc$w_length = 0;\
        dsc.dsc$b_dtype = DSC$K_DTYPE_T;\
        dsc.dsc$b_class = DSC$K_CLASS_D;\
        dsc.dsc$a_pointer = NULL;}

/*
**  Define descriptors for all of the CLI parameters and qualifiers.
*/
$DESCRIPTOR(cli_delete,         "DELETE");              /* -d */
$DESCRIPTOR(cli_freshen,        "FRESHEN");             /* -f */
$DESCRIPTOR(cli_move,           "MOVE");                /* -m */
$DESCRIPTOR(cli_update,         "UPDATE");              /* -u */
$DESCRIPTOR(cli_exclude,        "EXCLUDE");             /* -x */
$DESCRIPTOR(cli_include,        "INCLUDE");             /* -i */
$DESCRIPTOR(cli_adjust,         "ADJUST_OFFSETS");      /* -A */
$DESCRIPTOR(cli_append,         "APPEND");              /* -g */
$DESCRIPTOR(cli_batch,          "BATCH");               /* -@ */
$DESCRIPTOR(cli_comments,       "COMMENTS");            /* -c,-z */
$DESCRIPTOR(cli_comment_zipfile,"COMMENTS.ZIP_FILE");   /* -z */
$DESCRIPTOR(cli_comment_files,  "COMMENTS.FILES");      /* -c */
$DESCRIPTOR(cli_dirnames,       "DIRNAMES");            /* -D */
#ifdef CRYPT
$DESCRIPTOR(cli_encrypt,        "ENCRYPT");             /* -e */
#endif /* CRYPT */
$DESCRIPTOR(cli_extra_fields,   "EXTRA_FIELDS");        /* -X */
$DESCRIPTOR(cli_fix_archive,    "FIX_ARCHIVE");         /* -F[F] */
$DESCRIPTOR(cli_fix_normal,     "FIX_ARCHIVE.NORMAL");  /* -F */
$DESCRIPTOR(cli_fix_full,       "FIX_ARCHIVE.FULL");    /* -FF */
$DESCRIPTOR(cli_full_path,      "FULL_PATH");           /* -p */
$DESCRIPTOR(cli_help,           "HELP");                /* -h */
$DESCRIPTOR(cli_junk,           "JUNK");                /* -j */
$DESCRIPTOR(cli_keep_version,   "KEEP_VERSION");        /* -w */
$DESCRIPTOR(cli_latest,         "LATEST");              /* -o */
$DESCRIPTOR(cli_level,          "LEVEL");               /* -[0-9] */
$DESCRIPTOR(cli_license,        "LICENSE");             /* -L */
$DESCRIPTOR(cli_pkzip,          "PKZIP");               /* -k */
$DESCRIPTOR(cli_quiet,          "QUIET");               /* -q */
$DESCRIPTOR(cli_recurse,        "RECURSE");             /* -r */
$DESCRIPTOR(cli_since,          "SINCE");               /* -t */
$DESCRIPTOR(cli_store_types,    "STORE_TYPES");         /* -n */
$DESCRIPTOR(cli_temp_path,      "TEMP_PATH");           /* -b */
$DESCRIPTOR(cli_test,           "TEST");                /* -T */
$DESCRIPTOR(cli_translate_eol,  "TRANSLATE_EOL");       /* -l[l] */
$DESCRIPTOR(cli_transl_eol_lf,  "TRANSLATE_EOL.LF");    /* -l */
$DESCRIPTOR(cli_transl_eol_crlf,"TRANSLATE_EOL.CRLF");  /* -ll */
$DESCRIPTOR(cli_unsfx,          "UNSFX");               /* -J */
$DESCRIPTOR(cli_verbose,        "VERBOSE");             /* -v */
$DESCRIPTOR(cli_verbose_more,   "VERBOSE.MORE");        /* -vv */
$DESCRIPTOR(cli_verbose_debug,  "VERBOSE.DEBUG");       /* -vvv */
$DESCRIPTOR(cli_vms,            "VMS");                 /* -V */

$DESCRIPTOR(cli_yyz,            "YYZ_ZIP");

$DESCRIPTOR(cli_zipfile,        "ZIPFILE");
$DESCRIPTOR(cli_infile,         "INFILE");
$DESCRIPTOR(zip_command,        "zip ");

#ifdef __DECC
extern void *zip_clitable;
#else
globalref void *zip_clitable;
#endif

/* extern unsigned long LIB$GET_INPUT(void), LIB$SIG_TO_RET(void); */

#ifndef __STARLET_LOADED
extern int sys$bintim ();
extern int sys$numtim ();
#endif
extern unsigned long cli$dcl_parse ();
extern unsigned long cli$present ();
extern unsigned long cli$get_value ();

unsigned long vms_zip_cmdline (int *, char ***);
unsigned long get_list (struct dsc$descriptor_s *, char **,
                        struct dsc$descriptor_d *, char);
unsigned long check_cli (struct dsc$descriptor_s *);


#ifdef TEST
unsigned long
main(int argc, char **argv)
{
    register status;
    return (vms_zip_cmdline(&argc, &argv));
}
#endif /* TEST */


unsigned long
vms_zip_cmdline (int *argc_p, char ***argv_p)
{
/*
**  Routine:    vms_zip_cmdline
**
**  Function:
**
**      Parse the DCL command line and create a fake argv array to be
**      handed off to Zip.
**
**      NOTE: the argv[] is built as we go, so all the parameters are
**      checked in the appropriate order!!
**
**  Formal parameters:
**
**      argc_p          - Address of int to receive the new argc
**      argv_p          - Address of char ** to receive the argv address
**
**  Calling sequence:
**
**      status = vms_zip_cmdline (&argc, &argv);
**
**  Returns:
**
**      SS$_NORMAL      - Success.
**      SS$_INSFMEM     - A malloc() or realloc() failed
**      SS$_ABORT       - Bad time value
**
*/
    register status;
    char options[48];
    char *the_cmd_line;
    char *ptr;
    int  x, len;

    int new_argc;
    char **new_argv;

    struct dsc$descriptor_d work_str;
    struct dsc$descriptor_d foreign_cmdline;

    init_dyndesc (work_str);
    init_dyndesc (foreign_cmdline);

    /*
    **  See if the program was invoked by the CLI (SET COMMAND) or by
    **  a foreign command definition.  Check for /YYZ_ZIP, which is a
    **  valid default qualifier solely for this test.
    */
    status = check_cli (&cli_yyz);
    if (!(status & 1)) {
        lib$get_foreign (&foreign_cmdline);
        /*
        **  If nothing was returned or the first character is a "-", then
        **  assume it's a UNIX-style command and return.
        */
        if ((foreign_cmdline.dsc$w_length == 0) ||
            (*(foreign_cmdline.dsc$a_pointer) == '-'))
            return(SS$_NORMAL);

        str$concat (&work_str, &zip_command, &foreign_cmdline);
        status = cli$dcl_parse(&work_str, &zip_clitable, lib$get_input,
                        lib$get_input, 0);
        if (!(status & 1)) return(status);
    }

    /*
    **  There's always going to be a new_argv[] because of the image name.
    */
    if ((the_cmd_line = (char *) malloc (sizeof("zip")+1)) == NULL)
        return(SS$_INSFMEM);

    strcpy (the_cmd_line, "zip");

    /*
    **  First, check to see if any of the regular options were specified.
    */

    options[0] = '-';
    ptr = &options[1];          /* Point to temporary buffer */

    /*
    **  Delete the specified files from the zip file?
    */
    status = cli$present (&cli_delete);
    if (status & 1)
        *ptr++ = 'd';

    /*
    **  Freshen (only changed files).
    */
    status = cli$present (&cli_freshen);
    if (status & 1)
        *ptr++ = 'f';

    /*
    **  Delete the files once they've been added to the zip file.
    */
    status = cli$present (&cli_move);
    if (status & 1)
        *ptr++ = 'm';

    /*
    **  Add changed and new files.
    */
    status = cli$present (&cli_update);
    if (status & 1)
        *ptr++ = 'u';

    /*
    **  Check for the compression level (-0 through -9).
    */
    status = cli$present(&cli_level);
    if (status & 1) {

        unsigned long binval;

        status = cli$get_value (&cli_level, &work_str);
        status = ots$cvt_tu_l (&work_str, &binval);
        if (!(status & 1) || (binval > 9)) {
           return(SS$_ABORT);
        }
        *ptr++ = binval + '0';
    }

    /*
    **  Adjust offsets of zip archive entries.
    */
    status = cli$present(&cli_adjust);
    if (status & 1)
        *ptr++ = 'A';

    /*
    **  Add comments?
    */
    status = cli$present (&cli_comments);
    if (status & 1) {
/*        while ((status = cli$get_value (&cli_comments, &work_str)) & 1) {
            if (strncmp(work_str.dsc$a_pointer,"ZIP",3) == 0)
                *ptr++ = 'z';
            if (strncmp(work_str.dsc$a_pointer,"FIL",3) == 0)
                *ptr++ = 'c';
        } */
        if ((status = cli$present (&cli_comment_zipfile)) & 1)
            *ptr++ = 'z';
        if ((status = cli$present (&cli_comment_files)) & 1)
            *ptr++ = 'c';
    }

    /*
    **  Do not add/modify directory entries.
    */
    status = cli$present(&cli_dirnames);
    if (!(status & 1))
        *ptr++ = 'D';

#ifdef CRYPT
    /*
    **  Encrypt?
    */
    status = cli$present (&cli_encrypt);
    if (status & 1)
        *ptr++ = 'e';
#endif /* CRYPT */

    /*
    **  Fix the zip archive structure.
    */
    status = cli$present (&cli_fix_archive);
    if (status & 1) {
        *ptr++ = 'F';
        if ((status = cli$present (&cli_fix_full)) & 1) {
            *ptr++ = 'F';
        }
    }

    /*
    **  Append (allow growing of existing zip file).
    */
    status = cli$present (&cli_append);
    if (status & 1)
        *ptr++ = 'g';

    /*
    **  Show the help.
    */
    status = cli$present (&cli_help);
    if (status & 1)
        *ptr++ = 'h';

    /*
    **  Junk path names (directory specs).
    */
    status = cli$present (&cli_junk);
    if (status & 1)
        *ptr++ = 'j';

    /*
    **  Simulate zip file made by PKZIP.
    */
    status = cli$present (&cli_pkzip);
    if (status & 1)
        *ptr++ = 'k';

    /*
    **  Translage end-of-line.
    */
    status = cli$present (&cli_translate_eol);
    if (status & 1) {
        *ptr++ = 'l';
        if ((status = cli$present (&cli_transl_eol_crlf)) & 1) {
            *ptr++ = 'l';
        }
    }

    /*
    **  Show the software license.
    */
    status = cli$present (&cli_license);
    if (status & 1)
        *ptr++ = 'L';

    /*
    **  Set zip file time to time of latest file in it.
    */
    status = cli$present (&cli_latest);
    if (status & 1)
        *ptr++ = 'o';

    /*
    **  Store full path (default).
    */
    status = cli$present (&cli_full_path);
    if (status == CLI$_PRESENT)
        *ptr++ = 'p';
    else if (status == CLI$_NEGATED)
        *ptr++ = 'j';

    /*
    **  Junk Zipfile prefix (SFX stub etc.).
    */
    status = cli$present (&cli_unsfx);
    if (status & 1)
        *ptr++ = 'J';

    /*
    **  Recurse through subdirectories.
    */
    status = cli$present (&cli_recurse);
    if (status & 1)
        *ptr++ = 'r';

    /*
    **  Be verbose.
    */
    status = cli$present (&cli_verbose);
    if (status & 1) {
        *ptr++ = 'v';
        if ((status = cli$present (&cli_verbose_more)) & 1)
            *ptr++ = 'v';
        if ((status = cli$present (&cli_verbose_debug)) & 1) {
            *ptr++ = 'v';
            *ptr++ = 'v';
        }
    }

    /*
    **  Quiet mode.
    **  (Quiet mode is processed after verbose, because a "-v" modifier
    **  resets "noisy" to 1.)
    */
    status = cli$present (&cli_quiet);
    if (status & 1)
        *ptr++ = 'q';

    /*
    **  Suppress creation of any extra field.
    */
    status = cli$present (&cli_extra_fields);
    if (!(status & 1))
        *ptr++ = 'X';

    /*
    **  Save the VMS file attributes.
    */
    status = cli$present (&cli_vms);
    if (status & 1)
        *ptr++ = 'V';

    /*
    **  Keep the VMS version number as part of the file name when stored.
    */
    status = cli$present (&cli_keep_version);
    if (status & 1)
        *ptr++ = 'w';

    /*
    **  `Batch' processing: read filenames to archive from stdin
    **  or the specified file.
    */
    status = cli$present (&cli_batch);
    if (status & 1) {
               status = cli$get_value (&cli_batch, &work_str);
        if (status & 1) {
            work_str.dsc$a_pointer[work_str.dsc$w_length] = '\0';
            if ((stdin = freopen(work_str.dsc$a_pointer, "r", stdin)) == NULL)
            {
                sprintf(errbuf, "could not open list file: %s",
                        work_str.dsc$a_pointer);
                ziperr(ZE_PARMS, errbuf);
            }
        }
        *ptr++ = '@';
    }

    /*
    **  Now copy the final options string to the_cmd_line.
    */
    x = ptr - &options[0];
    if (x > 1) {
        options[x] = '\0';
        len = strlen(the_cmd_line) + x + 2;
        if ((the_cmd_line = (char *) realloc (the_cmd_line, len)) == NULL)
            return(SS$_INSFMEM);
        strcat (the_cmd_line, " ");
        strcat (the_cmd_line, options);
    }

    /*
    **
    **  OK.  We've done all the regular options, so check for -b (temporary
    **  file path), -n (special suffixes), zipfile, files to zip, and exclude
    **  list.
    **
    */
    status = cli$present (&cli_temp_path);
    if (status & 1) {
        status = cli$get_value (&cli_temp_path, &work_str);
        len = strlen(the_cmd_line) + work_str.dsc$w_length + 4 + 1;
        if ((the_cmd_line = (char *) realloc (the_cmd_line, len)) == NULL)
            return(SS$_INSFMEM);
        strcat (the_cmd_line, " -b ");
        x = strlen(the_cmd_line);
        strncpy(&the_cmd_line[x], work_str.dsc$a_pointer,
                work_str.dsc$w_length);
        the_cmd_line[len] = '\0';
    }

    /*
    **  Handle "-t mmddyy".
    */
    status = cli$present (&cli_since);
    if (status & 1) {
#ifdef __DECC
#pragma member_alignment save
#pragma nomember_alignment
#endif  /* __DECC */

        struct tim {
            short year;
            short month;
            short day;
            short hour;
            short minute;
            short second;
            short hundred;
        } numtimbuf;
        struct quadword {
            long high;
            long low;
        } bintimbuf = {0,0};
        struct dsc$descriptor_d since_time;
#ifdef __DECC
#pragma member_alignment restore
#endif
        init_dyndesc(since_time);
        status = cli$get_value (&cli_since, &since_time);
        /*
        **  If a date is given, convert it to 64-bit binary.
        */
        if (since_time.dsc$w_length) {
            status = sys$bintim (&since_time, &bintimbuf);
            if (!(status & 1)) return(status);
            str$free1_dx (&since_time);
        }
        /*
        **  Now call $NUMTIM to get the month, day, and year.
        */
        status = sys$numtim (&numtimbuf, (bintimbuf.low ? &bintimbuf : NULL));
        if (!(status & 1)) return(status);

        /*
        **  Now let's add the option "-t mmddyy" to the new command line.
        */
        len = strlen(the_cmd_line) + 4 + 6 + 1;
        if ((the_cmd_line = (char *) realloc (the_cmd_line, len)) == NULL)
            return(SS$_INSFMEM);
        strcat (the_cmd_line, " -t ");
        x = strlen(the_cmd_line);
        sprintf(&the_cmd_line[x], "%02d%02d%02d", numtimbuf.month,
                numtimbuf.day,
                ((numtimbuf.year < 2000) ? numtimbuf.year - 1900 :
                        numtimbuf.year - 2000));
    }

    /*
    **  Handle "-n suffix:suffix:...".  (File types to store only.)
    */
    status = cli$present (&cli_store_types);
    if (status & 1) {
        len = strlen(the_cmd_line) + 4 + 1;
        if ((the_cmd_line = (char *) realloc (the_cmd_line, len)) == NULL)
            return(SS$_INSFMEM);
        strcat (the_cmd_line, " -n ");

        status = get_list (&cli_store_types, &the_cmd_line,
                           &foreign_cmdline, ':');
        if (!(status & 1)) return (status);
    }

    /*
    **  Now get the specified zip file name.
    */
    status = cli$present (&cli_zipfile);
    if (status & 1) {
        status = cli$get_value (&cli_zipfile, &work_str);

        len = strlen(the_cmd_line) + work_str.dsc$w_length + 2;
        if ((the_cmd_line = (char *) realloc (the_cmd_line, len)) == NULL)
            return(SS$_INSFMEM);
        strcat (the_cmd_line, " ");
        x = strlen(the_cmd_line);
        strncpy(&the_cmd_line[x], work_str.dsc$a_pointer,
                work_str.dsc$w_length);
        the_cmd_line[len] = '\0';

    }

    /*
    **  Run through the list of input files.
    */
    status = cli$present (&cli_infile);
    if (status & 1) {
        len = strlen(the_cmd_line) + 2;
        if ((the_cmd_line = (char *) realloc (the_cmd_line, len)) == NULL)
            return(SS$_INSFMEM);
        strcat (the_cmd_line, " ");
        status = get_list (&cli_infile, &the_cmd_line, &foreign_cmdline, ' ');
        if (!(status & 1)) return (status);
    }

    /*
    **  Any files to exclude? ("-x file file")
    */
    status = cli$present (&cli_exclude);
    if (status & 1) {
        len = strlen(the_cmd_line) + work_str.dsc$w_length + 4 + 1;
        if ((the_cmd_line = (char *) realloc (the_cmd_line, len)) == NULL)
            return(SS$_INSFMEM);
        strcat (the_cmd_line, " -x ");
        status = get_list (&cli_exclude, &the_cmd_line, &foreign_cmdline, ' ');
        if (!(status & 1)) return (status);
    }

    /*
    **  Any files to include? ("-i file file")
    */
    status = cli$present (&cli_include);
    if (status & 1) {
        len = strlen(the_cmd_line) + work_str.dsc$w_length + 4 + 1;
        if ((the_cmd_line = (char *) realloc (the_cmd_line, len)) == NULL)
            return(SS$_INSFMEM);
        strcat (the_cmd_line, " -i ");
        status = get_list (&cli_include, &the_cmd_line, &foreign_cmdline, ' ');
        if (!(status & 1)) return (status);
    }


    /*
    **  Now that we've built our new UNIX-like command line, count the
    **  number of args and build an argv array.
    */

#if defined(TEST) || defined(DEBUG)
    printf("%s\n",the_cmd_line);
#endif /* TEST || DEBUG */

    new_argc = 1;
    for (ptr = the_cmd_line;
         (ptr = strchr(ptr,' ')) != NULL;
         ptr++, new_argc++);

    /*
    **  Allocate memory for the new argv[].  The last element of argv[]
    **  is supposed to be NULL, so allocate enough for new_argc+1.
    */
    if ((new_argv = (char **) calloc (new_argc+1, sizeof(char *))) == NULL)
        return(SS$_INSFMEM);

    /*
    **  For each option, store the address in new_argv[] and convert the
    **  separating blanks to nulls so each argv[] string is terminated.
    */
    for (ptr = the_cmd_line, x = 0; x < new_argc; x++) {
        new_argv[x] = ptr;
        if ((ptr = strchr (ptr, ' ')) != NULL)
            *ptr++ = '\0';
    }
    new_argv[new_argc] = NULL;

#if defined(TEST) || defined(DEBUG)
    printf("new_argc    = %d\n", new_argc);
    for (x = 0; x < new_argc; x++)
        printf("new_argv[%d] = %s\n", x, new_argv[x]);
#endif /* TEST || DEBUG */

    /*
    **  All finished.  Return the new argc and argv[] addresses to Zip.
    */
    *argc_p = new_argc;
    *argv_p = new_argv;

    return(SS$_NORMAL);
}



unsigned long
get_list (struct dsc$descriptor_s *qual, char **str,
          struct dsc$descriptor_d *cmdline, char c)
{
/*
**  Routine:    get_list
**
**  Function:   This routine runs through a comma-separated CLI list
**              and copies the strings to the command line.  The
**              specified separation character is used to separate
**              the strings on the command line.
**
**              All unquoted strings are converted to lower-case.
**
**  Formal parameters:
**
**      qual    - Address of descriptor for the qualifier name
**      str     - Address of pointer pointing to string (command line)
**      cmdline - Address of descriptor for the full command line tail
**      c       - Character to use to separate the list items
**
*/

    register status;
    struct dsc$descriptor_d work_str;

    init_dyndesc(work_str);

    status = cli$present (qual);
    if (status & 1) {

        unsigned long len, old_len, lower_it, ind, sind;

        len = strlen(*str);
        while ((status = cli$get_value (qual, &work_str)) & 1) {
            /*
            **  Just in case the string doesn't exist yet, though it does.
            */
            if (*str == NULL) {
                len = work_str.dsc$w_length + 1;
                if ((*str = (char *) malloc (work_str.dsc$w_length)) == NULL)
                    return(SS$_INSFMEM);
                strncpy(*str,work_str.dsc$a_pointer,len);
            } else {
                char *src, *dst; int x;
                old_len = len;
                len += work_str.dsc$w_length + 1;
                if ((*str = (char *) realloc (*str, len)) == NULL)
                    return(SS$_INSFMEM);

                /*
                **  Look for the filename in the original foreign command
                **  line to see if it was originally quoted.  If so, then
                **  don't convert it to lowercase.
                */
                lower_it = 0;
                str$find_first_substring (cmdline, &ind, &sind, &work_str);
                if (*(cmdline->dsc$a_pointer + ind - 2) == '"')
                    lower_it = 1;

                /*
                **  Copy the string to the buffer, converting to lowercase.
                */
                src = work_str.dsc$a_pointer;
                dst = *str+old_len;
                for (x = 0; x < work_str.dsc$w_length; x++) {
                    if (!lower_it && ((*src >= 'A') && (*src <= 'Z')))
                        *dst++ = *src++ + 32;
                    else
                        *dst++ = *src++;
                }
            }
            if (status == CLI$_COMMA)
                (*str)[len-1] = c;
            else
                (*str)[len-1] = '\0';
        }
    }

    return (SS$_NORMAL);

}


unsigned long
check_cli (struct dsc$descriptor_s *qual)
{
/*
**  Routine:    check_cli
**
**  Function:   Check to see if a CLD was used to invoke the program.
**
**  Formal parameters:
**
**      qual    - Address of descriptor for qualifier name to check.
**
*/
    lib$establish(lib$sig_to_ret);      /* Establish condition handler */
    return (cli$present(qual));         /* Just see if something was given */
}


void help()  /* VMSCLI version */
/* Print help (along with license info) to stdout. */
{
  extent i;             /* counter for help array */

  /* help array */
  static char *text[] = {
"Zip %s (%s). Usage: zip==\"$disk:[dir]zip.exe\"",
"zip zipfile[.zip] [list] [/EXCL=(xlist)] /options /modifiers",
"  The default action is to add or replace zipfile entries from list, except",
"  those in xlist. The include file list may contain the special name - to",
"  compress standard input.  If both zipfile and list are omitted, zip",
"  compresses stdin to stdout.",
"  Major options include:",
"    /FRESHEN, /UPDATE, /DELETE, /[NO]MOVE, /COMMENTS[={ZIP_FILE|FILES}],",
"    /LATEST, /TEST, /ADJUST_OFFSETS, /FIX_ARCHIVE[=FULL], /UNSFX",
"  Modifiers include:",
"    /EXCLUDE=(file list), /INCLUDE=(file list), /SINCE=\"creation time\",",
#ifdef CRYPT
"\
    /QUIET,/VERBOSE[=MORE],/[NO]RECURSE,/[NO]DIRNAMES,/JUNK,/ENCRYPT[=VERIFY],",
#else /* !CRYPT */
"    /QUIET, /VERBOSE[=MORE], /[NO]RECURSE, /[NO]DIRNAMES, /JUNK,",
#endif /* ?CRYPT */
"    /[NO]KEEP_VERSION, /[NO]VMS, /[NO]PKZIP, /TRANSLATE_EOL[={LF|CRLF}],",
"    /[NO]EXTRA_FIELDS /LEVEL=[0-9], /TEMP_PATH=directory, /BATCH[=list file]"
  };

  for (i = 0; i < sizeof(copyright)/sizeof(char *); i++)
  {
    printf(copyright[i], "zip");
    putchar('\n');
  }
  for (i = 0; i < sizeof(text)/sizeof(char *); i++)
  {
    printf(text[i], VERSION, REVDATE);
    putchar('\n');
  }
} /* end function help() */
