
PROJ = test_lzmat

OBJS = $(PROJ).obj \
	lzmat_dec.obj \
	lzmat_enc.obj

!if "$(DEBUG)" == ""
DEBUG = 1
!endif
!if "$(UNI)" == ""
UNI = 1
!endif

CC = cl


RSC=rc.exe
RSC_PROJ=/l 0x419 /fo$(PROJ).res /d "NDEBUG" 

CFLAGS_D_DEXE32 = /D_X86_ /YX /W3 /D_DEBUG  /DLZMAT_COMPRESS_ALWAYS /D_WINDOWS /DWIN32 /Zi /Fp"$(PROJ).PCH" /YX
CFLAGS_R_DEXE32 = /c /nologo /D_X86_ /W3 /O2 /GX /D"NDEBUG"  /D "_WINDOWS" /D "WIN32" /YX 

LFLAGS_D_DEXE32 = /DEBUG /DEBUGTYPE:CV /SUBSYSTEM:console /incremental:no
LFLAGS_R_DEXE32 = /SUBSYSTEM:console /incremental:no
LIBS_D_DEXE32 = kernel32.lib
LIBS_R_DEXE32 = kernel32.lib

!if "$(UNI)" == "1"
UNI_FLAG = /D_UNICODE /DUNICODE
!else
UNI_FLAG = 
!endif

!if "$(DEBUG)" == "1"
CFLAGS = $(CFLAGS_D_DEXE32) $(UNI_FLAG)
LFLAGS = $(LFLAGS_D_DEXE32) 
LIBS = $(LIBS_D_DEXE32)
!else
CFLAGS = $(CFLAGS_R_DEXE32) $(UNI_FLAG)
LFLAGS = $(LFLAGS_R_DEXE32)
LIBS = $(LIBS_R_DEXE32)
!endif

all: $(PROJ).exe

.c.obj:
    $(CC) $(CFLAGS) /c $<

$(PROJ).exe: $(OBJS)
    echo >NUL @<<$(PROJ).CRF
$(LFLAGS)
$(OBJS)
-OUT:$(PROJ).exe
$(LIBS)
<<
    link @$(PROJ).CRF


$(PROJ).RES: $(PROJ).RC
  RC -r $(PROJ).RC

