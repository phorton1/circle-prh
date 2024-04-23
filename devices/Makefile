#
# Makefile
#

CIRCLEHOME = ../..

#  CPPFLAGS	+= -E


OBJS	= \
   ili_base.o \
   ili9486.o \
   ili9488.o \
   xpt2046.o \


libdevices.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include ../myRules.mk
