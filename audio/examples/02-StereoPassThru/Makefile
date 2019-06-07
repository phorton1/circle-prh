#
# Makefile
#

CIRCLEHOME = ../../../../../circle

OBJS = 02-StereoPassThru.o

MAKE_LIBS = \
	../std_kernel.mark \
	../../libaudio.mark \

LIBS	= \
	../std_kernel.a \
	../../libaudio.a \
	$(CIRCLEHOME)/lib/libcircle.a \
	$(CIRCLEHOME)/lib/sched/libsched.a \
	

include $(CIRCLEHOME)/Rules.mk
