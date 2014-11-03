/*
 Copyright (C) 1996 Mike White
 Permission is granted to any individual or institution to use, copy, or
 redistribute this software so long as all of the original files are included,
 that it is not sold for profit, and that this copyright notice is retained.

*/

/*
 *  wizmain.c by Mike White based on Mark Adler's zip.c
 */
#include <windows.h>
#include "zip.h"
#include "crypt.h"
#include <process.h>
#include <signal.h>
#include <stdarg.h>
#include "wizdll/wizzip.h"

/*
This structure is passed to the DLL when it is called.

typedef struct {
DLLPRNT print;
DLLSND sound;
FILE *Stdout;
HWND hInst;
* Zip flag section *
BOOL fEncryptVerify;
BOOL fSuffix;
BOOL fEncrypt;
BOOL fSystem;           = include system and hidden files
BOOL fVolume;           = Include volume label
BOOL fExtra;            = Include extra attributes
BOOL fNoDirEntries;     = Do not add directory entries
BOOL fDate;             = Exclude files earlier than specified date
BOOL fVerbose;          = Mention oddities in zip file structure
BOOL fQuiet;            = Quiet operation
char fLevel;            = Compression level (0 - 9)
BOOL fCRLF_LF;
BOOL fLF_CRLF;          = Translate end-of-line
BOOL fJunkDir;          = Junk directory names
BOOL fRecurse;          = Recurse into subdirectories
BOOL fGrow;             = Allow appending to a zip file
BOOL fForce;            = Make entries using DOS names (k for Katz)
BOOL fMove;             = Delete files added or updated in zip file
BOOL fDeleteEntries;    = Delete files from zip file
BOOL fUpdate;           = Update zip file--overwrite only if newer
BOOL fFreshen;          = Freshen zip file--overwrite only
BOOL fJunkSFX;          = Junk SFX prefix
BOOL fLatestTime;       = Set zip file time to time of latest file in it 
* End Zip Flag section *
char Date[7];           = Date to include files after
int  argc;              = Count of files to zip
LPSTR lpszZipFN;
char **FNV;
} ZCL, _far *LPZCL;
*/

DLLPRNT lpPrint;
DLLSND lpSound;

FILE *orig_stdout;

/*  DLL Entry Point */
#ifdef __BORLANDC__
#pragma warn -par
#endif
#if defined WIN32
BOOL WINAPI DllEntryPoint( HINSTANCE hinstDll,
									DWORD fdwRreason,
									LPVOID plvReserved)
#else
int FAR PASCAL LibMain( HINSTANCE hInstance,
								WORD wDataSegment,
								WORD wHeapSize,
								LPSTR lpszCmdLine )
#endif
{
#ifndef WIN32
/* The startup code for the DLL initializes the local heap(if there is one)
 with a call to LocalInit which locks the data segment. */

if ( wHeapSize != 0 )
   {
   UnlockData( 0 );
   }
#endif
return 1;   /* Indicate that the DLL was initialized successfully. */
}

int FAR PASCAL WEP ( int bSystemExit )
{
return 1;
}
#ifdef __BORLANDC__
#pragma warn .par
#endif


/* Local functions */

extern int  zipmain OF((int, char **));

