KASAN_GLOBALS ?= 1
KASAN_STACK ?= 0  # seems to cause crash
KASAN_ALLOCAS ?= 1

# params in the order seen at https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html
# NOTE: threshold=0 forces compiler to use callbacks instead of inlining code
CFLAGS += \
    -fsanitize=kernel-address \
    --param asan-globals=$(KASAN_GLOBALS) \
    --param asan-stack=$(KASAN_STACK) \
    --param asan-memintrin=0 \
    --param asan-use-after-return=0 \
    --param asan-instrumentation-with-call-threshold=0 \
    --param asan-instrument-allocas=$(KASAN_ALLOCAS)

# FIXME: crashes
# needed to track dynamic memory allocation, (un)poisoning affected regions
# EXTRALDFLAGS += \
#     -Wl,--wrap=free \
#     -Wl,--wrap=malloc

OPT_DEFS += \
    -DKASAN_GLOBALS=$(KASAN_GLOBALS) \
    -DKASAN_STACK=$(KASAN_STACK) \
    -DKASAN_ALLOCAS=$(KASAN_ALLOCAS)

SRC += $(MODULE_PATH_SANITIZER)/asan.c
