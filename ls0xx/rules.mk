QUANTUM_PAINTER_NEEDS_SURFACE := yes

# FIXME: not sure if this here is good enough, may be processed *after*
# qp's internal makefile and not impact it
QUANTUM_PAINTER_NEEDS_COMMS_SPI := yes

SRC += $(MODULE_PATH_LS0XX)/ls0xx.c
