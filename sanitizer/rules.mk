CFLAGS += \
    -fsanitize=kernel-address \
    --param asan-globals=1 \
    --param asan-instrumentation-with-call-threshold=0 \
    --param asan-instrument-allocas=1 \
    --param asan-stack=1

SRC += $(MODULE_PATH_SANITIZER)/asan.c
