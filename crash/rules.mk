CRASH_MOD := $(realpath $(dir $(lastword $(MAKEFILE_LIST))))

CFLAGS += \
    -funwind-tables \
    -Wframe-address \
    -mpoke-function-name \
    -fno-omit-frame-pointer

VPATH += $(CRASH_MOD)/backtrace/include

SRC += backtrace/backtrace/backtrace.c
