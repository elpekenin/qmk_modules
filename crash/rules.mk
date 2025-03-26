CFLAGS += \
    -funwind-tables \
    -Wframe-address \
    -mpoke-function-name \
    -fno-omit-frame-pointer

VPATH += $(MODULE_PATH_CRASH)/backtrace/include

SRC += $(MODULE_PATH_CRASH)/backtrace/backtrace/backtrace.c
