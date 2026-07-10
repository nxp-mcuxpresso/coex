## Overview

This document provides step-by-step procedures to build and test coex examples,
and also instructions for running the included sample applications.

### Hardware requirements

- Micro USB cable
- RD-RW612-BGA board
- Personal Computer

### Board settings

| PIN NAME     | RDRW612-BGA              |
| ------------ | ------------------------ |
| FC0_UART_TXD | HD2(pin 3) <---> HD3(TX) |
| FC0_UART_RXD | HD2(pin 2) <---> HD3(RX) |

**NOTE:**

1. For RDRW612-BGA/QFN A1&A2 board, need to confirm the following settings.
   - unload JP19, load JP9, JP23
   - Connect JP47 to GND.

     > _Connecting JP47 to GND only needs to be GND once after power-on, and then it can be removed_. (<font color=blue>**Recommendation:** always connect JP47 to ground.</font>)
     >
     > _Both A1&A2 JP47 and A0 JP30 are grounded to close U51._
     >
     > <font color=red>_The pins corresponding to U51 on different boards may not be JP47 or JP30, please check._</font>
     >
2. WiFi and BLE use `FC3` UART and OT uses `FC0` UART.


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
# NOTE: this wpa supplicant ot lib only used to compile with -DCOEX_ENABLE_WIFI=ON and -DCONFIG_WPA_SUPPLICANT=ON.
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
$ west build -b rdrw612bga examples/coex_examples/coex_wifi_edgefast --toolchain armgcc --config=flash_debug -d coex_wifi_edgefast
```

> flash_release:
```bash
$ cd <sdk root>
$ west build -b rdrw612bga examples/coex_examples/coex_wifi_edgefast --toolchain armgcc --config=flash_release -d coex_wifi_edgefast
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

```bash
# CMD to write CPU1 wifi image to flash in J-link window:
J-Link> loadbin C:\xxx\rw61x_sb_wifi_a2.bin,0x08400000
# CMD to write CPU2 ble to flash in J-link window:
J-Link> loadbin C:\xxx\rw61x_sb_ble_a2.bin,0x08540000
# CMD to write CPU2 combo to flash in J-link window:
J-Link> loadbin C:\xxx\rw61x_sb_ble_15d4_combo_a2.bin,0x085e0000
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

The log below shows the output of the coex examples (based on edgefast_open shell) in the terminal window:

```bash
========================================

SHELL build:         Coex APP
========================================
========================================
     Initialize RW612 Module
========================================
Jul  9 2026
Copyright  2020  NXP
uart:~$ ========================================
[FW Download] Start to download 1 firmware: 25
[FW Download] 1 FW is active: 3042
STA MAC Address: C0:95:DA:01:24:7B
supplicant_main_task: 636 Starting wpa_supplicant thread with debug level: 3

Successfully initialized wpa_supplicant
iface_cb: iface ml1 ifindex 2 c0:95:da:01:24:7b
Using interface ml1

Initializing interface 0: ml1

WPS: Converting push_button to virtual_push_button for WPS 2.0 compliance
board_type: 2, board_type mapping:
0----QFN
1----CSP
2----BGA
ml1: CTRL-EVENT-REGDOM-CHANGE init=USER type=COUNTRY alpha2=WW
board_type: 2, board_type mapping:
0----QFN
1----CSP
2----BGA
========================================
app_cb: WLAN: received event 12
========================================
app_cb: WLAN initialized
========================================
WLAN CLIs are initialized
========================================
ENHANCED WLAN CLIs are initialized
========================================
CLIs Available:
========================================

