## Overview

This document provides step-by-step procedures to build and test coex examples,
and also instructions for running the included sample applications.

> **FRDM-RW610FML NOTE (flash geometry):**
> This board carries the RW610 die together with an on-module SPI NOR flash
> **XM25QH64C (XMC, 8 MB / 64 Mbit)** instead of the 64 MB Winbond W25Q512JV
> used on FRDM-RW612. Because the total flash is only 8 MB, the connectivity
> firmware storage base was moved from `0x08400000` (4 MB offset, used on the
> 64 MB board) down to `0x08300000` (3 MB offset). The manual firmware download
> addresses in "4. Flash Binaries" below are updated accordingly. All storage
> regions (app text, firmware storage, factory/prod data, NV storage and
> LittleFS) are laid out relative to the flash size in
> `linker/RW61x_flash.ld` (`m_flash_size = 0x800000`) and
> `mcux_link/symbols.ldt`, so the C sources do not need any change.

### Hardware requirements

- Type-C cable
- FRDMRW610FML board
- Personal Computer

### Board settings

**NOTE:**
 WiFi and BLE use `FC3` UART and OT uses `FC0` UART.


## Build and flash

Prerequisites:
- CMake (version >=3.24)
- Ninja (version >=1.12)
- ARM GCC Toolchain (**only support ARM GCC**)
- Python3 (version >=3.6)

> **NOTE**: Make sure that the paths of all these tools are set into the Path system variable.

### 1. Downloading repo

i. Download SDK soruce code from NXP SDK github release and checkout to SDK release tag, or download SDK package from NXP SDK builder website. SDK Path: `<path-to-sdk>`

ii. Download the ot-nxp repo. (`<path-to-sdk>` and `<path-to-ot-nxp>` should use different `.west` workspace)

```bash
$ git clone https://github.com/NXP/ot-nxp.git -b <ot_github_release_tag>
$ cd <path-to-ot-nxp>
$ git submodule update --init
$ python third_party/nxp_matter_support/scripts/update_nxp_sdk.py --platform common
$ source ./third_party/nxp_matter_support/github_sdk/sdk_next/repo/mcuxsdk/mcux-env.sh
$ west mcuxsdk-export
```

iii: Create a soft link to the ot-nxp repo in the MCU SDK
```bash
#For SDK repo, 
cd <path-to-sdk>/middleware/wireless/coex/third_party/

# NOTE: <path-to-ot-nxp> must be absolute path.
ln -s <path-to-ot-nxp> ot-nxp
```

### 2. Build OT CLI library

<font color=red> **NOTE:**</font>
- `<value_for_configMAX_PRIORITIES>` should be set to the same as `configMAX_PRIORITIES-2`, which is `CONFIG_NUM_PREEMPT_PRIORITIES` in the coex_wifi_edgefast example's `FreeRTOSConfig.h` file (Path: `<path-to-sdk>/examples/_boards/\<board\>/coex_examples/coex_wifi_edgefast` OR `<path-to-sdk>/boards/\<board\>/coex_examples/coex_wifi_edgefast`).
- The minimum value of `<value_for_configMAX_PRIORITIES>` is 5.

```bash
# NOTE: Make sure the paths of arm-gcc tool are set to the Path system variable.
# For example,
# export ARMGCC_DIR='<path-to-armgcc>'

$ cd <path-to-ot-nxp>

# For embedded supplicant OT lib build:
$ ./script/build_rw612 ot_cli  -DOT_NXP_BUILD_APP_AS_LIB=ON -DBOARD_APP_UART_INSTANCE=0 -DOT_NXP_DISABLE_TCP=ON -DOT_NXP_LWIP_IPERF=ON -DOT_APP_CLI_FREERTOS_IPERF=ON -DOT_APP_BR_FREERTOS=OFF -DOT_NXP_ENABLE_WPA_SUPP_MBEDTLS=OFF -DCMAKE_BUILD_TYPE=Debug -DCOEX_RTOS_MAX_PRIO=<value_for_configMAX_PRIORITIES>
# For wpa supplicant OT lib build:
# Note: this wpa supplicant ot lib only used to compile with -DCOEX_ENABLE_WIFI=ON and -DCONFIG_WPA_SUPPLICANT=ON.
$ ./script/build_rw612 ot_cli  -DOT_NXP_BUILD_APP_AS_LIB=ON -DBOARD_APP_UART_INSTANCE=0 -DOT_NXP_DISABLE_TCP=ON -DOT_NXP_LWIP_IPERF=ON -DOT_APP_CLI_FREERTOS_IPERF=ON -DOT_APP_BR_FREERTOS=OFF -DOT_NXP_ENABLE_WPA_SUPP_MBEDTLS=ON -DCMAKE_BUILD_TYPE=Debug -DCOEX_RTOS_MAX_PRIO=<value_for_configMAX_PRIORITIES>
```

### 3. Building

Modify `examples/coex_examples/coex_wifi_edgefast/app_config.cmake` to generate different coexistence images.

