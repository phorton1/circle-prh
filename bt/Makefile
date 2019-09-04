#
# Makefile
#

CIRCLEHOME = ../../../circle


OBJS	= \
	btQueue.o \
	btTask.o \
	uartTransportBase.o \
	transportUart.o \
	transportMiniUart.o \
	hciBase.o \
	hciVendor.o \
	hciLayer.o \
	hciEvent.o \
	bluetooth.o \
	lcapLayer.o \
	sdpLayer.o \
	rfLayer.o \
	sdp_parser.o
	
libbluetooth.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

EXTRACLEAN = BCM43430A1.h BCM43435C0.h  converttool

include ../myRules.mk

%.h: %.hcd
	@echo "  GEN   $@"
	@./converttool -b $< > $@

btdevicemanager.o: BCM43430A1.h BCM4345C0.h

BCM43430A1.h: converttool

BCM43435C0.h: converttool


converttool: converttool.c
	@echo "  TOOL  $@"
	@gcc -o converttool converttool.c

