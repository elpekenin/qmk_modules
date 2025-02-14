# This file is part of the MicroPython project, http://micropython.org/
# The MIT License (MIT)
# Copyright (c) 2022-2023 Damien P. George

MICROPYTHON_TOP = src

# provided by rules.mk, pipe into CFLAGS
# can't use CFLAGS=... $(MAKE) from the other makefile because it overrides the variable completely
CFLAGS += -DMP_CONFIGFILE=\"$(MP_CONFIGFILE)\"

USER_C_MODULES = modules

include $(MICROPYTHON_TOP)/ports/embed/embed.mk
