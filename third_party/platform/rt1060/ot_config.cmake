# Copyright 2024-2026 NXP
# SPDX-License-Identifier: BSD-3-Clause

# RT1060 OpenThread Platform Configuration
message(STATUS "Loading RT1060 OpenThread configuration")

# RT1060 specific library directory
set(OT_LIB_DIR ${OT_SRC_DIR}/build_rt1060/iw610_spi/lib)
message(STATUS "Build with RT1060 ot-cli libs, path=${OT_LIB_DIR}")

# RT1060 specific include directories
target_include_directories(${MCUX_SDK_PROJECT_NAME} PUBLIC
    ${OT_SRC_DIR}/src/imx_rt/rt1060
    #${SdkRootDirPath}/middleware/mbedtls3x/framework/psasim/include
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
    ${OT_LIB_DIR}/libot-cli-rt1060-freertos.a
    ${OT_LIB_DIR}/libopenthread-url.a
    ${OT_LIB_DIR}/libopenthread-radio-spinel.a
    ${OT_LIB_DIR}/libtcplp-ftd.a
)

TARGET_LINK_LIBRARIES(${MCUX_SDK_PROJECT_NAME} PRIVATE -Wl,--end-group)
