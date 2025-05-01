# kasan
KASAN_GLOBALS ?= 1
KASAN_STACK ?= 0
KASAN_ALLOCAS ?= 1
KASAN_MALLOC ?= 1

CFLAGS += -fsanitize=kernel-address

# NOTE: listed in the same order as https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html

# buffer overflow detection
CFLAGS += \
    --param asan-globals=$(KASAN_GLOBALS) \
    --param asan-stack=$(KASAN_STACK) \
    --param asan-instrument-reads=0 \
    --param asan-instrument-writes=0

CFLAGS += --param asan-memintrin=0 # disable detection on built-ins
CFLAGS += --param asan-use-after-return=1
# CFLAGS += --param asan-instrumentation-with-call-threshold=0 # force compiler to use callbacks instead of inlining code
CFLAGS += --param asan-instrument-allocas=$(KASAN_ALLOCAS)

ifeq ($(KASAN_MALLOC), 1)
    # track allocation, (un)poisoning affected regions
    EXTRALDFLAGS += \
        -Wl,--wrap=malloc \
        -Wl,--wrap=free \
        -Wl,--wrap=calloc \
        -Wl,--wrap=realloc
endif

OPT_DEFS += \
    -DKASAN_GLOBALS=$(KASAN_GLOBALS) \
    -DKASAN_ALLOCAS=$(KASAN_ALLOCAS)

SRC += $(MODULE_PATH_SANITIZER)/kasan.c
