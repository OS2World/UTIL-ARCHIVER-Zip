$ ! LINK_ZIP.COM
$ !
$ !	Command procedure to (re)link the VMS versions of
$ !	Zip, ZipCloak, ZipNote, and ZipSplit
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
$	opts = ""
$	say "Linking on AXP using DECC"
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
$	  ARCH_CC_P = "''ARCH_PREF'DECC_"
$	  opts = ""
$	  say "Linking on VAX using DECC"
$	ELSE
$!	  We use VAXC (or GNU C):
$	  USE_DECC_VAX = 0
$	  opts = ",[.VMS]VAXCSHR.OPT/OPTIONS"
$	  if (.not.HAVE_VAXC_VAX .and. MAY_HAVE_GNUC) .or. (MAY_USE_GNUC)
$	  then
$		ARCH_CC_P = "''ARCH_PREF'GNUC_"
$		opts = ",GNU_CC:[000000]GCCLIB.OLB/LIB ''opts'"
$		say "Linking on VAX using GNUC"
$	  else
$		ARCH_CC_P = "''ARCH_PREF'VAXC_"
$		say "Linking on VAX using VAXC"
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
$ link'LFLAGS'/exe=zip.'ARCH_CC_P'exe -
	zip.'ARCH_CC_P'olb;/incl=(zip,globals)/lib 'opts'
$ !
$ !-------------------------- Zip utilities section ---------------------------
$ !
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
$ dummy = f$verify (old_verify)
$ exit
