.AUTODEPEND


#
# Borland C++ tools
#
IMPLIB  = Implib
BCC     = Bcc32 +BccW32.cfg 
# BCC     = bcc
TLINK   = TLink32
#
# PATHS
#
LIB=\borlandc\lib
INC=\borlandc\include
#
# Options
#
LDFLAGS =  -Tpd -aa -x -c -L$(LIB)
CFLAGS = -I. -DWIZZIPDLL;WIN32

#
# Dependency List
#
Dep_Zipdll32 = \
   wizzip32.lib

Zipdll32 : BccW32.cfg $(Dep_Zipdll32)
  echo MakeNode

wizzip32.lib : wizzip32.dll
  $(IMPLIB) $@ wizzip32.dll


Dep_CcbWIZZIPbEXE32bwizzip32ddll = \
   windll\wizdll.def\
   wizmain.obj\
   win32zip.obj\
   win32.obj\
   ttyio.obj\
   zip.obj\
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

wizzip32.dll : $(Dep_CcbWIZZIPbEXE32bwizzip32ddll)
  $(TLINK)   @&&|
 /v $(LDFLAGS) +
$(LIB)\c0d32.obj+
wizmain.obj+
win32zip.obj+
win32.obj+
ttyio.obj+
zip.obj+
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
$(LIB)\import32.lib+
$(LIB)\cw32.lib
windll\wizdll.def
|

wizmain.obj :  windll\wizmain.c
        $(BCC)   -P- -c  $(CFLAGS) -o$@ windll\wizmain.c

win32zip.obj : win32\win32zip.c
        $(BCC)   -P- -c  $(CFLAGS) -o$@ win32\win32zip.c

win32.obj : win32\win32.c
        $(BCC)   -P- -c  $(CFLAGS) -o$@ win32\win32.c

ttyio.obj :  ttyio.c
        $(BCC)   -P- -c  $(CFLAGS) -o$@ ttyio.c

zip.obj :  zip.c
  $(BCC)   -P- -c  $(CFLAGS) -o$@ zip.c

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
BccW32.cfg : 
   Copy &&|
-I$(INC)
-w
-R
-v
-vi
-H
-H=zipdll32.csm
-WD
| $@


