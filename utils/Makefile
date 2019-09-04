#
# Makefile
#

CIRCLEHOME ?= ../../

OBJS	= \
    myUtils.o \
	miniuart.o \

lib_my_utils.a : $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include ../myRules.mk
