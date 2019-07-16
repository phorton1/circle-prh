#
# Makefile
#

CIRCLEHOME = ../../..

OBJS = 02-StereoPassThru.o

MAKE_LIBS = \
	$(CIRCLEHOME)/_prh/ws/libws.mark \
	$(CIRCLEHOME)/_prh/audio/libaudio.mark \
	$(CIRCLEHOME)/_prh/system/std_kernel.mark \
	$(CIRCLEHOME)/_prh/devices/lib_devices.mark \

LIBS	= \
	$(CIRCLEHOME)/_prh/ws/libws.a \
	$(CIRCLEHOME)/_prh/audio/libaudio.a \
	$(CIRCLEHOME)/_prh/system/libsystem.a \
	$(CIRCLEHOME)/_prh/devices/libdevices.a \
	$(CIRCLEHOME)/lib/libcircle.a \
	$(CIRCLEHOME)/lib/input/libinput.a \
	$(CIRCLEHOME)/lib/sched/libsched.a \
	$(CIRCLEHOME)/lib/usb/libusb.a \
    $(CIRCLEHOME)/lib/fs/libfs.a \
	

include $(CIRCLEHOME)/Rules.mk
