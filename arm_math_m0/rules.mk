MATH_M0_MOD := $(realpath $(dir $(lastword $(MAKEFILE_LIST))))

VPATH += $(MATH_M0_MOD)/src/src/include

$(error Does not currently work: seems like QMK doesn't like .s files)
SRC += $(wildcard $(MATH_M0_MOD)/src/src/*/*.s)
