.AUTODEPEND


#
# Borland C++ tools
#
IMPLIB  = Implib
BCC     = Bcc +BccW16.cfg 
# BCC     = bcc
TLINK   = TLink
#
# PATHS
#
LIB=\borlandc\lib
INC=\borlandc\include
#
# Options
#
LDFLAGS =  -Twd -x -c -L$(LIB)
CFLAGS = -I. -DWIZZIPDLL;DOS

#
# Dependency List
#
Dep_Zipdll16 = \
   wizzip16.lib

Zipdll16 : BccW16.cfg $(Dep_Zipdll16)
  echo MakeNode

wizzip16.lib : wizzip16.dll
  $(IMPLIB) $@ wizzip16.dll


Dep_CcbWIZZIPbEXE16bwizzip16ddll = \
   wizmain.obj\
   windll\wizdll.def\
   ttyio.obj\
   zip.obj\
   msdos.obj\
   bits.obj\
   crc32.obj\
   crctab.obj\
   crypt.obj\
   deflate.obj\
   fileio.obj\
   globals.obj\
   trees.obj\
   util.obj\
   zipfile.obj\
   zipup.obj

wizzip16.dll : $(Dep_CcbWIZZIPbEXE16bwizzip16ddll)
  $(TLINK)   @&&|
 /v $(LDFLAGS) +
$(LIB)\c0dl.obj+
wizmain.obj+
ttyio.obj+
zip.obj+
msdos.obj+
bits.obj+
crc32.obj+
crctab.obj+
crypt.obj+
deflate.obj+
fileio.obj+
globals.obj+
trees.obj+
util.obj+
zipfile.obj+
zipup.obj
$<,$*
$(LIB)\import.lib+
$(LIB)\mathwl.lib+
$(LIB)\cwl.lib
windll\wizdll.def
|

wizmain.obj :  windll\wizmain.c
        $(BCC)   -P- -c  $(CFLAGS) -o$@ windll\wizmain.c

ttyio.obj :  ttyio.c
        $(BCC)   -P- -c  $(CFLAGS) -o$@ ttyio.c

zip.obj :  zip.c
  $(BCC)   -P- -c  $(CFLAGS) -o$@ zip.c

msdos.obj :  msdos\msdos.c
  $(BCC)   -P- -c  $(CFLAGS) -o$@ msdos\msdos.c

bits.obj :  bits.c
  $(BCC)   -P- -c $(CFLAGS) -o$@ bits.c

crc32.obj :  crc32.c
  $(BCC)   -P- -c $(CFLAGS) -o$@ crc32.c

crctab.obj :  crctab.c
  $(BCC)   -P- -c $(CFLAGS) -o$@ crctab.c

crypt.obj :  crypt.c
  $(BCC)   -P- -c $(CFLAGS) -o$@ crypt.c

deflate.obj :  deflate.c
  $(BCC)   -P- -c $(CFLAGS) -o$@ deflate.c

fileio.obj :  fileio.c
  $(BCC)   -P- -c $(CFLAGS) -o$@ fileio.c

globals.obj :  globals.c
  $(BCC)   -P- -c $(CFLAGS) -o$@ globals.c

trees.obj :  trees.c
  $(BCC)   -P- -c $(CFLAGS) -o$@ trees.c

util.obj :  util.c
  $(BCC)   -P- -c $(CFLAGS) -o$@ util.c

zipfile.obj :  zipfile.c
  $(BCC)   -P- -c $(CFLAGS) -o$@ zipfile.c

zipup.obj :  zipup.c
  $(BCC)   -P- -c $(CFLAGS) -o$@ zipup.c

# Compiler configuration file
BccW16.cfg : 
   Copy &&|
-I$(INC)
-w
-R
-v
-vi
-H
-H=zipdll16.csm
-ml
-WD
-H
-Ff
-d
-N
-dc
-Vf
| $@


