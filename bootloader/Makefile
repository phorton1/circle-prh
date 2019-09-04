#
# Makefile
#


CIRCLEHOME = ../..

# INCLUDE += -I ../

OBJS	= \
	main.o \
	kernel.o \
	softserial.o \
	httpbootserver.o \
	tftpbootserver.o \


TARGET_ROOT = recovery

CONTENT	= index.h style.h favicon.h

EXTRACLEAN = $(CONTENT) converttool


ifeq ($(strip $(RASPPI)),1)
COPY_IMG	= kernel.img
else
COPY_IMG	= kernel7.img
endif

all:	$(COPY_IMG)


include ../myRules.mk


$(COPY_IMG): $(TARGET).$(TARGET_SUFFIX)
	@echo "  CP  $(COPY_IMG)"
	cp $(TARGET).$(TARGET_SUFFIX)  $(COPY_IMG)
	


%.h: %.html
	@echo "  GEN   $@"
	@./converttool $< > $@

%.h: %.css
	@echo "  GEN   $@"
	@./converttool $< > $@

%.h: %.ico
	@echo "  GEN   $@"
	@./converttool -b $< > $@

httpbootserver.o: $(CONTENT)

$(CONTENT): converttool

converttool: converttool.c
	@echo "  TOOL  $@"
	@gcc -o converttool converttool.c
