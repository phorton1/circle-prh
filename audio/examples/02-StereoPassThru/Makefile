#
# Makefile
#

CIRCLEHOME = ../../../../../circle

OBJS = 02-StereoPassThru.o

MAKE_LIBS = \
	../std_kernel.mark \
	../../libaudio.mark \
	../../ui/libaudio_ui.mark \
	$(CIRCLEHOME)/addon/ugui/libugui.mark
	

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

# for some reason the usb library requires the file system library
# i hope this is not a conflict if I want to use the addon/fatfs in the future


include $(CIRCLEHOME)/Rules.mk
