#
# Makefile
#

CIRCLEHOME = ../..

#  CPPFLAGS	+= -E


OBJS	= \
	Wire.o \
	bcm_pcm.o \
    AudioStream.o \
	control_wm8731.o \
	input_i2s.o \
	output_i2s.o \
	synth_sine.o \
	effect_reverb.o \
	arm_q15_to_q31.o \
	arm_shift_q31.o \
	arm_add_q31.o \
	arm_q31_to_q15.o \
	arm_float_to_q31.o \
	mixer.o \
	effect_freeverb.o \
	

libaudio.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk
