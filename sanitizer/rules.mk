CFLAGS += -fsanitize=address

# make linker find our (empty) shim
LDFLAGS += -L $(MODULE_PATH_SANITIZER)

SRC += $(MODULE_PATH_SANITIZER)/asan.c
