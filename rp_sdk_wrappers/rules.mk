# NOTE: QMK adds divider and int64_ops wrappers

OPT_DEFS += \
    -DPICO_SDK_WRAPPERS_ENABLED \
    -DPICO_BITS_IN_RAM=1 \
    -DPICO_DOUBLE_IN_RAM=1 \
    -DPICO_FLOAT_IN_RAM=1

# bit
SRC += $(PICOSDKROOT)/src/rp2_common/pico_bit_ops/bit_ops_aeabi.S
EXTRALDFLAGS += \
    -Wl,--wrap=__clzsi2 \
    -Wl,--wrap=__clzdi2 \
    -Wl,--wrap=__ctzsi2 \
    -Wl,--wrap=__ctzdi2 \
    -Wl,--wrap=__popcountsi2 \
    -Wl,--wrap=__popcountdi2

# double
VPATH += $(PICOSDKROOT)/src/rp2_common/pico_double/include
SRC += \
    $(PICOSDKROOT)/src/rp2_common/pico_double/double_aeabi.S \
    $(PICOSDKROOT)/src/rp2_common/pico_double/double_init_rom.c \
    $(PICOSDKROOT)/src/rp2_common/pico_double/double_math.c \
    $(PICOSDKROOT)/src/rp2_common/pico_double/double_v1_rom_shim.S
EXTRALDFLAGS += \
    -Wl,--wrap=__aeabi_dadd \
    -Wl,--wrap=__aeabi_ddiv \
    -Wl,--wrap=__aeabi_dmul \
    -Wl,--wrap=__aeabi_drsub \
    -Wl,--wrap=__aeabi_dsub \
    -Wl,--wrap=__aeabi_cdcmpeq \
    -Wl,--wrap=__aeabi_cdrcmple \
    -Wl,--wrap=__aeabi_cdcmple \
    -Wl,--wrap=__aeabi_dcmpeq \
    -Wl,--wrap=__aeabi_dcmplt \
    -Wl,--wrap=__aeabi_dcmple \
    -Wl,--wrap=__aeabi_dcmpge \
    -Wl,--wrap=__aeabi_dcmpgt \
    -Wl,--wrap=__aeabi_dcmpun \
    -Wl,--wrap=__aeabi_i2d \
    -Wl,--wrap=__aeabi_l2d \
    -Wl,--wrap=__aeabi_ui2d \
    -Wl,--wrap=__aeabi_ul2d \
    -Wl,--wrap=__aeabi_d2iz \
    -Wl,--wrap=__aeabi_d2lz \
    -Wl,--wrap=__aeabi_d2uiz \
    -Wl,--wrap=__aeabi_d2ulz \
    -Wl,--wrap=__aeabi_d2f \
    -Wl,--wrap=sqrt \
    -Wl,--wrap=cos \
    -Wl,--wrap=sin \
    -Wl,--wrap=tan \
    -Wl,--wrap=atan2 \
    -Wl,--wrap=exp \
    -Wl,--wrap=log \
    -Wl,--wrap=ldexp \
    -Wl,--wrap=copysign \
    -Wl,--wrap=trunc \
    -Wl,--wrap=floor \
    -Wl,--wrap=ceil \
    -Wl,--wrap=round \
    -Wl,--wrap=sincos \
    -Wl,--wrap=asin \
    -Wl,--wrap=acos \
    -Wl,--wrap=atan \
    -Wl,--wrap=sinh \
    -Wl,--wrap=cosh \
    -Wl,--wrap=tanh \
    -Wl,--wrap=asinh \
    -Wl,--wrap=acosh \
    -Wl,--wrap=atanh \
    -Wl,--wrap=exp2 \
    -Wl,--wrap=log2 \
    -Wl,--wrap=exp10 \
    -Wl,--wrap=log10 \
    -Wl,--wrap=pow \
    -Wl,--wrap=powint \
    -Wl,--wrap=hypot \
    -Wl,--wrap=cbrt \
    -Wl,--wrap=fmod \
    -Wl,--wrap=drem \
    -Wl,--wrap=remainder \
    -Wl,--wrap=remquo \
    -Wl,--wrap=expm1 \
    -Wl,--wrap=log1p \
    -Wl,--wrap=fma