wlan-version
wlan-mac
wlan-thread-info
wlan-net-stats
wlan-set-mac <MAC_Address>
wlan-scan
wlan-scan-opt ssid <ssid> bssid ...
wlan-add <profile_name> ssid <ssid> bssid...
wlan-remove <profile_name>
wlan-list
wlan-connect <profile_name>
wlan-connect-opt <profile_name> ...
wlan-reassociate
wlan-start-network <profile_name>
wlan-stop-network
wlan-disconnect
wlan-stat
wlan-info
wlan-address
wlan-uap-disconnect-sta <mac address>
wlan-get-uap-channel
wlan-get-uap-sta-list
wlan-uap-set-beacon-interval
wlan-uap-get-beacon-interval
wlan-ieee-ps <0/1>
wlan-set-ps-cfg <null_pkt_interval>
wlan-deep-sleep-ps <0/1>
wlan-get-beacon-interval
wlan-get-ps-cfg
wlan-set-max-clients-count <max clients count>
wlan-get-max-clients-count
wlan-rts <sta/uap> <rts threshold>
wlan-frag <sta/uap> <fragment threshold>
wlan-host-11k-enable <0/1>
wlan-host-11k-neighbor-req [ssid <ssid>]
wlan-host-11v-bss-trans-query <0..16>
wlan-mbo-nonprefer-ch "<oper_class>:<chan>:<preference>:<reason> <oper_class>:<chan>:<preference>:<reason>"
wlan-mbo-set-cell-capa <cell capa: 1/2/3(default)>
wlan-mbo-set-oce <oce: 1(default)/2>
wlan-set-okc <okc: 0(default)/1>
wlan-pmksa-list
wlan-pmksa-flush
wlan-set-scan-interval <scan_int: in seconds>
wlan-sta-filter  <filter mode> [<mac address list>]
wlan-get-log <sta/uap> <ext>
wlan-get-diag
wlan-roaming <bitmap> [<rssi_threshold>] [<snr_threshold>]
wlan-multi-mef <ping/arp/multicast/del> [<action>]
wlan-wakeup-condition <mef/wowlan wake_up_conds>
wlan-add-packet-filter  sta/uap 0/1 <patterns number> <ptn_len> <pkt_offset> <ptn> ...........
wlan-send-hostcmd
wlan-ext-coex-uwb
wlan-set-uap-hidden-ssid <0/1/2>
wlan-eu-crypto-rc4 <EncDec>
wlan-eu-crypto-aes-wrap <EncDec>
wlan-eu-crypto-aes-ecb <EncDec>
wlan-eu-crypto-ccmp-128 <EncDec>
wlan-eu-crypto-ccmp-256 <EncDec>
wlan-eu-crypto-gcmp-128 <EncDec>
wlan-eu-crypto-gcmp-256 <EncDec>
wlan-ft-roam <bssid> <channel>
wlan-set-antcfg <ant_mode> <evaluate_time> <evaluate_mode>
wlan-get-antcfg
wlan-scan-channel-gap <channel_gap_value>
wlan-wmm-stat <bss_type>
wlan-reset
wlan-set-regioncode <region-code>
wlan-get-regioncode
wlan-11d-enable <sta/uap> <0/1>
wlan-uap-set-ecsa-cfg <block_tx> <oper_class> <new_channel> <switch_count> <bandwidth>
wlan-csi-cfg
wlan-set-csi-param-header  <sta/uap> <csi_enable> <head_id> <tail_id> <chip_id> <band_config> <channel> <csi_monitor_enable> <ra4us>
wlan-set-csi-filter <opt> <macaddr> <pkt_type> <type> <flag>
wlan-txrx-histogram <action> <enable>
wlan-subscribe-event <action> <type> <value> <freq>
wlan-uapsd-enable <uapsd_enable>
wlan-uapsd-qosinfo <qos_info>
wlan-uapsd-sleep-period <sleep_period>
wlan-tx-ampdu-prot-mode <mode>
wlan-rx-abort-cfg
wlan-set-rx-abort-cfg-ext enable <enable> margin <margin> ceil <ceil_thresh> floor <floor_thresh>
wlan-get-rx-abort-cfg-ext
wlan-cck-desense-cfg
wlan-generate-wps-pin
wlan-start-wps-pbc
wlan-start-wps-pin <8 digit pin>
wlan-wps-cancel
wlan-start-ap-wps-pbc
wlan-start-ap-wps-pin <8 digit pin>
wlan-wps-ap-cancel
wlan-dpp-configurator-add
wlan-dpp-configurator-params  conf=<sta-dpp/ap-dpp> ssid=<ascii> configurator=<id>
wlan-dpp-mud-url https://...
wlan-dpp-bootstrap-gen type=<qrcode> chan=<op>/<ch> mac=<addr>
wlan-dpp-bootstrap-get-uri <bootstrap_gen id>
wlan-dpp-qr-code <DPP:...>
wlan-dpp-auth-init  peer=<id> role=<enrollee/configurator>
wlan-dpp-listen <frequency>...
wlan-dpp-stop-listen
wlan-dpp-pkex-add  own=<bootstrap_id> identifier=<string> code=<string>
wlan-dpp-chirp  own=<bootstrap id> listen=<freq>...
wlan-dpp-reconfig <network id> ...
wlan-dpp-configurator-sign  conf=<sta-dpp/ap-dpp> ssid=<ascii> configurator=<id>
wlan-net-monitor-cfg
wlan-set-monitor-filter <opt> <macaddr>
wlan-set-monitor-param <action> <monitor_activity> <filter_flags> <radio_type> <chan_number>
wlan-set-mgmtframetx-cfg
wlan-set-mgmtframetx-header <PktType> <PktSubType> <FromDS> <ToDS> <SeqNum> <FragNum> <Addr1> <Addr2> <Addr3> <Addr4>
wlan-set-mgmtframetx-payload <Data0 Data1 ... Datan>
wlan-set-tsp-cfg <enable> <backoff> <highThreshold> <lowThreshold> <dutycycstep> <dutycycmin> <highthrtemp> <lowthrtemp> <pollcnt>
wlan-get-tsp-cfg
wlan-get-signal
wlan-set-bandcfg
wlan-get-bandcfg
wlan-set-ips <option>
wlan-enable-disable-htc <option>
wlan-set-su <0/1>
wlan-set-forceRTS <0/1>
wlan-set-mmsf <enable> <Density> <MMSF>
wlan-get-mmsf
wlan-get-turbo-mode <STA/UAP>
wlan-set-turbo-mode <STA/UAP> <mode>
wlan-set-multiple-dtim <value>
wlan-set-country <country_code_str>
wlan-set-country-ie-ignore <0/1>
wlan-single-ant-duty-cycle <enable/disable> [<Ieee154Duration> <TotalDuration>]
wlan-dual-ant-duty-cycle <enable/disable> [<Ieee154Duration> <TotalDuration> <Ieee154FarRangeDuration>]
wlan-external-coex-pta enable <PTA/WCI-2/WCI-2 GPIO> ExtWifiBtArb <enable/disable> PolGrantPin <high/low> PriPtaInt <enable/disable> StateFromPta <state pin/ priority pin/ state input disable> SampTiming <Sample timing> InfoSampTiming <Sample timing> TrafficPrio <enable/disable> CoexHwIntWic <enable/disable>
wlan-sta-inactivityto <n> <m> <l> [k] [j]
wlan-get-temperature
wlan-auto-null-tx <sta/uap> <start/stop>
wlan-detect-ant <detect_mode> <ant_port_count> channel <channel> ...
wlan-get-channel-load <set/get> <duration>
wlan-set-network-chanlist <profile_name> [freq_band <band>] [channel <chan_list>]
wlan-get-txpwrlimit <subband>
wlan-set-chanlist
wlan-get-chanlist
wlan-set-txratecfg <sta/uap> <format> <index> <nss> <rate_setting> <autoTx_set>
wlan-get-txratecfg <sta/uap>
wlan-get-data-rate <sta/uap>
wlan-get-pmfcfg
wlan-uap-get-pmfcfg
wlan-set-ed-mac-mode <interface> <ed_ctrl_2g> <ed_offset_2g> <ed_ctrl_5g> <ed_offset_5g>
wlan-get-ed-mac-mode <interface>
wlan-set-tx-omi <interface> <tx-omi> <tx-option> <num_data_pkts>
wlan-set-toltime <value>
wlan-set-rutxpwrlimit
wlan-11ax-cfg <11ax_cfg>
wlan-11ax-bcast-twt <set/get>
wlan-11ax-twt-setup <dump/set/done> [<param_id> <param_data>]
wlan-11ax-twt-teardown <dump/set/done> [<param_id> <param_data>]
wlan-11ax-twt-report
wlan-11ax-twt-information <flow_identifier> <suspend_duration>
wlan-get-tsfinfo <format-type>
wlan-set-clocksync <mode> <role> <gpio_pin> <gpio_level> <pulse width>
wlan-set-rf-test-mode
wlan-set-rf-disable-11ax
wlan-unset-rf-test-mode
wlan-set-rf-tx-antenna <antenna>
wlan-get-rf-tx-antenna
wlan-set-rf-rx-antenna <antenna>
wlan-get-rf-rx-antenna
wlan-set-rf-xtal <xtal_cal>
wlan-get-rf-xtal
wlan-set-rf-bandwidth <bandwidth>
wlan-get-rf-bandwidth
wlan-set-rf-channel <channel>
wlan-get-rf-channel
wlan-set-rf-radio-mode <radio_mode>
wlan-get-rf-radio-mode
wlan-set-rf-tx-power <tx_power> <modulation> <path_id>
wlan-set-rf-tx-cont-mode <enable_tx> <cw_mode> <payload_pattern> <cs_mode> <act_sub_ch> <tx_rate>
wlan-set-rf-tx-frame <start> <data_rate> <frame_pattern> <frame_len> <adjust_burst_sifs> <burst_sifs_in_us> <short_preamble> <act_sub_ch> <short_gi> <adv_coding> <tx_bf> <gf_mode> <stbc> <bssid> <signal_bw> <NumPkt> <MaxPE> <BeamChange> <Dcm> <Doppler> <MidP> <QNum>
wlan-set-rf-trigger-frame-cfg <Enable_tx> <Standalone_hetb> <FRAME_CTRL_TYPE> <FRAME_CTRL_SUBTYPE> <FRAME_DURATION><TriggerType> <UlLen> <MoreTF> <CSRequired> <UlBw> <LTFType> <LTFMode><LTFSymbol> <UlSTBC> <LdpcESS> <ApTxPwr> <PreFecPadFct> <PeDisambig> <SpatialReuse><Doppler> <HeSig2> <AID12> <RUAllocReg> <RUAlloc> <UlCodingType> <UlMCS> <UlDCM><SSAlloc> <UlTargetRSSI> <MPDU_MU_SF> <TID_AL> <AC_PL> <Pref_AC>
wlan-set-rf-he-tb-tx <enable> <qnum> <aid> <axq_mu_timer> <tx_power>
wlan-get-and-reset-rf-per
wlan-set-rf-otp-mac-addr <mac_addr>
wlan-get-rf-otp-mac-addr
wlan-set-rf-otp-cal-data
wlan-get-rf-otp-cal-data
wlan-set-rf-rx-mac-filter
ping [-s <packet_size>] [-c <packet_count>] [-W <timeout in sec>] <ipv4/ipv6 address>
iperf [-s|-c <host>|-a|-h] [options]
dhcp-stat
wlan-set-rtc-time <year> <month> <day> <hour> <minute> <second>
wlan-get-rtc-time
========================================
========================================
app_cb: WLAN: received event 16
========================================
app_cb: WLAN: PS_ENTER
========================================
app_cb: WLAN: received event 16
========================================
app_cb: WLAN: PS_ENTER
[FW Download] Start to download 3 firmware: 5569
[FW Download] 3 FW is active: 7220
host init done

