# prh - this makefile generally modifies the behavior of 
# the base circle/Rules.mk for use with my source code.
# it is NOT used in the with the utils or bootloader subprojects

CIRCLEHOME ?= ../

# extra defines and include paths
# set PLUS3B to one with rRASSPI=3 for B+ to get
# correct .hcd file for bcm bluetooth

INCLUDE	+=  -I $(CIRCLEHOME)/_prh

# support for top-down recursive make and clean_all
# spsecify MAKE_LIBS variable in your top most makefile

%.a: 
	@make -C $(dir $@) 

%.mark: %.a
	@make -s -C $(dir $@) 

%.clean:
	@make clean_all -C $(dir $@) 

-include $(CIRCLEHOME)/Rules.mk

# this must come after including the base Rules.mk

clean_all: $(subst .mark,.clean,$(MAKE_LIBS))
	rm -f *.o *.a *.elf *.lst *.$(TARGET_SUFFIX) *.hex *.cir *.map *~ $(EXTRACLEAN)

