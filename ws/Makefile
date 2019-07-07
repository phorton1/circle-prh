#
# Makefile
#

CIRCLEHOME = ../..

# CPPFLAGS += -Wno-write-strings

OBJS = \
	wsDC.o \
	wsApp.o \
	wsFont.o \
	wsWindow.o \
	wsThemeStandard.o \
	

libws.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/sample/Rules.mk
