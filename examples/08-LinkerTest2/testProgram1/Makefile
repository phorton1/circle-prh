#
# Makefile
#

CIRCLEHOME = ../../../..

# CFLAGS   =  -fPIE
# CPPFLAGS =  -fPIE 		-fno-function-cse 
# # OPTIMIZE ?= -O0

PROG_NAME = program1
LOAD_LOCATION = 0xC0000


OBJS = $(PROG_NAME).o start.o

#		-fpic -shared \
#		-fPIE -shared \
# 		-fPIE -shared \
	
# SEE ../07-LinkerTest/loader.cpp for constant offset
# for --section-start !!

$(PROG_NAME).img : $(OBJS) $(PROG_NAME).ld
	@echo "  creating  $(PROG_NAME).img"
	
	@$(LD) -o $(PROG_NAME).elf 						\
		-Map $(PROG_NAME).map 						\
		--section-start=.init=$(LOAD_LOCATION)		\
		-T $(PROG_NAME).ld 							\
		$(CRTBEGIN) 							\
		$(OBJS) 								\
		--just-symbols 							\
		../kernel7.elf				\
		--start-group 							\
		--end-group $(CRTEND)
		
	@echo "  DUMP  $(PROG_NAME).lst"
	@$(PREFIX)objdump -d $(PROG_NAME).elf | $(PREFIX)c++filt > $(PROG_NAME).lst
	
	@echo "  COPY  $(PROG_NAME).img"
	@$(PREFIX)objcopy $(PROG_NAME).elf -O binary $(PROG_NAME).img
	
	@echo -n "  WC    $(PROG_NAME).img => "
	@wc -c < $(PROG_NAME).img


include $(CIRCLEHOME)/Rules.mk
