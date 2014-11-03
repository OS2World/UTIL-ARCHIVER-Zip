$ ! MAKE_ZIP.COM
$ !
$ !	"Makefile" for VMS versions of Zip, ZipCloak, ZipNote,
$ !	and ZipSplit (stolen from Unzip)
$ !
$ !	To define additional options, define the global symbol
$ !	LOCAL_ZIP prior to executing MAKE_ZIP.COM:
$ !
$ !		$ LOCAL_ZIP == "VMSCLI,VMS_PK_EXTRA,"
$ !		$ @MAKE_ZIP
$ !
$ !	The trailing "," may be omitted.  Valid VMS-specific options
$ !	include VMSCLI, VMS_PK_EXTRA and VMS_IM_EXTRA; see the INSTALL file
$ !	for other options.
$ !
$ !
$ on error then goto error
$ on control_y then goto error
$ OLD_VERIFY = f$verify (0)
$!
$ say := write sys$output
$!##################### Customizing section #############################
$!
$ MAY_USE_DECC = 1
$ MAY_USE_GNUC = 0
$!
$! Process command line parameters requesting optional features:
$ arg_cnt = 1
$ argloop:
$  current_arg_name = "P''arg_cnt'"
$  curr_arg = f$edit('current_arg_name',"UPCASE")
$  IF curr_arg .eqs. "" THEN GOTO argloop_out
$  IF curr_arg .eqs. "VAXC"
$  THEN MAY_USE_DECC = 0
$    MAY_USE_GNUC = 0
$  ENDIF
$  IF curr_arg .eqs. "DECC"
$  THEN MAY_USE_DECC = 1
$    MAY_USE_GNUC = 0
$  ENDIF
$  IF curr_arg .eqs. "GNUC"
$  THEN MAY_USE_DECC = 0
$    MAY_USE_GNUC = 1
$  ENDIF
$  arg_cnt = arg_cnt + 1
$ GOTO argloop
$ argloop_out:
$!
$!#######################################################################
$!
$ ! Find out current disk, directory, compiler and options
$ !
$ my_name = f$env("procedure")
$ workdir = f$env("default")
$ here = f$parse(workdir,,,"device") + f$parse(workdir,,,"directory")
$ if f$type(LOCAL_ZIP).eqs.""
$ then
$	local_zip = ""
$ else	! Trim blanks and append comma if missing
$	local_zip = f$edit(local_zip, "TRIM")
$	if f$extract(f$length(local_zip)-1, 1, local_zip).nes."," then -
		local_zip = local_zip + ","
$ endif
$ axp = f$getsyi("HW_MODEL").ge.1024
$ if axp
$ then
$	! Alpha AXP
$	ARCH_NAME == "Alpha"
$	ARCH_PREF = "AXP_"
$	HAVE_DECC_VAX = 0
$	USE_DECC_VAX = 0
$	if MAY_USE_GNUC
$	then say "GNU C has not yet been ported to OpenVMS AXP."
$	     say "You must use DEC C to build Zip."
$	     goto error
$	endif
$	ARCH_CC_P = ARCH_PREF
$	cc = "cc/standard=relax/prefix=all/ansi"
$	defs = "''local_zip'VMS"
$	opts = ""
$	say "Compiling on AXP using DECC"
$ else
$	! VAX
$	ARCH_NAME == "VAX"
$	ARCH_PREF = "VAX_"
$	HAVE_DECC_VAX = (F$TRNLNM("DECC$LIBRARY_INCLUDE") .NES. "")
$	HAVE_VAXC_VAX = (f$search("SYS$SYSTEM:VAXC.EXE").nes."")
$	MAY_HAVE_GNUC = (f$trnlnm("GNU_CC").nes."")
$	IF HAVE_DECC_VAX .AND. MAY_USE_DECC
$	THEN
$!	  We use DECC:
$	  USE_DECC_VAX = 1
$	  cc = "cc/decc/standard=vaxc/prefix=all"
$	  ARCH_CC_P = "''ARCH_PREF'DECC_"
$	  defs = "''local_zip'VMS"
$	  opts = ""
$	  say "Compiling on VAX using DECC"
$	ELSE
$!	  We use VAXC (or GNU C):
$	  USE_DECC_VAX = 0
$	  defs = "''local_zip'VMS"
$	  opts = ",[.VMS]VAXCSHR.OPT/OPTIONS"
$	  if (.not.HAVE_VAXC_VAX .and. MAY_HAVE_GNUC) .or. (MAY_USE_GNUC)
$	  then
$		ARCH_CC_P = "''ARCH_PREF'GNUC_"
$		cc = "gcc"
$		opts = ",GNU_CC:[000000]GCCLIB.OLB/LIB ''opts'"
$		say "Compiling on VAX using GNUC"
$	  else
$		ARCH_CC_P = "''ARCH_PREF'VAXC_"
$		if HAVE_DECC_VAX
$		then
$		    cc = "cc/vaxc"
$		else
$		    cc = "cc"
$		endif
$		say "Compiling on VAX using VAXC"
$	  endif
$	ENDIF
$ endif
$ LFLAGS = "/notrace"
$ if (opts .nes. "") .and. (f$search("[.vms]vaxcshr.opt") .eqs. "")
$ then	create [.vms]vaxcshr.opt
$	open/append tmp [.vms]vaxcshr.opt
$	write tmp "SYS$SHARE:VAXCRTL.EXE/SHARE"
$	close tmp
$ endif
$ set verify    ! like "echo on", eh?
$ !
$ !------------------------------- Zip section --------------------------------
$ !
$ cc/def=('DEFS') /obj=.'ARCH_CC_P'obj zip.c, crc32.c, crctab.c, crypt.c, -
	ttyio.c, zipfile.c, zipup.c, fileio.c, globals.c, util.c
$ cc/def=('DEFS') /obj=.'ARCH_CC_P'obj deflate.c, trees.c, bits.c
$ cc/def=('DEFS') /obj=.'ARCH_CC_P'obj/inc=SYS$DISK:[] -
	[.vms]vms.c, [.vms]vmszip.c, [.vms]VMSmunch.c
$ !
$ if f$locate("VMSCLI",local_zip).ne.f$length(local_zip)
$ then
$	cc/INCLUDE=SYS$DISK:[]'DEF' /OBJ=cmdline.'ARCH_CC_P'obj; -
		[.vms]cmdline.c
$	set command/obj=zip_cli.'ARCH_CC_P'obj [.vms]zip_cli.cld
$	cliobjs = ",cmdline.'ARCH_CC_P'obj, zip_cli.'ARCH_CC_P'obj"
$	set default [.vms]
$	edit/tpu/nosection/nodisplay/command=cvthelp.tpu zip_cli.help
$	set default [-]
$	runoff/out=zip.hlp [.vms]zip_cli.rnh
$ else
$	cliobjs = ""
$	runoff/out=zip.hlp [.vms]vms_zip.rnh
$ endif
$ !
$ if f$search("zip.''ARCH_CC_P'olb") .EQS. "" then -
	lib/obj/create zip.'ARCH_CC_P'olb
$ lib/obj/replace zip.'ARCH_CC_P'olb -
	zip.'ARCH_CC_P'obj;, crc32.'ARCH_CC_P'obj;, crctab.'ARCH_CC_P'obj;, -
	crypt.'ARCH_CC_P'obj;, ttyio.'ARCH_CC_P'obj;, -
	zipfile.'ARCH_CC_P'obj;, zipup.'ARCH_CC_P'obj;, -
	fileio.'ARCH_CC_P'obj;, util.'ARCH_CC_P'obj;, globals.'ARCH_CC_P'obj;,-
	deflate.'ARCH_CC_P'obj;, trees.'ARCH_CC_P'obj;, bits.'ARCH_CC_P'obj;, -
	vms.'ARCH_CC_P'obj;, vmszip.'ARCH_CC_P'obj;, -
	VMSmunch.'ARCH_CC_P'obj; 'cliobjs'
$ link'LFLAGS'/exe=zip.'ARCH_CC_P'exe -
	zip.'ARCH_CC_P'olb;/incl=(zip,globals)/lib 'opts'
$ !
$ !-------------------------- Zip utilities section ---------------------------
$ !
$ cc/def=('DEFS',UTIL) /obj=zipfile_.'ARCH_CC_P'obj zipfile.c
$ cc/def=('DEFS',UTIL) /obj=fileio_.'ARCH_CC_P'obj fileio.c
$ cc/def=('DEFS',UTIL) /obj=util_.'ARCH_CC_P'obj util.c
$ cc/def=('DEFS',UTIL) /obj=crypt_.'ARCH_CC_P'obj crypt.c
$ cc/def=('DEFS',UTIL)/incl=SYS$DISK:[] /obj=vms_.'ARCH_CC_P'obj [.vms]vms.c
$ if f$search("ziputils.''ARCH_CC_P'olb") .EQS. "" then -
	lib/obj/create ziputils.'ARCH_CC_P'olb
$ lib/obj/replace ziputils.'ARCH_CC_P'olb -
	zipfile_.'ARCH_CC_P'obj;, fileio_.'ARCH_CC_P'obj;, -
	util_.'ARCH_CC_P'obj;, globals.'ARCH_CC_P'obj;, -
	crctab.'ARCH_CC_P'obj;, crypt_.'ARCH_CC_P'obj;, ttyio.'ARCH_CC_P'obj;,-
	vms_.'ARCH_CC_P'obj;, VMSmunch.'ARCH_CC_P'obj;
$ cc /def=('DEFS') /obj=.'ARCH_CC_P'obj zipcloak, zipnote, zipsplit
$ link'LFLAGS'/exe=zipcloak.'ARCH_CC_P'exe zipcloak.'ARCH_CC_P'obj, -
	ziputils.'ARCH_CC_P'olb;/incl=(globals)/lib 'opts'
$ link'LFLAGS'/exe=zipnote.'ARCH_CC_P'exe zipnote.'ARCH_CC_P'obj, -
	ziputils.'ARCH_CC_P'olb;/incl=(globals)/lib 'opts'
$ link'LFLAGS'/exe=zipsplit.'ARCH_CC_P'exe zipsplit.'ARCH_CC_P'obj, -
	ziputils.'ARCH_CC_P'olb;/incl=(globals)/lib 'opts'
$ !
$ !----------------------------- Symbols section ------------------------------
$ !
$ ! Set up symbols for the various executables.  Edit the example below,
$ ! changing "disk:[directory]" as appropriate.
$ !
$ zip           == "$''here'zip.''ARCH_CC_P'exe"
$ zipcloak      == "$''here'zipcloak.''ARCH_CC_P'exe"
$ zipnote       == "$''here'zipnote.''ARCH_CC_P'exe"
$ zipsplit      == "$''here'zipsplit.''ARCH_CC_P'exe"
$ !
$error:
$ if here .nes. "" then set default 'here'
$ dummy = f$verify (old_verify)
$ exit