| coexistence images | CONFIG_WIFI | CONFIG_BLE | CONFIG_OT | Simulation Case          |
| ------------------ | ---------------- | --------------- | -------------- | ------------------------ |
| WiFi + BLE         | 1               | 1              | 0             | Matter over WiFi         |
| WiFi + OT          | 1               | 0              | 1             | /                        |
| BLE  + OT          | 0               | 1              | 1             | Matter over Thread       |
| WiFi + BLE + OT    | 1               | 1              | 1             | Matter over WiFi + OT BR |

> NOTE: If building BLE+OT, the ot libs should set `DOT_NXP_ENABLE_WPA_SUPP_MBEDTLS` to `OFF`.

Macros related to Wi-Fi supplicant

|   Wi-Fi supplicant   | CONFIG_WPA_SUPPLICANT  |
| -------------------- | ------------------------ |
| embedded supplicant  | 0                        |
| wpa supplicant       | 1(default)               |

Enable Monolithic feature
> Modify `examples/${board}/coex_examples/coex_wifi_edgefast/app_config.h`.

| Component          | CONFIG_MONOLITHIC_WIFI | CONFIG_MONOLITHIC_BLE | CONFIG_MONOLITHIC_BLE_15_4 |
| ------------------ | ---------------------- | --------------------- | -------------------------- |
| Wi-Fi              | 1(default)             | NA                    | NA                         |
| BLE                | NA                     | 1(default)            | NA                         |
| OT                | NA                     | NA            | 1                         |

If want to disable Wi-Fi monolithic feature, define `CONFIG_MONOLITHIC_WIFI` to 0.
If want to disable BLE monolithic feature, define `CONFIG_MONOLITHIC_BLE` to 0.
If want to disable OT or combo monolithic feature, define `CONFIG_MONOLITHIC_BLE_15_4` to 0.

### Building coex examples with CMake


> flash_debug:
```bash
$ cd <sdk root>
$ west build -b frdmrw610fml examples/coex_examples/coex_wifi_edgefast --toolchain armgcc --config=flash_debug -d coex_wifi_edgefast
```

> flash_release:
```bash
$ cd <sdk root>
$ west build -b frdmrw610fml examples/coex_examples/coex_wifi_edgefast --toolchain armgcc --config=flash_release -d coex_wifi_edgefast
```

**NOTE:**

> 1. ```-d coex_wifi_edgefast``` -> Specify the generated project path. Can name it as needed.
> 2. Find coex_wifi_edgefast.elf/coex_wifi_edgefast.bin in coex_wifi_edgefast folder.
> 3. Only support armgcc to build coex application.
> 4. The link option "-Wl,-z -Wl,muldefs" for armgcc in <mcusdk>/arch/arm/configuration/wireless.cmake will be removed. Then multiple
definition will be treated as error. If build example with OT and have multiple definition errors, please add the link option back for
temporary fix.

### 4. Flash Binaries

Flash the image with the following command,

```bash
# CMD to write CPU3 coex app image to flash in J-link window:
J-Link> loadbin C:\xxx\coex_wifi_edgefast.bin, 0x08000000
```

**NOTE:**

Monolithic feature is default enabled, this means it's no need to flash binaries manually. 

If disable Wi-Fi or BLE or OT monolithic feature, download firmware manually. 

SB firmware path: `<sdk root>`/components/conn_fwloader/fw_bin

> **FRDM-RW610FML addresses:** The firmware storage base is `0x08300000` on the
> 8 MB module (vs `0x08400000` on the 64 MB FRDM-RW612). The addresses below are
> derived from `fw_storage_start = 0x08300000`, `wifi_fw_size = 0xA0000` and
> `ble_fw_size = 0x50000` in `mcux_link/symbols.ldt` / `linker/RW61x_flash.ld`.

```bash
# CMD to write CPU1 wifi image to flash in J-link window:
J-Link> loadbin C:\xxx\rw61x_sb_wifi_a2.bin,0x08300000
# CMD to write CPU2 ble to flash in J-link window:
J-Link> loadbin C:\xxx\rw61x_sb_ble_a2.bin,0x08440000
# CMD to write CPU2 combo to flash in J-link window:
J-Link> loadbin C:\xxx\rw61x_sb_ble_15d4_combo_a2.bin,0x084e0000
```
## Run

### Prepare the Demo

1. Connect a micro USB cable between the PC host and the MCU-Link USB port (J7) on the board.
2. Open a serial terminal with the following settings:
   - 115200 baud rate
   - 8 data bits
   - No parity
   - One stop bit
   - No flow control
3. Download the program to the target board.
4. Launch the debugger in your IDE to begin running the example.

### Running the example

The console output, WiFi/BLE test procedures and command reference are identical
to the FRDM-RW612 coex example. Refer to the FRDM-RW612 coex readme for the full
sample logs and the WiFi/BLE test command walkthrough; only the board name and
the flash geometry (8 MB, firmware base `0x08300000`) differ on FRDM-RW610FML.
