# tell ChibiOS' crt0_v6m.S to enable second core
OPT_DEFS += \
    -UCRT0_EXTRA_CORES_NUMBER \
    -DCRT0_EXTRA_CORES_NUMBER=1
