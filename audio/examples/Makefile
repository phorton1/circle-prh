#
# Makefile
#

CIRCLEHOME = ../../../


OBJS	= \
    main.o \
	std_kernel.o \
	statusScreen.o \


std_kernel.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk
