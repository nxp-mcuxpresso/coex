# Copyright 2024-2026 NXP
# SPDX-License-Identifier: BSD-3-Clause

# RT1060 OpenThread Platform Configuration
message(STATUS "Loading RT1060 OpenThread configuration")

# RT1060 specific library directory
set(OT_LIB_DIR ${OT_SRC_DIR}/build_rt1060/rt1060_ot_cli/lib)
message(STATUS "Build with RT1060 ot-cli libs, path=${OT_LIB_DIR}")

# RT1060 specific include directories
target_include_directories(${MCUX_SDK_PROJECT_NAME} PUBLIC
    ${OT_SRC_DIR}/src/imx_rt/rt1060
    #${SdkRootDirPath}/middleware/mbedtls3x/framework/psasim/include
)

# Add LPSPI adapter for OpenThread SPI interface
mcux_add_source(
    BASE_PATH ${SdkRootDirPath}
    SOURCES components/spi/fsl_adapter_lpspi.c
            components/spi/fsl_adapter_spi.h
            drivers/lpspi/fsl_lpspi.c
)
mcux_add_include(
    BASE_PATH ${SdkRootDirPath}
    INCLUDES components/spi
             drivers/lpspi
)

mcux_add_source(
    BASE_PATH ${SdkRootDirPath}
    SOURCES middleware/wireless/framework/services/FileSystem/fwk_filesystem.c
            middleware/wireless/framework/services/FileSystem/fwk_filesystem.h
            middleware/wireless/coex/build/evkcmimxrt1060/common/peripherals.c
)
mcux_add_include(
    BASE_PATH ${SdkRootDirPath}
    INCLUDES middleware/wireless/framework/services/FileSystem
             middleware/wireless/framework/platform/rt1060
             middleware/wireless/coex/build/evkcmimxrt1060/common
)

target_compile_definitions(${MCUX_SDK_PROJECT_NAME} PUBLIC
    MBEDTLS_MCUX_ENTROPY=1
)

# Link RT1060 OpenThread libraries
TARGET_LINK_LIBRARIES(${MCUX_SDK_PROJECT_NAME} PRIVATE -Wl,--start-group)

target_link_libraries(${MCUX_SDK_PROJECT_NAME} PRIVATE
    ${OT_LIB_DIR}/libopenthread-cli-ftd.a
    ${OT_LIB_DIR}/libopenthread-cli-ftd.a
    ${OT_LIB_DIR}/libopenthread-ftd.a
    ${OT_LIB_DIR}/libopenthread-hdlc.a
    ${OT_LIB_DIR}/libopenthread-ncp-ftd.a
    ${OT_LIB_DIR}/libopenthread-platform.a
    ${OT_LIB_DIR}/libopenthread-rt1060.a
    ${OT_LIB_DIR}/libopenthread-spinel-ncp.a
    ${OT_LIB_DIR}/libot-cli-addons.a
    ${OT_LIB_DIR}/libot-cli-rt1060.a
    ${OT_LIB_DIR}/libopenthread-url.a
    ${OT_LIB_DIR}/libopenthread-radio-spinel.a
    ${OT_LIB_DIR}/libtcplp-ftd.a
)

TARGET_LINK_LIBRARIES(${MCUX_SDK_PROJECT_NAME} PRIVATE -Wl,--end-group)
