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


LIBS	= \
	$(CIRCLEHOME)/lib/usb/libusb.a \
	$(CIRCLEHOME)/lib/input/libinput.a \
	$(CIRCLEHOME)/lib/fs/libfs.a \
	$(CIRCLEHOME)/lib/sched/libsched.a \
	$(CIRCLEHOME)/lib/libcircle.a \
	$(CIRCLEHOME)/addon/SDCard/libsdcard.a \
	$(CIRCLEHOME)/addon/fatfs/libfatfs.a \
	$(CIRCLEHOME)/lib/fs/fat/libfatfs.a \
	$(CIRCLEHOME)/lib/net/libnet.a \


TARGET_ROOT = recovery

CONTENT	= index.h style.h favicon.h

EXTRACLEAN = $(CONTENT) converttool


ifeq ($(strip $(RASPPI)),1)
COPY_IMG	= kernel.img
else
COPY_IMG	= kernel7.img
endif

all:	$(COPY_IMG)


include $(CIRCLEHOME)/sample/Rules.mk


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
