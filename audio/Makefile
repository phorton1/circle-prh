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
	analyze_peak.o \
	analyze_rms.o \
	control_wm8731.o \
	control_cs42448.o \
	control_sgtl5000.o \
	effect_freeverb.o \
	effect_reverb.o \
	input_i2s.o \
	input_tdm.o \
	input_teensy_quad.o \
	mixer.o \
	output_i2s.o \
	output_tdm.o \
	output_teensy_quad.o \
	recorder.o \
	synth_sine.o \
	

libaudio.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk
