#
# Makefile
#

CIRCLEHOME = ../..

#  CPPFLAGS	+= -E


OBJS	= \
   ili9486.o \
   xpt2046.o \
		

libdevices.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include ../myRules.mk

