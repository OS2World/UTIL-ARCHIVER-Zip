# VMS Makefile for Zip, ZipNote, ZipCloak and ZipSplit

#
#  Modified to support both AXP and VAX by Hunter Goatley, 10-SEP-1993 06:43
#  Modified (DECC VAX, Zip 2.1) by Christian Spieler, 16-SEP-1995
#  Modified (Addition of VMS CLI) by Christian Spieler, 10-OCT-1995
#  Modified (fixed VAXC, changed compiler opts) by C. Spieler, 10-DEC-1995
#  Modified (removed zipup_.obj from Zip utils) by C. Spieler, 08-JAN-1996
#  Modified (cmdline$O depends on crypt.h) by C. Spieler, 09-JAN-1996
#  Last modified (split crypt -> crypt, ttyio) by C. Spieler, 16-JAN-1996
#
#  To build Zip and the Ziputils, use one of the following commands,
#  depending on your system:
#
#	$ MMS/MACRO=(__ALPHA__=1)		! Alpha AXP, (DEC C)
#	$ MMS/MACRO=(__DECC__=1)		! VAX, using DEC C
#	$ MMS/MACRO=(__FORCE_VAXC__=1)		! VAX, prefering VAXC over DECC
#	$ MMS/MACRO=(__VAXC__=1)		! VAX, where VAXC is default
#	$ MMS/MACRO=(__GNUC__=1)		! VAX, using GNU C
#
#  For some discussion on the compiler switches used, see documentation
#  in 00readme.vms.
#
.IFDEF __ALPHA__
E = .AXP_EXE
O = .AXP_OBJ
A = .AXP_OLB
.ELSE
.IFDEF __DECC__
E = .VAX_DECC_EXE
O = .VAX_DECC_OBJ
A = .VAX_DECC_OLB
.ENDIF
.IFDEF __FORCE_VAXC__
__VAXC__ = 1
.ENDIF
.IFDEF __VAXC__
E = .VAX_VAXC_EXE
O = .VAX_VAXC_OBJ
A = .VAX_VAXC_OLB
.ENDIF
.IFDEF __GNUC__
E = .VAX_GNUC_EXE
O = .VAX_GNUC_OBJ
A = .VAX_GNUC_OLB
.ENDIF
.ENDIF
.IFDEF O
.ELSE
!If EXE and OBJ extentions aren't defined, define them
E = .EXE
O = .OBJ
A = .OLB
.ENDIF

!!!!!!!!!!!!!!!!!!!!!!!!!!! USER CUSTOMIZATION !!!!!!!!!!!!!!!!!!!!!!!!!!!!
! uncomment the following line if you want the VMS CLI$ interface:
!VMSCLI = VMSCLI,

! add any other optional preprocessor flags (macros) to the following line
! for a custom version (do not forget a trailing comma!!):
COMMON_DEFS =
!!!!!!!!!!!!!!!!!!!!!!!! END OF USER CUSTOMIZATION !!!!!!!!!!!!!!!!!!!!!!!!

.IFDEF __GNUC__
CC = gcc
LIBS = ,GNU_CC:[000000]GCCLIB.OLB/LIB
.ELSE
CC = cc
LIBS =
.ENDIF

CFLAGS = /NOLIST/INCL=(SYS$DISK:[])

OPTFILE = [.vms]vaxcshr.opt

.IFDEF __ALPHA__		!Under OpenVMS AXP, we must use /PREFIX=ALL
CFLG_ARCH = /STANDARD=RELAX/PREFIX=ALL/ANSI_ALIAS
OPTFILE_LIST =
OPTIONS = $(LIBS)
.ELSE
.IFDEF __DECC__			!Under DECC VAX, we must use /PREFIX=ALL
CFLG_ARCH = /DECC/STANDARD=VAXC/PREFIX=ALL
OPTFILE_LIST =
OPTIONS = $(LIBS)
.ELSE				!VAXC, or GNU C on VAX
.IFDEF __FORCE_VAXC__		!Select VAXC on systems where DEC C exists
CFLG_ARCH = /VAXC
.ELSE				!No flag allowed/needed on a pure VAXC system
CFLG_ARCH =
.ENDIF
OPTFILE_LIST = ,$(OPTFILE)
OPTIONS = $(LIBS),$(OPTFILE)/OPT
.ENDIF
.ENDIF

.IFDEF __DEBUG__
CDEB = /DEBUG/NOOPTIMIZE
LDEB = /DEBUG
.ELSE
CDEB =
LDEB = /NOTRACE
.ENDIF

CFLAGS_ALL  = $(CFLG_ARCH) $(CFLAGS) $(CDEB) -
	/def=($(COMMON_DEFS) $(VMSCLI) VMS)
CFLAGS_UTIL = $(CFLG_ARCH) $(CFLAGS) $(CDEB) -
	/def=($(COMMON_DEFS) $(VMSCLI) UTIL, VMS)

LINKFLAGS = $(LDEB)


OBJZ =	zip$(O), crc32$(O), crctab$(O), crypt$(O), ttyio$(O),-
	zipfile$(O), zipup$(O), fileio$(O), globals$(O), util$(O)
OBJV =  vmszip$(O), vms$(O), vmsmunch$(O)
OBJI =	deflate$(O),trees$(O),bits$(O)
OBJU =	ZIPFILE=zipfile_$(O),FILEIO=fileio_$(O),globals$(O),-
	UTIL=util_$(O),VMSZIP=vmszip_$(O),VMS=vms_$(O),vmsmunch$(O)
OBJR =	crctab$(O), CRYPT=crypt_$(O), ttyio$(O)
OBJC =	zipcloak$(O)
OBJN =	zipnote$(O)
OBJS =	zipsplit$(O)

.IFDEF VMSCLI
CLI_OBJS = ZIP_CLITABLE=zip_cli$(O),-
	VMS_ZIP_CMDLINE=cmdline$(O)
OBJSZIP = $(OBJZ),$(OBJV),$(OBJI),$(CLI_OBJS)
ZIPHELP_RNH = [.vms]zip_cli.rnh
.ELSE
CLI_OBJS =
OBJSZIP = $(OBJZ),$(OBJV),$(OBJI)
ZIPHELP_RNH = [.vms]vms_zip.rnh
.ENDIF

OLBZIP = zip$(A)
OLBUTI = ziputils$(A)

ZIP_H =	zip.h,ziperr.h,tailor.h,[.vms]osdep.h

ZIPS =	zip$(E),zipcloak$(E),zipnote$(E),zipsplit$(E)

#
#  Define our new suffixes list
#
.SUFFIXES :
.SUFFIXES :	$(E) $(A) $(O) .C .MAR .CLD .HLP .RNH

$(O)$(E) :
	$(LINK) $(LINKFLAGS) /EXE=$(MMS$TARGET) $(MMS$SOURCE)

$(O)$(A) :
	If "''F$Search("$(MMS$TARGET)")'" .EQS. "" Then $(LIBR)/Create $(MMS$TARGET)
	$(LIBR)$(LIBRFLAGS) $(MMS$TARGET) $(MMS$SOURCE)

.CLD$(O) :
	SET COMMAND /OBJECT=$(MMS$TARGET) $(CLDFLAGS) $(MMS$SOURCE)

.c$(O) :
	$(CC) $(CFLAGS_ALL) /OBJ=$(MMS$TARGET) $(MMS$SOURCE)


# rules for zip, zipnote, zipsplit, and zip.doc.

default :	$(ZIPS),zip.hlp
	@ !

vmszip$(O)   : [.vms]vmszip.c
vmsmunch$(O) : [.vms]vmsmunch.c
vms$(O)      : [.vms]vms.c [.vms]vms_im.c [.vms]vms_pk.c [.vms]vms.h
cmdline$(O)  : [.vms]cmdline.c $(ZIP_H) crypt.h revision.h
zip_cli$(O)  : [.vms]zip_cli.cld

zipfile_$(O) :	zipfile.c,[.vms]vmsmunch.h,[.vms]vmsdefs.h
	$(CC) $(CFLAGS_UTIL) /OBJECT=$(MMS$TARGET) $<
fileio_$(O)  :	fileio.c
	$(CC) $(CFLAGS_UTIL) /OBJECT=$(MMS$TARGET) $<
util_$(O)    :	util.c
	$(CC) $(CFLAGS_UTIL) /OBJECT=$(MMS$TARGET) $<
crypt_$(O)   :	crypt.c,crypt.h,ttyio.h
	$(CC) $(CFLAGS_UTIL) /OBJECT=$(MMS$TARGET) $<
vms_$(O)     :	[.vms]vms.c,[.vms]vms_im.c,[.vms]vms_pk.c, -
		[.vms]vms.h,[.vms]vmsdefs.h
	$(CC) $(CFLAGS_UTIL) /OBJECT=$(MMS$TARGET) $<
vmszip_$(O)  :	[.vms]vmszip.c,[.vms]vmsmunch.h
	$(CC) $(CFLAGS_UTIL) /OBJECT=$(MMS$TARGET) $<

zip$(O),zipcloak$(O),zipnote$(O),zipsplit$(O),zipup$(O) : revision.h

zip$(O),zipcloak$(O),zipup$(O),bits$(O),crypt$(O),ttyio$(O) : crypt.h

