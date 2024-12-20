# ot-nxp config
mcux_set_variable(OT_SRC_DIR ${SdkRootDirPath}/middleware/wireless/coex/third_party/ot-nxp)
mcux_set_variable(OT_LIB_DIR ${SdkRootDirPath}/middleware/wireless/coex/third_party/ot-nxp/build_rw612/rw612_ot_cli/lib)

mcux_add_include(
    BASE_PATH ${OT_SRC_DIR}
    INCLUDES src/rw/rw612
             src/common
             openthread/src
             openthread/include
             openthread/src/core
             openthread/examples/platforms
             openthread/third_party/mbedtls
)

mcux_add_library(
    BASE_PATH ${OT_LIB_DIR}
    LIBS libopenthread-cli-ftd.a
         libopenthread-cli-ftd.a
         libopenthread-ftd.a
         libopenthread-hdlc.a
         libopenthread-ncp-ftd.a
         libopenthread-platform.a
         libopenthread-rw612.a
         libopenthread-spinel-ncp.a
         libot-cli-addons.a
         libot-cli-rw612.a
         libopenthread-url.a
         libopenthread-radio-spinel.a
         libtcplp-ftd.a
)