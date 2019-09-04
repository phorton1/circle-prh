#
# Makefile
#

CIRCLEHOME = ../../..

# CFLAGS   =  -fPIE
# CPPFLAGS =  -fPIE 		-fno-function-cse 
# # OPTIMIZE ?= -O0


OBJS = program.o start.o

#		-fpic -shared \
#		-fPIE -shared \
# 		-fPIE -shared \
	
# SEE ../07-LinkerTest/loader.cpp for constant offset
# for --section-start !!

program.img : $(OBJS) program.ld
	@echo "  creating  program.img"
	
	@$(LD) -o program.elf 						\
		-Map program.map 						\
		--section-start=.init=0x80000 			\
		-T program.ld 							\
		$(CRTBEGIN) 							\
		$(OBJS) 								\
		--just-symbols 							\
		../07-LinkerTest/kernel7.elf				\
		--start-group 							\
		--end-group $(CRTEND)
		
	@echo "  DUMP  program.lst"
	@$(PREFIX)objdump -d program.elf | $(PREFIX)c++filt > program.lst
	
	@echo "  COPY  program.img"
	@$(PREFIX)objcopy program.elf -O binary program.img
	
	@echo -n "  WC    program.img => "
	@wc -c < program.img


include ../../myRules.mk
