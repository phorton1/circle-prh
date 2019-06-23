#
# Makefile
#

CIRCLEHOME = ../../..

CPPFLAGS += -Wno-write-strings

OBJS	= controlwindow.o scopewindow.o  timeline.o 

# scopeconfig.o

# main.o kernel.o 		recorder.o sampler.o

libaudio_ui.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/sample/Rules.mk