# float
VPATH += $(PICOSDKROOT)/src/rp2_common/pico_float/include
SRC += \
    $(PICOSDKROOT)/src/rp2_common/pico_float/float_aeabi.S \
    $(PICOSDKROOT)/src/rp2_common/pico_float/float_init_rom.c \
    $(PICOSDKROOT)/src/rp2_common/pico_float/float_math.c \
    $(PICOSDKROOT)/src/rp2_common/pico_float/float_v1_rom_shim.S

EXTRALDFLAGS += \
    -Wl,--wrap=__aeabi_fadd \
    -Wl,--wrap=__aeabi_fdiv \
    -Wl,--wrap=__aeabi_fmul \
    -Wl,--wrap=__aeabi_frsub \
    -Wl,--wrap=__aeabi_fsub \
    -Wl,--wrap=__aeabi_cfcmpeq \
    -Wl,--wrap=__aeabi_cfrcmple \
    -Wl,--wrap=__aeabi_cfcmple \
    -Wl,--wrap=__aeabi_fcmpeq \
    -Wl,--wrap=__aeabi_fcmplt \
    -Wl,--wrap=__aeabi_fcmple \
    -Wl,--wrap=__aeabi_fcmpge \
    -Wl,--wrap=__aeabi_fcmpgt \
    -Wl,--wrap=__aeabi_fcmpun \
    -Wl,--wrap=__aeabi_i2f \
    -Wl,--wrap=__aeabi_l2f \
    -Wl,--wrap=__aeabi_ui2f \
    -Wl,--wrap=__aeabi_ul2f \
    -Wl,--wrap=__aeabi_f2iz \
    -Wl,--wrap=__aeabi_f2lz \
    -Wl,--wrap=__aeabi_f2uiz \
    -Wl,--wrap=__aeabi_f2ulz \
    -Wl,--wrap=__aeabi_f2d \
    -Wl,--wrap=sqrtf \
    -Wl,--wrap=cosf \
    -Wl,--wrap=sinf \
    -Wl,--wrap=tanf \
    -Wl,--wrap=atan2f \
    -Wl,--wrap=expf \
    -Wl,--wrap=logf \
    -Wl,--wrap=ldexpf \
    -Wl,--wrap=copysignf \
    -Wl,--wrap=truncf \
    -Wl,--wrap=floorf \
    -Wl,--wrap=ceilf \
    -Wl,--wrap=roundf \
    -Wl,--wrap=sincosf \
    -Wl,--wrap=asinf \
    -Wl,--wrap=acosf \
    -Wl,--wrap=atanf \
    -Wl,--wrap=sinhf \
    -Wl,--wrap=coshf \
    -Wl,--wrap=tanhf \
    -Wl,--wrap=asinhf \
    -Wl,--wrap=acoshf \
    -Wl,--wrap=atanhf \
    -Wl,--wrap=exp2f \
    -Wl,--wrap=log2f \
    -Wl,--wrap=exp10f \
    -Wl,--wrap=log10f \
    -Wl,--wrap=powf \
    -Wl,--wrap=powintf \
    -Wl,--wrap=hypotf \
    -Wl,--wrap=cbrtf \
    -Wl,--wrap=fmodf \
    -Wl,--wrap=dremf \
    -Wl,--wrap=remainderf \
    -Wl,--wrap=remquof \
    -Wl,--wrap=expm1f \
    -Wl,--wrap=log1pf \
    -Wl,--wrap=fmaf

# mem functions are harder to adapt, due to ChibiOS' bootstrap code (in crt1.c)
# SRC += $(PICOSDKROOT)/src/rp2_common/pico_mem_ops/mem_ops_aeabi.S
# memcpy __aeabi_memcpy __aeabi_memcpy4 __aeabi_memcpy8 memset __aeabi_memset __aeabi_memset4 __aeabi_memset8