WINAPI DllZipUpFiles(ZCL far *C)
/* Add, update, freshen, or delete zip entries in a zip file.  See the
   command help in help() zip.c */
{
int argCee, i=0, k;
char **argVee;

lpPrint = C->print;
lpSound = C->sound;
orig_stdout = C->Stdout;

argCee = C->argc+3; /* Allow for argv[0], compression level, and the zip file name */
/* malloc'd additional 26 to allow for additional command line arguements */
argVee = (char **)malloc((C->argc+26)*sizeof(char *));

argVee[i] = (char *) malloc( sizeof(char) * strlen("wizzip.exe")+1 );
strcpy( argVee[i++], "wizzip.exe" );

/* Set compression level efficacy -0...-9 */
argVee[i] = (char *) malloc( sizeof(char) * strlen("-0")+1 );
strcpy(argVee[i], "-0");
argVee[i++][1] = C->fLevel;

if (C->fDeleteEntries)    /* Delete files from zip file -d */
   {
   argVee[i] = (char *) malloc( sizeof(char) * strlen("-d")+1 );
   strcpy( argVee[i++], "-d" );
   argCee++;
   }
if (C->fNoDirEntries) /* Do not add directory entries -D */
   {
   argVee[i] = (char *) malloc( sizeof(char) * strlen("-D")+1 );
   strcpy( argVee[i++], "-D" );
   argCee++;
   }
if (C->fFreshen) /* Freshen zip file--overwrite only -f */
   {
   argVee[i] = (char *) malloc( sizeof(char) * strlen("-f")+1 );
   strcpy( argVee[i++], "-f" );
   argCee++;
   }
if (C->fGrow) /* Allow appending to a zip file -g */
   {
   argVee[i] = (char *) malloc( sizeof(char) * strlen("-g")+1 );
   strcpy( argVee[i++], "-g" );
   argCee++;
   }
if (C->fJunkDir) /* Junk directory names -j */
   {
   argVee[i] = (char *) malloc( sizeof(char) * strlen("-j")+1 );
   strcpy( argVee[i++], "-j" );
   argCee++;
   }
if (C->fJunkSFX) /* Junk sfx prefix */
   {
   argVee[i] = (char *) malloc( sizeof(char) * strlen("-J")+1 );
   strcpy( argVee[i++], "-J" );
   argCee++;
   }

if (C->fForce) /* Make entries using DOS names (k for Katz) -k */
   {
   argVee[i] = (char *) malloc( sizeof(char) * strlen("-k")+1 );
   strcpy( argVee[i++], "-k" );
   argCee++;
   }

if (C->fLF_CRLF) /* Translate end-of-line -l */
   {
   argVee[i] = (char *) malloc( sizeof(char) * strlen("-l")+1 );
   strcpy( argVee[i++], "-l" );
   argCee++;
   }
if (C->fMove) /* Delete files added or updated in zip file -m */
   {
   argVee[i] = (char *) malloc( sizeof(char) * strlen("-m")+1 );
   strcpy( argVee[i++], "-m" );
   argCee++;
   }

if (C->fLatestTime) /* Set zip file time to time of latest file in it -o */
   {
   argVee[i] = (char *) malloc( sizeof(char) * strlen("-o")+1 );
   strcpy( argVee[i++], "-o" );
   argCee++;
   }

if (C->fQuiet) /* quiet operation -q */
   {
   argVee[i] = (char *) malloc( sizeof(char) * strlen("-q")+1 );
   strcpy( argVee[i++], "-q" );
   argCee++;
   }
if (C->fRecurse) /* recurse into subdirectories -r */
   {
   argVee[i] = (char *) malloc( sizeof(char) * strlen("-r")+1 );
   strcpy( argVee[i++], "-r" );
   argCee++;
   }
if (C->fSystem)  /* include system and hidden files -S */
   {
   argVee[i] = (char *) malloc( sizeof(char) * strlen("-S")+1 );
   strcpy( argVee[i++], "-S" );
   argCee++;
   }
if (C->fDate)    /* Exclude files earlier than specified date -t */
   {
   argVee[i] = (char *) malloc( sizeof(char) * strlen("-t")+1 );
   strcpy( argVee[i++], "-t" );
   argCee++;
   argVee[i] = (char *) malloc( sizeof(char) * strlen(C->Date)+1);
   strcpy( argVee[i++], C->Date);
   argCee++;
   }

if (C->fUpdate) /* Update zip file--overwrite only if newer -u */
   {
   argVee[i] = (char *) malloc( sizeof(char) * strlen("-u")+1 );
   strcpy( argVee[i++], "-u" );
   argCee++;
   }
if (C->fVerbose)  /* Mention oddities in zip file structure -v */
   {
   argVee[i] = (char *) malloc( sizeof(char) * strlen("-v")+1 );
   strcpy( argVee[i++], "-v" );
   argCee++;
   }
//if (C->
//   {
//            case 'z':   /* Edit zip file comment */
//   }
if (C->fVolume)  /* Include volume label -$ */
   {
   argVee[i] = (char *) malloc( sizeof(char) * strlen("-$")+1 );
   strcpy( argVee[i++], "-$" );
   argCee++;
   }
if (C->fExtra)  /* Include extra attributes -X */
   {
   argVee[i] = (char *) malloc( sizeof(char) * strlen("-X")+1 );
   strcpy( argVee[i++], "-X" );
   argCee++;
   }

argVee[i] = (char *) malloc( sizeof(char) * strlen(C->lpszZipFN)+1 );
strcpy( argVee[i++], C->lpszZipFN );

for (k = 0; k < C->argc; k++)
   {
   argVee[i] = (char *) malloc( sizeof(char) * strlen(C->FNV[k])+1 );
   strcpy( argVee[i++], C->FNV[k] );
   }

argVee[i] = NULL;

zipmain(argCee, argVee);

/* Free the arguments in the array */
for (i = 0; i < argCee; i++)
   {
      free (argVee[i]);
      argVee[i] = NULL;
   }

/* Then free the array itself */
free(argVee);


return;
}

#define STDIO_BUF_SIZE 16384

/* printf buffers the current output and counts the number of lines
 * within it.  It makes sure there is enough space in the global
 * buffer, then copies the buffered data to the global buffer.
 * It then triggers a repaint of the status buffer.
 */
int __far __cdecl printf(const char *format, ...)
{
va_list argptr;
HANDLE hMemory;
PSTR pszBuffer;
int len;

va_start(argptr, format);
hMemory = GlobalAlloc(GMEM_MOVEABLE, STDIO_BUF_SIZE);
WinAssert(hMemory);
if (!hMemory)
   {
   return 0;
   }
pszBuffer = (PSTR)GlobalLock(hMemory);
WinAssert(pszBuffer);
len = wvsprintf(pszBuffer, format, argptr);
va_end(argptr);
WinAssert(strlen(pszBuffer) < STDIO_BUF_SIZE);
len = lpPrint(orig_stdout, len, pszBuffer);
GlobalUnlock(hMemory);
GlobalFree(hMemory);
return len;
}

#ifdef __BORLANDC__
#pragma argsused
#endif
/* fprintf clone for code in zip.c, etc. */
int __far __cdecl fprintf(FILE *file, const char *format, ...)
{
va_list argptr;
HANDLE hMemory;
LPSTR pszBuffer;
int len;

va_start(argptr, format);
hMemory = GlobalAlloc(GMEM_MOVEABLE, STDIO_BUF_SIZE);
WinAssert(hMemory);
if (!hMemory)
   {
   return 0;
   }
pszBuffer = GlobalLock(hMemory);
WinAssert(pszBuffer);
len = wvsprintf(pszBuffer, format, argptr);
va_end(argptr);
WinAssert(strlen(pszBuffer) < STDIO_BUF_SIZE);
if ((file != stderr) && (file != stdout))
   {
   len = write(fileno(file),(char *)(pszBuffer), len);
   }
else
   {
   len = lpPrint(orig_stdout, len, pszBuffer);
   }
GlobalUnlock(hMemory);
GlobalFree(hMemory);
return len;
}

void __far __cdecl perror(const char *parm1)
{
printf(parm1);
}

