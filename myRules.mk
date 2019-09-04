# prh - this makefile is used by example and other programs
# built by my system, which do not include $CIRCLEHOME/Rules.mk
# directly.
#
# support for top-down recursive make and clean_all
# spsecify MAKE_LIBS variable in your top most makefile
# and generally specifies all circle libraries that must
# be (possibly) linked to ...


CIRCLEHOME ?= ../


LIBS	+= \
	$(CIRCLEHOME)/_prh/audio/libaudio.a \
	$(CIRCLEHOME)/_prh/devices/libdevices.a \
	$(CIRCLEHOME)/_prh/utils/lib_my_utils.a \
	$(CIRCLEHOME)/_prh/system/libsystem.a \
	$(CIRCLEHOME)/_prh/ws/libws.a \
	$(CIRCLEHOME)/lib/libcircle.a \
    $(CIRCLEHOME)/lib/fs/libfs.a \
	$(CIRCLEHOME)/lib/input/libinput.a \
	$(CIRCLEHOME)/lib/net/libnet.a \
	$(CIRCLEHOME)/lib/sched/libsched.a \
	$(CIRCLEHOME)/lib/usb/libusb.a \
    $(CIRCLEHOME)/addon/fatfs/libfatfs.a \
    $(CIRCLEHOME)/addon/SDCard/libsdcard.a \


%.a: 
	@make -C $(dir $@) 

%.mark: %.a
	@make -s -C $(dir $@) 

%.clean:
	@make clean_all -C $(dir $@) 

-include $(CIRCLEHOME)/Rules.mk

# this must come after including the base Rules.mk

clean_all: $(subst .mark,.clean,$(MAKE_LIBS))
	rm -f *.o *.a *.elf *.lst *.$(TARGET_SUFFIX) *.hex *.cir *.map *~ $(EXTRACLEAN)

