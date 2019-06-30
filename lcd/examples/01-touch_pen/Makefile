#
# Makefile
#

CIRCLEHOME = ../../../../../circle

OBJS	= main.o kernel.o  touch_pen.o

MAKE_LIBS = \
	../../liblcd.mark 

LIBS	= \
	../../liblcd.a \
	$(CIRCLEHOME)/lib/libcircle.a \
	$(CIRCLEHOME)/lib/sched/libsched.a \
	

include $(CIRCLEHOME)/Rules.mk
