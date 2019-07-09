#
# Makefile
#

CIRCLEHOME = ../..

# CPPFLAGS += -Wno-write-strings

OBJS = \
	wsDC.o \
	wsApp.o \
	wsFont.o \
	wsRect.o \
	wsWindow.o \
	wsControl.o \
	wsTopWindow.o \
	wsThemeStandard.o \
	

libws.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/sample/Rules.mk
