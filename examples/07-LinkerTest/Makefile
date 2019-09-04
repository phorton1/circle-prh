#
# Makefile
#

CIRCLEHOME = ../../..

OBJS	= main.o kernel.o loader.o

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
