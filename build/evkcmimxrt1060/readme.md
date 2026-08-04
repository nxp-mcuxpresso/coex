## Overview

This document provides step-by-step procedures to build and test coex examples,
and also instructions for running the included sample applications.

### Hardware requirements

- Micro USB cable
- USB-to-UART converter (for OT CLI via LPUART8)
- evkcmimxrt1060 board
- Personal Computer
- One of the following modules:
  - Embedded Artists 2LL M.2 Module (EAR00500) - direct M2 connection
  - Embedded Artists 2EL M.2 Module (Rev-A1) - direct M2 connection

### Board settings

Before building the example application, define the Wi-Fi module in `_boards/evkcmimxrt1060/coex_examples/coex_wifi_edgefast/prj.conf`.

If you want to use Embedded Artists 2LL M.2 Module (EAR00500):
> `CONFIG_MCUX_COMPONENT_component.wifi_bt_module.IW61X=y`
> `CONFIG_MCUX_COMPONENT_component.wifi_bt_module.board_murata_2ll_m2=y`

If you want to use Embedded Artists 2EL M.2 Module (Rev-A1):
> `CONFIG_MCUX_COMPONENT_component.wifi_bt_module.IW61X=y`
> `CONFIG_MCUX_COMPONENT_component.wifi_bt_module.board_murata_2el_m2=y`

#### Jumper settings for RT1060-EVKC (enables external 5V supply):
remove  J40 5-6
connect J40 1-2
connect J45 with external power (controlled by SW6)

#### Murata Solution Board settings
Embedded Artists 2LL module datasheet: https://www.embeddedartists.com/wp-content/uploads/2024/12/2LL_M2_Datasheet.pdf
Embedded Artists 2EL module datasheet: https://www.embeddedartists.com/doc/ds/2EL_M2_Datasheet.pdf

The hardware should be reworked according to the hardware rework guide for evkcmimxrt1060 and Murata 1XK/1ZM/2EL/2LL M.2 Adapter in document Hardware Rework Guide for EdgeFast BT PAL.

**NOTE:**

1. To ensure that the LITTLEFS flash region has been cleaned, all flash sectors need to be erased before downloading example code.
2. After downloaded binary into qspiflash and boot from qspiflash directly, please reset the board by pressing SW7 or power off and on the board to run the application.
3. Nighthawk and Firecrest BT UART Rework (LPUART3):
   - Mount R93, R96
   - Remove R193
   - Connect J109, J76 2-3
4. WiFi and BLE use LPUART3. OT uses LPUART8, connected via J33 on the board:

| PIN NAME       | MCU PAD        | evkcmimxrt1060   |
| -------------- | -------------- | ---------------- |
| LPUART8_TX     | GPIO_AD_B1_10  | J33 pin 1        |
| LPUART8_RX     | GPIO_AD_B1_11  | J33 pin 2        |
| GND            | -              | J17 pin 7 (recommended) |


## Build and flash

Prerequisites:
- CMake (version >=3.24)
- Ninja (version >=1.12)
- ARM GCC Toolchain (**only support ARM GCC**)
- Python3 (version >=3.6)

> **NOTE**: Make sure that the paths of all these tools are set into the Path system variable.

### 1. Downloading repo

i. Download SDK source code from NXP SDK github release and checkout to SDK release tag, or download SDK package from NXP SDK builder website. SDK Path: `<path-to-sdk>`

ii. Download the ot-nxp repo. (`<path-to-sdk>` and `<path-to-ot-nxp>` should use different `.west` workspace)

```bash
$ git clone https://github.com/NXP/ot-nxp.git -b <ot_github_release_tag>
$ cd <path-to-ot-nxp>
$ git submodule update --init
$ python third_party/nxp_matter_support/scripts/update_nxp_sdk.py --platform common
$ source ./third_party/nxp_matter_support/github_sdk/sdk_next/repo/mcuxsdk/mcux-env.sh
$ west mcuxsdk-export
```

iii. Create a soft link to the ot-nxp repo in the MCU SDK
```bash
# For SDK repo:
cd <path-to-sdk>/middleware/wireless/coex/third_party/

# NOTE: <path-to-ot-nxp> must be absolute path.
ln -s <path-to-ot-nxp> ot-nxp
```

### 2. Build OT CLI library

<font color=red> **NOTE:**</font>
- `<value_for_configMAX_PRIORITIES>` should be set to `CONFIG_NUM_PREEMPT_PRIORITIES`, which equals `configMAX_PRIORITIES-2` defined in the coex_wifi_edgefast example's `FreeRTOSConfig.h` file (Path: `<path-to-sdk>/examples/_boards/evkcmimxrt1060/coex_examples/coex_wifi_edgefast` OR `<path-to-sdk>/boards/evkcmimxrt1060/coex_examples/coex_wifi_edgefast`). The default value is **12**.
- The minimum value of `<value_for_configMAX_PRIORITIES>` is 5.

```bash
# NOTE: Make sure the paths of arm-gcc tool are set to the Path system variable.
# For example,
# export ARMGCC_DIR='<path-to-armgcc>'

$ cd <path-to-ot-nxp>

# For embedded supplicant OT lib build:
$ ./script/build_rt1060 iw610_spi -DOT_NXP_BUILD_APP_AS_LIB=ON -DBOARD_APP_UART_INSTANCE=8 -DOT_NXP_DISABLE_TCP=ON -DOT_NXP_LWIP_IPERF=ON -DOT_APP_CLI_FREERTOS_IPERF=ON -DOT_APP_BR_FREERTOS=OFF -DOT_NXP_ENABLE_WPA_SUPP_MBEDTLS=OFF -DCMAKE_BUILD_TYPE=Debug -DCOEX_RTOS_MAX_PRIO=<value_for_configMAX_PRIORITIES>
# For wpa supplicant OT lib build:
# NOTE: this wpa supplicant ot lib is only used to compile with -DCOEX_ENABLE_WIFI=ON and -DCONFIG_WPA_SUPPLICANT=ON.
$ ./script/build_rt1060 iw610_spi -DOT_NXP_BUILD_APP_AS_LIB=ON -DBOARD_APP_UART_INSTANCE=8 -DOT_NXP_DISABLE_TCP=ON -DOT_NXP_LWIP_IPERF=ON -DOT_APP_CLI_FREERTOS_IPERF=ON -DOT_APP_BR_FREERTOS=OFF -DOT_NXP_ENABLE_WPA_SUPP_MBEDTLS=ON -DCMAKE_BUILD_TYPE=Debug -DCOEX_RTOS_MAX_PRIO=<value_for_configMAX_PRIORITIES>
```

The compiled static libraries will be output to `<path-to-ot-nxp>/build_rt1060/iw610_spi/lib/`.

### 3. Building

Modify `examples/coex_examples/coex_wifi_edgefast/app_config.cmake` to generate different coexistence images.

| coexistence images | CONFIG_WIFI | CONFIG_BLE | CONFIG_OT | Simulation Case          |
| ------------------ | ----------- | ---------- | --------- | ------------------------ |
| WiFi + BLE         | 1           | 1          | 0         | Matter over WiFi         |
| WiFi + OT          | 1           | 0          | 1         | /                        |
| BLE  + OT          | 0           | 1          | 1         | Matter over Thread       |
| WiFi + BLE + OT    | 1           | 1          | 1         | Matter over WiFi + OT BR |

> NOTE: If building BLE+OT, the ot libs should set `DOT_NXP_ENABLE_WPA_SUPP_MBEDTLS` to `OFF`.
> NOTE: When enabling OT (`CONFIG_OT=1`), the IW610 combo firmware must be used. Modify `_boards/evkcmimxrt1060/coex_examples/coex_wifi_edgefast/prj.conf`:
> ```
> #CONFIG_MCUX_COMPONENT_component.wifi_bt_module.bt_only_fw=y
> CONFIG_MCUX_COMPONENT_component.wifi_bt_module.nb_fw=y
> ```
> The default `bt_only_fw` does not include 802.15.4 support; `nb_fw` (combo firmware) is required for OT coexistence.

Macros related to Wi-Fi supplicant:

|   Wi-Fi supplicant   | CONFIG_WPA_SUPPLICANT  |
| -------------------- | ---------------------- |
| embedded supplicant  | 0                      |
| wpa supplicant       | 1 (default)            |

> **NOTE**: 2EL_M2 only supports embedded supplicant and WPA supplicant.

Enable Monolithic feature:
> Modify `examples/_boards/evkcmimxrt1060/coex_examples/coex_wifi_edgefast/app_config.h`.

| Component | CONFIG_MONOLITHIC_WIFI | CONFIG_MONOLITHIC_BLE | CONFIG_MONOLITHIC_BLE_15_4 |
| --------- | ---------------------- | --------------------- | -------------------------- |
| Wi-Fi     | 1 (default)            | NA                    | NA                         |
| BLE       | NA                     | 1 (default)           | NA                         |
| OT        | NA                     | NA                    | 1                          |

If want to disable Wi-Fi monolithic feature, define `CONFIG_MONOLITHIC_WIFI` to 0.
If want to disable BLE monolithic feature, define `CONFIG_MONOLITHIC_BLE` to 0.
If want to disable OT or combo monolithic feature, define `CONFIG_MONOLITHIC_BLE_15_4` to 0.

### Building coex examples with CMake


> flexspi_nor_debug:
```bash
$ cd <sdk root>
$ west build -b evkcmimxrt1060 examples/coex_examples/coex_wifi_edgefast --toolchain armgcc --config flexspi_nor_debug -d evkc1060_coex_wifi_edgefast
```

> flexspi_nor_release:
```bash
$ cd <sdk root>
$ west build -b evkcmimxrt1060 examples/coex_examples/coex_wifi_edgefast --toolchain armgcc --config flexspi_nor_release -d evkc1060_coex_wifi_edgefast
```

**NOTE:**

> 1. `-d evkc1060_coex_wifi_edgefast` -> Specify the generated project path. Can name it as needed.
> 2. Find coex_wifi_edgefast.elf/coex_wifi_edgefast.bin in evkc1060_coex_wifi_edgefast folder.
> 3. Only support armgcc to build coex application.
> 4. The link option `-Wl,-z -Wl,muldefs` for armgcc in `<mcusdk>/arch/arm/configuration/wireless.cmake` will be removed. Then multiple definition will be treated as error. If build example with OT and have multiple definition errors, please add the link option back for temporary fix.
> 5. CSI and NET_MONITOR are disabled by default due to RAM limitation. If to test CSI and NET_MONITOR, enable them and disable enterprise in wifi_config.h.
>    Modify `middleware/wireless/coex/src/configs/evkcmimxrt1060/wifi/wifi_config.h`:
>    Enable CSI and NET_MONITOR:
>    ```
>    #define CONFIG_CSI 1
>    #define CONFIG_NET_MONITOR 1
>    ```
>    Disable enterprise:
>    ```
>    #define CONFIG_WPA_SUPP_CRYPTO_ENTERPRISE 0
>    #define CONFIG_WPA_SUPP_CRYPTO_AP_ENTERPRISE 0
>    ```

### 4. Flash Binaries

Flash the image with the following command:

```bash
# CMD to write coex app image to flash in J-link window:
J-Link> loadbin C:\xxx\coex_wifi_edgefast.bin, 0x60000000
```

**NOTE:**

Monolithic feature is default enabled, which means it is not necessary to flash binaries manually.

If disable Wi-Fi or BLE or OT monolithic feature, download firmware manually.

SB firmware path: `<sdk root>`/components/conn_fwloader/fw_bin

## Run

### Prepare the Demo

1. Connect a micro USB cable between the PC host and the MCU-Link USB port (J7) on the board.
2. Connect a USB-to-UART converter between the PC host and the board's J33 header for the OT CLI:
   - Converter RX <---> J33 pin 1 (LPUART8_TX / GPIO_AD_B1_10)
   - Converter TX <---> J33 pin 2 (LPUART8_RX / GPIO_AD_B1_11)
   - Converter GND <---> J17 pin 7 (GND)
3. Open two serial terminals with the following settings:
   - 115200 baud rate
   - 8 data bits
   - No parity
   - One stop bit
   - No flow control
   - Terminal 1 (MCU-Link, debug/WiFi/BLE shell): connected to the MCU-Link COM port
   - Terminal 2 (USB-to-UART converter, OT CLI): connected to the converter COM port
4. Download the program to the target board.
5. Launch the debugger in your IDE to begin running the example.

### Running the example

The log below shows the output of the coex examples (based on edgefast-shell) in the terminal window:

```bash
SHELL build:         Coex APP
========================================
========================================
     Initialize Nighthawk-2LL (IW610) M2 Module
========================================
Feb 24 2025
Copyright  2020  NXP
========================================

WiFi shell initialization
========================================
BLE shell initialization
@bt> ========================================

@Coex>
```

1. WiFi Test

> NOTE: All wifi commands require adding `wifi.` prefix.

- Get the Wi-Fi driver and firmware version:

```bash
@Coex> wifi wlan-version
WLAN Driver Version   : v1.3.r48.p32
@Coex> WLAN Firmware Version : iw610w-V0, SDIO, FP99, 18.99.5.p51, PVE_FIX 1
Command wlan-version
```

- Get MAC Address:

```bash
@Coex> wifi wlan-mac
MAC address
@Coex> STA MAC Address: 00:50:43:02:98:23
uAP MAC Address: 00:50:43:02:99:23
Command wlan-mac
```

- Scan the network:

```bash
@Coex> wifi wlan-scan
Scan scheduled...
@Coex> Command wlan-scan
2 networks found:
1C:6A:7A:87:FF:BF  "NXP" Infra
        mode: 802.11AC
        channel: 161
        rssi: -74 dBm
        security: WPA2 Enterprise
        WMM: YES
        802.11K: YES
        802.11V: YES
        802.11W: NA
        WPS: NO
1C:6A:7A:87:FF:BE  "NXPOPEN" Infra
        mode: 802.11AC
        channel: 161
        rssi: -75 dBm
        security: WPA2
        WMM: YES
        802.11K: YES
        802.11V: YES
        802.11W: NA
        WPS: NO
```

2. BLE Test

> NOTE: Please use the command "help" to view the specific commands supported by the example.

- BLE scan devices (the BLE host must be initialized before):

```bash
@Coex> bt init
@Coex> Bluetooth initialized
Settings Loaded

@Coex> bt scan on
Bluetooth active scan enabled
@Coex> [DEVICE]: 44:6D:F5:85:DC:5F (random), AD evt type 0, RSSI -64  C:1 S:1 D:0 SR:0 E:0 Prim: LE 1M, Secn: No packets, Interval: 0x0000 (0 ms), SID: 0xff
[DEVICE]: 44:6D:F5:85:DC:5F (random), AD evt type 4, RSSI -63  C:0 S:1 D:0 SR:1 E:0 Prim: LE 1M, Secn: No packets, Interval: 0x0000 (0 ms), SID: 0xff
@Coex> bt scan off
Scan successfully stopped
@Coex>
```

- BLE advertise (the BLE host must be initialized before):

```bash
@Coex> bt init
@Coex> Bluetooth initialized
@Coex> bt advertise on
Advertising started
@Coex> bt advertise off
Advertising stopped
```

- BLE connect (the BLE host must be initialized before):

```bash
@Coex> bt init
@Coex> Bluetooth initialized
@Coex> bt connect C0:95:63:23:55:87 random
Connection pending
Connected: 7D:FD:FD:4D:FD:90 (random)
```

- BLE pairing and bonding:

```bash
GATT peripheral role side,
1. Initialize the Host, press "bt init",
2. Advertising, press "bt advertise on",
3. After the connection is established, perform the pairing sequence,
   it could be started from peripheral side by pressing "bt security <level>", such as "bt security 2".
4. If the bondable is unsupported by peripheral role, press "bt bondable off". Then start step 3.

GATT central role side,
1. Initialize the Host, press "bt init",
2. Scanning advertising packets, press "bt scan on",
3. A few seconds later, stop the scanning, press "bt scan off"
4. Select the target board and create a new connection. If the target is not listed, repeat steps 2 and 3.
   Then press "bt connect <address: XX:XX:XX:XX:XX:XX> <type: (public|random)>"
5. After the connection is established, perform the pairing sequence,
   it could be started from central side by pressing "bt security <level>", such as "bt security 2".
6. If the bondable is unsupported by central role, press "bt bondable off". Then start step 5.
```

- BLE 1M/2M/Coded PHY update:

```bash
GATT peripheral role side,
1. Initialize the Host, press "bt init",
2. Advertising, press "bt advertise on",
3. After the connection is established.
4. Send phy update command, press "bt phy-update <tx_phy> [rx_phy] [s2] [s8]", tx_phy/rx_phy could be 1(1M) or 2(2M) or 4(Coded).
   such as "bt phy-update 2 2".
5. The message "LE PHY updated: TX PHY LE 2M, RX PHY LE 2M" would be printed if the phy is updated. note, if peer do not support phy update, then this message will not be printed.

GATT central role side,
1. Initialize the Host, press "bt init",
2. start scan, press "bt scan on", Bluetooth device around your current bluetooth will be listed, for example,
[DEVICE]: 72:78:C1:B5:0F:DA (random), AD evt type 4, RSSI -32 BLE Peripheral C:0 S:1 D:0 SR:1 E:0 Prim: LE 1M, Secn: No packets, Interval: 0x0000 (0 ms), SID: 0xff
3. stop scan, press "bt scan off",
4. connect target device, press "bt connect <address: XX:XX:XX:XX:XX:XX> <type: (public|random)>", such as bt connect 72:78:C1:B5:0F:DA random
5. Send phy update command, press "bt phy-update <tx_phy> [rx_phy] [s2] [s8]", tx_phy/rx_phy could be 1(1M) or 2(2M) or 4(Coded).
   such as "bt phy-update 2 2".
6. The message "LE PHY updated: TX PHY LE 2M, RX PHY LE 2M" would be printed if the phy is updated. note, if peer do not support phy update, then this message will not be printed.
```

- BLE GATT data signing:

```bash
GATT peripheral role side,
1. Initialize the Host, press "bt init",
2. Advertising, press "bt advertise on",
3. After the connection is established, perform the pairing sequence,
   it could be started from peripheral side by pressing "bt security <level>", such as "bt security 2",
4. After the authentication is successfully, disconnect the connection,
   it could be started from peripheral side by pressing "bt disconnect",
5. Waiting for new connection. After the connection is established (LL encryption should be disabled),
   add new service "gatt register".

GATT central role side,
1. Initialize the Host, press "bt init",
2. Scanning advertising packets, press "bt scan on",
3. A few seconds later, stop the scanning, press "bt scan off"
4. Select the target board and create a new connection. If the target is not listed, repeat steps 2 and 3.
   Then press "bt connect <address: XX:XX:XX:XX:XX:XX> <type: (public|random)>"
5. After the connection is established, perform the pairing sequence,
   it could be started from central side by pressing "bt security <level>", such as "bt security 2",
6. After the authentication is successfully, disconnect the connection,
   it could be started from central side by pressing "bt disconnect",
7. Repeat the steps 2 and 3. After the connection is established (LL encryption should be disabled),
   perform the GATT data signing sequence, press "gatt signed-write <handle> <data> [length] [repeat]",
   such as "gatt signed-write 22 AA 1"
```

- BLE GATT Service Changed Indication:

```bash
GATT peripheral role side,
1. Initialize the Host, press "bt init",
2. Advertising, press "bt advertise on",
3. After the connection is established. and waiting for the service changed indication is subscribed,
4. Add new service, press "gatt register",
5. Remove the added service, press "gatt unregister".

GATT central role side,
1. Initialize the Host, press "bt init",
2. Scanning advertising packets, press "bt scan on",
3. A few seconds later, stop the scanning, press "bt scan off"
4. Select the target board and create a new connection. If the target is not listed, repeat steps 2 and 3.
   Then press "bt connect <address: XX:XX:XX:XX:XX:XX> <type: (public|random)>"
5. After the connection is established, subscribe the GATT service changed indicator. press "bt subscribe <CCC handle> <value handle> [ind]",
   such as "gatt subscribe f e ind".
```

3. OT Test

> NOTE: Use Terminal 2 (USB-to-UART converter, connected to J33) to interact with the OT CLI.

- Get the OT version:

```bash
> version
OPENTHREAD/thread-reference-20230706-1789-g8eb1b8aa8; NXP-RT1060; Aug  4 2026 14:00:00
Done
```

- Start Thread as a leader (standalone test):

```bash
> dataset init new
Done
> dataset commit active
Done
> ifconfig up
Done
> thread start
Done
> state
leader
Done
> ipaddr
fdde:ad00:beef:0:0:ff:fe00:fc00
fdde:ad00:beef:0:0:ff:fe00:7800
fdde:ad00:beef:0:e6e7:75f4:4b65:9084
fe80:0:0:0:18b5:6d7a:c5b3:99b8
Done
```

- ping test (from OT CLI):

```bash
> ping fdde:ad00:beef:0:0:ff:fe00:7800
16 bytes from fdde:ad00:beef:0:0:ff:fe00:7800: icmp_seq=1 hlim=64 time=8ms
Done
```
