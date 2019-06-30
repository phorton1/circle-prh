#
# Makefile
#

CIRCLEHOME = ../..

#  CPPFLAGS	+= -E

OBJS = \
	analyze_peak.o \
	analyze_rms.o \
	arm_q15_to_q31.o \
	arm_shift_q31.o \
	arm_add_q31.o \
	arm_q31_to_q15.o \
	arm_float_to_q31.o \
	AudioConnection.o \
	AudioStream.o \
	AudioSystem.o \
	bcm_pcm.o \
	control_wm8731.o \
	control_cs42448.o \
	control_sgtl5000.o \
	effect_freeverb.o \
	effect_reverb.o \
	input_i2s.o \
	input_tdm.o \
	input_teensy_quad.o \
	mixer.o \
	output_teensy_quad.o \
	output_i2s.o \
	output_tdm.o \
	recorder.o \
	synth_sine.o \
	Wire.o \
	

libaudio.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk
