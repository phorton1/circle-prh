#
# Makefile
#

CIRCLEHOME = ../..

#  CPPFLAGS	+= -E


OBJS	= \
	Wire.o \
	arm_q15_to_q31.o \
	arm_shift_q31.o \
	arm_add_q31.o \
	arm_q31_to_q15.o \
	arm_float_to_q31.o \
    AudioStream.o \
	bcm_pcm.o \
	control_wm8731.o \
	control_cs42448.o \
	effect_freeverb.o \
	effect_reverb.o \
	input_i2s.o \
	mixer.o \
	output_i2s.o \
	synth_sine.o \
	

libaudio.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk
