#
# Makefile
#

CIRCLEHOME = ../../..

CPPFLAGS += -Wno-write-strings

OBJS = \
	app.o \
	menu.o \
	window_main.o \
	window_status.o \
	window_record.o \
 	vu_meter.o \


# OBJS	= \
# 	controlwindow.o \
# 	scopewindow.o  \
# 	timeline.o \

libaudio_ui.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/sample/Rules.mk
