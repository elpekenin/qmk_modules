KASAN_GLOBALS ?= 1
KASAN_STACK ?= 1
KASAN_ALLOCAS ?= 1

# params in the order seen at https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html
# NOTE: threshold=0 forces compiler to use callbacks instead of inlining code
CFLAGS += \
    -fsanitize=kernel-address \
    --param asan-globals=$(KASAN_GLOBALS) \
    --param asan-stack=$(KASAN_STACK) \
    --param asan-use-after-return=0 \
    --param asan-instrumentation-with-call-threshold=0 \
    --param asan-instrument-allocas=$(KASAN_ALLOCAS)

SRC += $(MODULE_PATH_SANITIZER)/asan.c
