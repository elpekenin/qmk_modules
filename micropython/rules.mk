EMBED_DIR := $(MODULE_PATH_MICROPYTHON)/micropython_embed

#
# MicroPy config selection.
##############################################################################

ifneq ("$(wildcard $(KEYMAP_PATH)/mpconfigport.h)","")
	MPCONFDIR = $(KEYMAP_PATH)
else ifneq ("$(wildcard $(KEYBOARD_PATH_5)/mpconfigport.h)","")
	MPCONFDIR = $(KEYBOARD_PATH_5)
else ifneq ("$(wildcard $(KEYBOARD_PATH_4)/mpconfigport.h)","")
	MPCONFDIR = $(KEYBOARD_PATH_4)
else ifneq ("$(wildcard $(KEYBOARD_PATH_3)/mpconfigport.h)","")
	MPCONFDIR = $(KEYBOARD_PATH_3)
else ifneq ("$(wildcard $(KEYBOARD_PATH_2)/mpconfigport.h)","")
	MPCONFDIR = $(KEYBOARD_PATH_2)
else ifneq ("$(wildcard $(KEYBOARD_PATH_1)/mpconfigport.h)","")
	MPCONFDIR = $(KEYBOARD_PATH_1)
else
	MPCONFDIR = $(MODULE_PATH_MICROPYTHON)
endif

MP_CONFIGFILE = $(abspath $(MPCONFDIR))/mpconfigport.h

# generate files by invoking the makefile pointing at the configuration file found
# need to redirect its output to NULL, otherwise make gets confused with the command's output
# NOTE: chaining multiple ones (eg: userspace -> keyboard -> module) probably doesn't work fine now
#       im not even sure the inclusion of module's configfile is robust as of now
# TODO: Doesn't seem to regenerate when config files are changed, find a way to fix that
$(shell \
    MP_CONFIGFILE="$(MP_CONFIGFILE)" \
    $(MAKE) \
    -f micropython_embed.mk \
    -C $(MODULE_PATH_MICROPYTHON) \
    > /dev/null)

#
# MicroPy compilation along the rest of QMK
##############################################################################
VPATH += $(MODULE_PATH_MICROPYTHON) \
         $(EMBED_DIR) \
         $(EMBED_DIR)/port

# without MP_CONFIGFILE, compilation will use the module configuration, and not the user overrides
CFLAGS += -DMP_CONFIGFILE=\"$(MP_CONFIGFILE)\"

MICROPY_SRC := $(wildcard $(EMBED_DIR)/*/*.c) $(wildcard $(EMBED_DIR)/*/*/*.c)
QMK_MOD_SRC := $(wildcard $(MODULE_PATH_MICROPYTHON)/user_c_modules/*/*.c)

SRC += \
    $(MODULE_PATH_MICROPYTHON)/mphal.c \
    $(MICROPY_SRC) \
    $(QMK_MOD_SRC)
