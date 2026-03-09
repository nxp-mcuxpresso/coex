# OpenThread Platform Configuration

This directory contains platform-specific OpenThread configurations for the coex middleware.

## Directory Structure

```
platform/
├── rw612/
│   └── ot_config.cmake      # RW612 OpenThread configuration
├── rt1060/                  # To be added for RT1060 support
│   └── ot_config.cmake
└── rt1170/                  # To be added for RT1170 support
    └── ot_config.cmake
```

## Architecture

The platform-specific configurations are automatically included based on the board name:

- `frdmrw612` or `rdrw612bga` → `platform/rw612/ot_config.cmake`
- `evkcmimxrt1060` → `platform/rt1060/ot_config.cmake` (future)
- `evkmimxrt1170` or `evkbmimxrt1170` → `platform/rt1170/ot_config.cmake` (future)

## Adding a New Platform

To add support for a new platform:

1. Create a new directory under `platform/` (e.g., `platform/rt1060/`)
2. Create `ot_config.cmake` in the new directory
3. Set platform-specific variables:
   - `OT_LIB_DIR`: Path to OpenThread libraries
   - Platform-specific include directories
   - Platform-specific libraries to link

### Example: `platform/rt1060/ot_config.cmake`

```cmake
# RT1060 OpenThread Platform Configuration
message(STATUS "Loading RT1060 OpenThread configuration")

# RT1060 specific library directory
set(OT_LIB_DIR ${OT_SRC_DIR}/build_rt1060/rt1060_ot_cli/lib)
message(STATUS "Build with RT1060 ot-cli libs, path=${OT_LIB_DIR}")

# RT1060 specific include directories
target_include_directories(${MCUX_SDK_PROJECT_NAME} PUBLIC
    ${OT_SRC_DIR}/src/rt/rt1060
)

# Link RT1060 OpenThread libraries
TARGET_LINK_LIBRARIES(${MCUX_SDK_PROJECT_NAME} PRIVATE -Wl,--start-group)

target_link_libraries(${MCUX_SDK_PROJECT_NAME} PRIVATE
    ${OT_LIB_DIR}/libopenthread-cli-ftd.a
    ${OT_LIB_DIR}/libopenthread-ftd.a
    # ... other RT1060-specific libraries ...
    ${OT_LIB_DIR}/libopenthread-rt1060.a
    ${OT_LIB_DIR}/libot-cli-rt1060.a
)

TARGET_LINK_LIBRARIES(${MCUX_SDK_PROJECT_NAME} PRIVATE -Wl,--end-group)
```

4. Update the platform detection logic in `third_party.cmake` if needed

## Manual Platform Override

You can manually override the platform detection by setting `COEX_PLATFORM` in your board's `reconfig.cmake`:

```cmake
# Force platform to rt1060
set(COEX_PLATFORM "rt1060")
```
