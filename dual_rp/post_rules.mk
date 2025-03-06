ifneq ($(MCU_SERIES), RP2040)
    $(call CATASTROPHIC_ERROR,Invalid MCU,This module can only be used on RP2040)
endif
