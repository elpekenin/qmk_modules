# whether to wrap standard library's memory allocation functions to gather extra info
ALLOCATOR_WRAP_STD ?= no
ifeq ($(strip $(ALLOCATOR_WRAP_STD)), yes)
    LDFLAGS += \
        -Wl,--wrap=malloc \
        -Wl,--wrap=free \
        -Wl,--wrap=calloc \
        -Wl,--wrap=realloc

    OPT_DEFS += -DALLOCATOR_WRAP_STD=1
    SRC += $(MODULE_PATH_ALLOCATOR)/std_wrappers.cc
else
    OPT_DEFS += -DALLOCATOR_WRAP_STD=0
endif
