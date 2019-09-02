#
# Makefile
#

CIRCLEHOME = ../../


OBJS	= \
    main.o \
	std_kernel.o \
	std_empty_ui.o \

libsystem.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk
