#
# Makefile
#

CIRCLEHOME = ../../


OBJS	= \
    main.o \
	std_kernel.o \

libsystem.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk
