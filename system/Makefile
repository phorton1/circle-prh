#
# Makefile
#

# USE_MIDI_SYSTEM = 1
	# must match definition in std_kernel.h


CIRCLEHOME = ../../

OBJS	= \
    main.o \
	std_kernel.o \
	std_empty_ui.o \
	std_null_audio_system.o \
	midiEvent.o
	
# ifeq ($(strip $(USE_MIDI_SYSTEM)),1)
# endif	

libsystem.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include ../myRules.mk

