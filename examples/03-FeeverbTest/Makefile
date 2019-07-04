#
# Makefile
#

CIRCLEHOME = ../../..

OBJS = 03-FeeverbTest.o

MAKE_LIBS = \
	../../kernel/std_kernel.mark \
	../../audio/libaudio.mark \
	../../ui/libui.mark \
	../../devices/libdevices.mark \
	$(CIRCLEHOME)/addon/ugui/libugui.mark \
	

LIBS	= \
	../../kernel/std_kernel.a \
	../../audio/libaudio.a \
	../../ui/libui.a\
	../../devices/libdevices.a \
	$(CIRCLEHOME)/lib/libcircle.a \
	$(CIRCLEHOME)/lib/sched/libsched.a \
	\
	$(CIRCLEHOME)/addon/ugui/libugui.a \
	$(CIRCLEHOME)/lib/usb/libusb.a \
    $(CIRCLEHOME)/lib/fs/libfs.a \
	$(CIRCLEHOME)/lib/input/libinput.a \

	

include $(CIRCLEHOME)/Rules.mk
