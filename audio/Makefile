#
# Makefile
#

CIRCLEHOME = ../..

#  CPPFLAGS	+= -E


OBJS	= \
    AudioStream.o \
	Wire.o \
	bcm_pcm.o \
	control_wm8731.o \
	synth_sine.o \
	input_i2s.o \
	output_i2s.o \
	

libaudio.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk
