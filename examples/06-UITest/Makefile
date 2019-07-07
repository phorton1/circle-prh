#
# Makefile
#

CIRCLEHOME = ../../..

OBJS = kernel.o main.o

MAKE_LIBS = \
	../../ws/libws.mark \
	
LIBS	= \
	$(CIRCLEHOME)/lib/libcircle.a \
	$(CIRCLEHOME)/lib/input/libinput.a \
	$(CIRCLEHOME)/lib/usb/libusb.a \
	$(CIRCLEHOME)/lib/sched/libsched.a \
    $(CIRCLEHOME)/lib/fs/libfs.a \
	../../ws/libws.a \


include $(CIRCLEHOME)/Rules.mk
