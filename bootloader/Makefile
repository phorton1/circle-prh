# Makefile
#
# This is the only one of my makefiles that does NOT use
# myRules.mk, and calls circle's Rules.mk directly ..


CIRCLEHOME = ../..

# INCLUDE += -I ../

OBJS	= \
	main.o \
	kernel.o \



LIBS	+= \
	$(CIRCLEHOME)/_prh/utils/lib_my_utils.a \
	$(CIRCLEHOME)/lib/libcircle.a \
    $(CIRCLEHOME)/lib/fs/libfs.a \
	$(CIRCLEHOME)/lib/net/libnet.a \
	$(CIRCLEHOME)/lib/sched/libsched.a \
    $(CIRCLEHOME)/addon/fatfs/libfatfs.a \
    $(CIRCLEHOME)/addon/SDCard/libsdcard.a \


TARGET_ROOT = recovery


#	ifeq ($(strip $(RASPPI)),1)
#	COPY_IMG	= kernel.img
#	else
#	COPY_IMG	= kernel7.img
#	endif

#	all:	$(COPY_IMG)


include $(CIRCLEHOME)/Rules.mk


#	$(COPY_IMG): $(TARGET).$(TARGET_SUFFIX)
#		@echo "  CP  $(COPY_IMG)"
#		cp $(TARGET).$(TARGET_SUFFIX)  $(COPY_IMG)
	