uart:~$
```

1. WiFi Test

> NOTE: All wifi commands require adding `wifi ` prefix.

- Get the Wi-Fi driver and firmware version:

```bash
uart:~$ wifi wlan-version
WLAN Driver Version   : v1.3.r48.p10
uart:~$ WLAN Firmware Version : rw610w-V2, IMU, FP99, 18.99.6.p6, PVE_FIX 1
Command wlan-version
```

- Get MAC Address:

```bash
uart:~$ wifi wlan-mac
MAC address
uart:~$ STA MAC Address: 00:50:43:02:98:23
uAP MAC Address: 00:50:43:02:99:23
Command wlan-mac
```

- Scan the network:

```bash
uart:~$ wifi wlan-scan
Scan scheduled...
uart:~$ Command wlan-scan
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

- BLE scan devices (the BLE host must initialized before):

```bash
uart:~$ bt init
uart:~$ Bluetooth initialized
Settings Loaded

uart:~$ bt scan on
Bluetooth active scan enabled
uart:~$ [DEVICE]: 44:6D:F5:85:DC:5F (random), AD evt type 0, RSSI -64  C:1 S:1 D:0 SR:0 E:0 Prim: LE 1M, Secn: No packets, Interval: 0x0000 (0 ms), SID: 0xff
[DEVICE]: 44:6D:F5:85:DC:5F (random), AD evt type 4, RSSI -63  C:0 S:1 D:0 SR:1 E:0 Prim: LE 1M, Secn: No packets, Interval: 0x0000 (0 ms), SID: 0xff
[DEVICE]: 6D:B3:D3:8E:ED:A2 (random), AD evt type 0, RSSI -77  C:1 S:1 D:0 SR:0 E:0 Prim: LE 1M, Secn: No packets, Interval: 0x0000 (0 ms), SID: 0xff
[DEVICE]: 6D:B3:D3:8E:ED:A2 (random), AD evt type 4, RSSI -76  C:0 S:1 D:0 SR:1 E:0 Prim: LE 1M, Secn: No packets, Interval: 0x0000 (0 ms), SID: 0xff
[DEVICE]: 3F:FB:95:F7:F9:14 (random), AD evt type 3, RSSI -75  C:0 S:0 D:0 SR:0 E:0 Prim: LE 1M, Secn: No packets, Interval: 0x0000 (0 ms), SID: 0xff
[DEVICE]: 49:A3:4E:86:63:0C (random), AD evt type 0, RSSI -76  C:1 S:1 D:0 SR:0 E:0 Prim: LE 1M, Secn: No packets, Interval: 0x0000 (0 ms), SID: 0xff
[DEVICE]: 49:A3:4E:86:63:0C (random), AD evt type 4, RSSI -75  C:0 S:1 D:0 SR:1 E:0 Prim: LE 1M, Secn: No packets, Interval: 0x0000 (0 ms), SID: 0xff
[DEVICE]: 5C:28:50:F9:DD:57 (random), AD evt type 0, RSSI -82  C:1 S:1 D:0 SR:0 E:0 Prim: LE 1M, Secn: No packets, Interval: 0x0000 (0 ms), SID: 0xff
[DEVICE]: 4A:7D:B4:12:7B:7A (random), AD evt type 0, RSSI -82  C:1 S:1 D:0 SR:0 E:0 Prim: LE 1M, Secn: No packets, Interval: 0x0000 (0 ms), SID: 0xff
[DEVICE]: 4A:7D:B4:12:7B:7A (random), AD evt type 4, RSSI -82  C:0 S:1 D:0 SR:1 E:0 Prim: LE 1M, Secn: No packets, Interval: 0x0000 (0 ms), SID: 0xff
[DEVICE]: 5A:54:C8:99:13:4A (random), AD evt type 0, RSSI -76  C:1 S:1 D:0 SR:0 E:0 Prim: LE 1M, Secn: No packets, Interval: 0x0000 (0 ms), SID: 0xff
[DEVICE]: 3B:95:00:4D:F3:EB (random), AD evt type 3, RSSI -82  C:0 S:0 D:0 SR:0 E:0 Prim: LE 1M, Secn: No packets, Interval: 0x0000 (0 ms), SID: 0xff
[DEVICE]: 47:9D:D0:CB:5F:0D (random), AD evt type 0, RSSI -86  C:1 S:1 D:0 SR:0 E:0 Prim: LE 1M, Secn: No packets, Interval: 0x0000 (0 ms), SID: 0xff
uart:~$ bt scan off
Scan successfully stopped
uart:~$
```

