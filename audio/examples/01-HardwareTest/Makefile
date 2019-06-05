#
# Makefile
#

CIRCLEHOME = ../../../../../circle

OBJS = hardware_test.o

MAKE_LIBS = \
	../std_kernel.mark \
	../../libaudio.mark \
	$(CIRCLEHOME)/lib/sched/libsched.mark \


LIBS	= \
	../std_kernel.a \
	../../libaudio.a \
	$(CIRCLEHOME)/lib/libcircle.a \
	$(CIRCLEHOME)/lib/sched/libsched.a \
	

include $(CIRCLEHOME)/Rules.mk