zip$(O),zipcloak$(O),crypt$(O),ttyio$(O) : ttyio.h

zipup$(O) : [.vms]zipup.h

zip$(O), zipfile$(O), vmszip$(O), vmsmunch$(O) : [.vms]vmsmunch.h

zipfile$(O), vms$(O), vmsmunch$(O) : [.vms]vmsdefs.h

$(OBJZ) : $(ZIP_H)
$(OBJV) : $(ZIP_H)
$(OBJI) : $(ZIP_H)
$(OBJU) : $(ZIP_H)
$(OBJR) : $(ZIP_H)
$(OBJC) : $(ZIP_H)
$(OBJN) : $(ZIP_H)
$(OBJS) : $(ZIP_H)


zip$(E) : $(OLBZIP)($(OBJSZIP))$(OPTFILE_LIST)
	$(LINK)$(LINKFLAGS) /EXE=$@ $(OLBZIP)/inc=(zip,globals)/lib$(OPTIONS)

zipcloak$(E) : $(OBJC),$(OLBUTI)($(OBJR),$(OBJU))$(OPTFILE_LIST)
	$(LINK)$(LINKFLAGS) /EXE=$@ $<, -
	$(OLBUTI)/inc=(globals)/lib$(OPTIONS)

zipnote$(E) : $(OBJN),$(OLBUTI)($(OBJU))$(OPTFILE_LIST)
	$(LINK)$(LINKFLAGS) /EXE=$@ $<, -
	$(OLBUTI)/inc=(globals)/lib$(OPTIONS)

zipsplit$(E) : $(OBJS),$(OLBUTI)($(OBJU))$(OPTFILE_LIST)
	$(LINK)$(LINKFLAGS) /EXE=$@ $<, -
	$(OLBUTI)/inc=(globals)/lib$(OPTIONS)

$(OPTFILE) :
	@ open/write tmp $(OPTFILE)
	@ write tmp "SYS$SHARE:VAXCRTL.EXE/SHARE"
	@ close tmp

[.vms]zip_cli.rnh	: [.vms]zip_cli.help
	@ set default [.vms]
	edit/tpu/nosection/nodisplay/command=cvthelp.tpu zip_cli.help
	@ set default [-]

zip.hlp : $(ZIPHELP_RNH)
	runoff /out=$@ $<

clean.com :
	@ open/write tmp $(MMS$TARGET)
	@ write tmp "$!"
	@ write tmp "$!	Clean.com --	procedure to delete files. It always returns success"
	@ write tmp "$!			status despite any error or warnings. Also it extracts"
	@ write tmp "$!			filename from MMS ""module=file"" format."
	@ write tmp "$!"
	@ write tmp "$ on control_y then goto ctly"
	@ write tmp "$ if p1.eqs."""" then exit 1"
	@ write tmp "$ i = -1"
	@ write tmp "$scan_list:"
	@ write tmp "$	i = i+1"
	@ write tmp "$	item = f$elem(i,"","",p1)"
	@ write tmp "$	if item.eqs."""" then goto scan_list"
	@ write tmp "$	if item.eqs."","" then goto done		! End of list"
	@ write tmp "$	item = f$edit(item,""trim"")		! Clean of blanks"
	@ write tmp "$	wild = f$elem(1,""="",item)"
	@ write tmp "$	show sym wild"
	@ write tmp "$	if wild.eqs.""="" then wild = f$elem(0,""="",item)"
	@ write tmp "$	vers = f$parse(wild,,,""version"",""syntax_only"")"
	@ write tmp "$	if vers.eqs."";"" then wild = wild - "";"" + "";*"""
	@ write tmp "$scan:"
	@ write tmp "$		f = f$search(wild)"
	@ write tmp "$		if f.eqs."""" then goto scan_list"
	@ write tmp "$		on error then goto err"
	@ write tmp "$		on warning then goto warn"
	@ write tmp "$		delete/log 'f'"
	@ write tmp "$warn:"
	@ write tmp "$err:"
	@ write tmp "$		goto scan"
	@ write tmp "$done:"
	@ write tmp "$ctly:"
	@ write tmp "$	exit 1"
	@ close tmp

clean : clean.com
	@clean "$(OBJZ)"
	@clean "$(OBJV)"
	@clean "$(OBJI)"
	@clean "$(OBJU)"
	@clean "$(OBJR)"
	@clean "$(OBJN)"
	@clean "$(OBJS)"
	@clean "$(OBJC)"
	@clean "$(CLI_OBJS)"
	@clean "$(OLBZIP)"
	@clean "$(OLBUTI)"
	@clean $(OPTFILE)
	@clean $(ZIPS)
	@clean zip.hlp
	- delete/noconfirm/nolog clean.com;*
