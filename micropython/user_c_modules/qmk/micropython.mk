QMK_MOD_DIR := $(USERMOD_DIR)
SRC_USERMOD_C += $(wildcard $(QMK_MOD_DIR)/*.c)
QSTR_DEFS += $(QMK_MOD_DIR)/qmk_qstrdefs.h
