# Copyright 2024-2025 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
# The BSD-3-Clause license can be found at https://spdx.org/licenses/BSD-3-Clause.html

# ot-nxp config
if(${CONFIG_OT})
message(STATUS "CONFIG_OT=${CONFIG_OT}")

# path of ot-nxp repo/ot libs
if(EXISTS ${SdkRootDirPath}/SW-Content-Register.txt)
    set(SDK_TYPE "PACKAGE")
    set(OT_SRC_DIR ${ProjDirPath}/../ot-nxp)
    set(OT_LIB_DIR ${ProjDirPath}/../ot-nxp/build_rw612/rw612_ot_cli/lib)
else()
    set(SDK_TYPE "INTERNAL")
    set(OT_SRC_DIR ${SdkRootDirPath}/middleware/wireless/coex/third_party/ot-nxp)
    set(OT_LIB_DIR ${SdkRootDirPath}/middleware/wireless/coex/third_party/ot-nxp/build_rw612/rw612_ot_cli/lib)
endif()
message(STATUS "SDK_TYPE: ${SDK_TYPE}")
message(STATUS "Build with external ot-cli libs, path=${OT_LIB_DIR}")

target_compile_definitions(${MCUX_SDK_PROJECT_NAME}
    PUBLIC
    OPENTHREAD_CONFIG_FILE="openthread-core-rw612-config.h"
    OPENTHREAD_PROJECT_CORE_CONFIG_FILE="openthread-core-rw612-config.h"
    OPENTHREAD_CORE_CONFIG_PLATFORM_CHECK_FILE="openthread-core-rw612-config-check.h"
    OPENTHREAD_PROJECT_LIB_CONFIG_FILE="openthread-core-rw612-config.h"
)

target_include_directories(${MCUX_SDK_PROJECT_NAME} PUBLIC
    ${OT_SRC_DIR}/src/rw/rw612
    ${OT_SRC_DIR}/src/common
    ${OT_SRC_DIR}/openthread/src
    ${OT_SRC_DIR}/openthread/include
    ${OT_SRC_DIR}/openthread/src/core
    ${OT_SRC_DIR}/openthread/examples/platforms
    ${OT_SRC_DIR}/openthread/third_party/mbedtls
    ${OT_SRC_DIR}/third_party/mbedtls/configs
)

TARGET_LINK_LIBRARIES(${MCUX_SDK_PROJECT_NAME} PRIVATE -Wl,--start-group)

# ot libs
target_link_libraries(${MCUX_SDK_PROJECT_NAME} PRIVATE
    ${OT_LIB_DIR}/libopenthread-cli-ftd.a
    ${OT_LIB_DIR}/libopenthread-cli-ftd.a
    ${OT_LIB_DIR}/libopenthread-ftd.a
    ${OT_LIB_DIR}/libopenthread-hdlc.a
    ${OT_LIB_DIR}/libopenthread-ncp-ftd.a
    ${OT_LIB_DIR}/libopenthread-platform.a
    ${OT_LIB_DIR}/libopenthread-rw612.a
    ${OT_LIB_DIR}/libopenthread-spinel-ncp.a
    ${OT_LIB_DIR}/libot-cli-addons.a
    ${OT_LIB_DIR}/libot-cli-rw612.a
    ${OT_LIB_DIR}/libopenthread-url.a
    ${OT_LIB_DIR}/libopenthread-radio-spinel.a
    ${OT_LIB_DIR}/libtcplp-ftd.a
)

TARGET_LINK_LIBRARIES(${MCUX_SDK_PROJECT_NAME} PRIVATE -Wl,--end-group)

endif()
