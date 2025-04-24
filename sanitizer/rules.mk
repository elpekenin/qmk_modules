# make linker find our (empty) shims
LDFLAGS += -L $(MODULE_PATH_SANITIZER)

# ASAN
CFLAGS += -fsanitize=address
SRC += $(MODULE_PATH_SANITIZER)/asan.c
