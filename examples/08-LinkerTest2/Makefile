#
# Makefile
#

CIRCLEHOME = ../../..

OBJS = shell.o loader.o

# MAKE_LIBS = \
# 	$(CIRCLEHOME)/_prh/ws/libws.mark \
# 	$(CIRCLEHOME)/_prh/audio/libaudio.mark \
# 	$(CIRCLEHOME)/_prh/system/std_kernel.mark \
# 	$(CIRCLEHOME)/_prh/devices/lib_devices.mark \


$(TARGET).$(TARGET_SUFFIX) : $(OBJS) $(LIBS) $(CIRCLEHOME)/circle.ld $(MAKE_LIBS)
	@echo "  LD    $(TARGET).elf"
	@$(LD) -o $(TARGET).elf -Map $(TARGET).map --section-start=.init=$(LOADADDR) \
		-T kernel.ld $(CRTBEGIN) $(OBJS) \
		--start-group $(LIBS) $(EXTRALIBS) --end-group $(CRTEND)
	@echo "  DUMP  $(TARGET).lst"
	@$(PREFIX)objdump -d $(TARGET).elf | $(PREFIX)c++filt > $(TARGET).lst
	@echo "  COPY  $(TARGET).$(TARGET_SUFFIX)"
	@$(PREFIX)objcopy $(TARGET).elf -O binary $(TARGET).$(TARGET_SUFFIX)
	@echo -n "  WC    $(TARGET).$(TARGET_SUFFIX) => "
	@wc -c < $(TARGET).$(TARGET_SUFFIX)
		

include ../../myRules.mk
