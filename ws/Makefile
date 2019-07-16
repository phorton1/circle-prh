#
# Makefile
#

CIRCLEHOME = ../..

# CPPFLAGS += -Wno-write-strings

OBJS = \
	wsFont.o \
	wsRect.o \
	wsDC.o \
	wsWindow.o \
	wsTopWindow.o \
	wsControl.o \
	wsButton.o \
	wsCheckbox.o \
	wsApp.o \
	wsThemeStandard.o \
	wsMenu.o \
	

libws.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/sample/Rules.mk
