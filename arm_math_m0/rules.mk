$(error Does not currently work: seems like QMK doesn't like .s files)

VPATH += \
    $(MODULE_PATH_ARM_MATH_M0)/src/src/include \
    $(MODULE_PATH_ARM_MATH_M0)/src/src/Denormals

SRC += $(wildcard $(MODULE_PATH_ARM_MATH_M0)/src/src/*/*.s)
