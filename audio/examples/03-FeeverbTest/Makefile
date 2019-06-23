#
# Makefile
#

CIRCLEHOME = ../../../../../circle

OBJS = 03-FeeverbTest.o

MAKE_LIBS = \
	../std_kernel.mark \
	../../libaudio.mark \
	../../ui/libaudio_ui.mark \
	

LIBS	= \
	../std_kernel.a \
	../../libaudio.a \
	../../ui/libaudio_ui.a \
	$(CIRCLEHOME)/lib/libcircle.a \
	$(CIRCLEHOME)/lib/sched/libsched.a \
	\
	$(CIRCLEHOME)/addon/ugui/libugui.a \
	$(CIRCLEHOME)/lib/usb/libusb.a \
    $(CIRCLEHOME)/lib/fs/libfs.a \
	$(CIRCLEHOME)/lib/input/libinput.a \

	

include $(CIRCLEHOME)/Rules.mk
