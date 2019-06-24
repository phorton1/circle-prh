#
# Makefile
#

CIRCLEHOME = ../../..

CPPFLAGS += -Wno-write-strings

OBJS	= \
	controlwindow.o \
	scopewindow.o  \
	timeline.o \
	vu_meter.o \

libaudio_ui.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/sample/Rules.mk