- BLE advertise (the BLE host must initialized before):

```bash
uart:~$ bt init
uart:~$ Bluetooth initialized
uart:~$ bt advertise on
Advertising started
uart:~$ bt advertise off
Advertising stopped
```

- BLE connect (the BLE host must initialized before):

```bash
uart:~$ bt init
uart:~$ Bluetooth initialized
uart:~$ bt connect C0:95:63:23:55:87 random
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
2. start scan, press "bt scan on", Bluetooth device around your current bluetooth will be list, for example,
[DEVICE]: 72:78:C1:B5:0F:DA (random), AD evt type 4, RSSI -32 BLE Peripheral C:0 S:1 D:0 SR:1 E:0 Prim: LE 1M, Secn: No packets, Interval: 0x0000 (0 ms), SID: 0xff
[DEVICE]: C4:0D:02:55:5E:AD (random), AD evt type 0, RSSI -83  C:1 S:1 D:0 SR:0 E:0 Prim: LE 1M, Secn: No packets, Interval: 0x0000 (0 ms), SID: 0xff
[DEVICE]: 66:8F:26:27:1F:52 (random), AD evt type 0, RSSI -82  C:1 S:1 D:0 SR:0 E:0 Prim: LE 1M, Secn: No packets, Interval: 0x0000 (0 ms), SID: 0xff
3. stop scan, press "bt scan off",
4. connect target device, press "bt connect <address: XX:XX:XX:XX:XX:XX> <type: (public|random)>", such as bt connect 72:78:C1:B5:0F:DA random
5. Send phy update command, press "bt phy-update <tx_phy> [rx_phy] [s2] [s8]", tx_phy/rx_phy could be 1(1M) or 2(2M) or 4(Coded).
   such as "bt phy-update 2 2".
6. The message "LE PHY updated: TX PHY LE 2M, RX PHY LE 2M" would be printed if the phy is updated. note, if peer do not support phy update, then this message will not be printed.
```

