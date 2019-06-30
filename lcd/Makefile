#
# Makefile
#

CIRCLEHOME = ../..

#  CPPFLAGS	+= -E


OBJS	= \
    LCDWIKI_GUI.o \
    LCDWIKI_KBV.o \
    _pins_arduino.o
	
# TouchScreen.o \
		

liblcd.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk
