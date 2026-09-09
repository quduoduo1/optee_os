make O=out/fvp-arm64 V=1 -j1 \
    COMPILER=clang \
    PLATFORM=vexpress \
    PLATFORM_FLAVOR=fvp \
    CFG_CORE_SEL2_SPMC=y \
    CFG_ARM64_core=y \
    CFG_DT=y \
    CFG_USER_TA_TARGETS=ta_arm64 \
    CROSS_COMPILE32=arm-linux-gnueabihf- \
    CROSS_COMPILE64=aarch64-linux-gnu- \
    CROSS_COMPILE_core=aarch64-linux-gnu- \
    CROSS_COMPILE_ta_arm64=aarch64-linux-gnu-