- BLE Data Packet Length Extension update:

```bash
GATT peripheral role side,
1. Initialize the Host, press "bt init".
2. Advertising, press "bt advertise on".
3. After the connection is established.
4. Check current LE RX/TX maximum data length and time, press "bt info", as below, default RX/TX maximum data length is 27 and default RX/TX maxumum time is 328.
Type: LE, Role: slave, Id: 0
59:8F:3C:20:93:86 (random)
Remote address: 59:8F:3C:20:93:86 (random) (resolvable)
Local address: 80:D2:1D:E8:30:EC (public) (identity)
Remote on-air address: 59:8F:3C:20:93:86 (random) (resolvable)
Local on-air address: 7C:59:48:2E:A4:51 (random) (resolvable)
Interval: 0x0024 (45 ms)
Latency: 0x0000 (0 ms)
Supervision timeout: 0x0190 (4000 ms)
LE PHY: TX PHY LE 1M, RX PHY LE 1M
LE data len: TX (len: 27 time: 328) RX (len: 27 time: 328)
5. When LE data len is updated by the peer device, below information will be printed.
LE data len updated: TX (len: 27 time: 328) RX (len: 50 time: 512)
6. Update maximum tx data length, press "bt data-len-update <tx_max_len> [tx_max_time]", such as bt data-len-update 65, below information will be printed.
Calculated tx time: 632
59:8F:3C:20:93:86 (random)
data len update initiated.
LE data len updated: TX (len: 65 time: 632) RX (len: 50 time: 512)

GATT central role side,
1. Initialize the Host, press "bt init".
2. Start scan, press "bt scan on", Bluetooth device around your current bluetooth will be list, for example, 
[DEVICE]: 7C:59:48:2E:A4:51 (random), AD evt type 4, RSSI -44 BLE Peripheral C:0 S:1 D:0 SR:1 E:0 Prim: LE 1M, Secn: No packets, Interval: 0x0000 (0 ms), SID: 0xff
3. Stop scan, press "bt scan off",
4. Connect target device, press "bt connect <address: XX:XX:XX:XX:XX:XX> <type: (public|random)>", such as bt connect 7C:59:48:2E:A4:51 random
5. Check current LE RX/TX maximum data length and time, press "bt info", as below, default RX/TX maximum data length is 27 and default RX/TX maxumum time is 328.
Type: LE, Role: master, Id: 0
7C:59:48:2E:A4:51 (random)
Remote address: 7C:59:48:2E:A4:51 (random) (resolvable)
Local address: C0:95:DA:00:BC:82 (public) (identity)
Remote on-air address: 7C:59:48:2E:A4:51 (random) (resolvable)
Local on-air address: 59:8F:3C:20:93:86 (random) (resolvable)
Interval: 0x0024 (45 ms)
Latency: 0x0000 (0 ms)
Supervision timeout: 0x0190 (4000 ms)
LE PHY: TX PHY LE 1M, RX PHY LE 1M
LE data len: TX (len: 27 time: 328) RX (len: 27 time: 328)
6. Update maximum tx data length, press "bt data-len-update <tx_max_len> [tx_max_time]", such as bt data-len-update 50, below information will be printed.
Calculated tx time: 512
7C:59:48:2E:A4:51 (random)
data len update initiated.
LE data len updated: TX (len: 50 time: 512) RX (len: 27 time: 328)
7. When LE data len is updated by the peer device, below information will be printed.
LE data len updated: TX (len: 50 time: 512) RX (len: 65 time: 632)
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
   add new service "gatt.register".

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
   perform the GATT data signing sequence, press "gatt.signed-write <handle> <data> [length] [repeat]",
   such as "gatt.signed-write 22 AA 1"
```

- BLE GATT Service Changed Indication:

```bash
GATT peripheral role side,
1. Initialize the Host, press "bt init",
2. Advertising, press "bt advertise on",
3. After the connection is established. and waiting for the service changed indication is subsribed,
4. Add new service, press "gatt.register",
5. Remove the added service, press "gatt.unregister".

GATT central role side,
1. Initialize the Host, press "bt init",
2. Scanning advertising packets, press "bt scan on",
3. A few seconds later, stop the scanning, press "bt scan off"
4. Select the target board and create a new connection. If the target is not listed, repeat steps 2 and 3.
   Then press "bt connect <address: XX:XX:XX:XX:XX:XX> <type: (public|random)>"
5. After the connection is established, subscribe the GATT service changed indicator. press "bt subscribe <CCC handle> <value handle> [ind]",
   such as "gatt.subscribe f e ind".
```
