#
# Makefile
#

CIRCLEHOME ?= ../../

OBJS	= \
    myUtils.o \

lib_my_utils.a : $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk
