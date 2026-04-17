# MCUXpresso with OpenThread Library Integration Guide

> Complete guide for integrating OpenThread library into MCUXpresso IDE projects

## Table of Contents

- [Prerequisites](#prerequisites)
- [Step 1: Clone OpenThread Repository (Platform-Specific)](#step-1-clone-openthread-repository-platform-specific)
- [Step 2: Build OpenThread CLI Library](#step-2-build-openthread-cli-library)
- [Step 3: Update MCUXpresso Project XML Manifest](#step-3-update-mcuxpresso-project-xml-manifest)
- [Step 4: Enable OpenThread in Project Configuration](#step-4-enable-openthread-in-project-configuration)
- [Step 5: Import and Build in MCUXpresso IDE](#step-5-import-and-build-in-mcuxpresso-ide)
- [Platform-Specific Hardware Instructions](#platform-specific-hardware-instructions)
- [Quick Reference: Platform Differences](#quick-reference-platform-differences)

---

## Prerequisites

### Software Requirements

- **MCUXpresso IDE**
- **MCUXpresso SDK** for your target platform
- **Python 3.6+** (for automation scripts)
- **Git** (for cloning repositories)
- **ARM GCC Toolchain** (arm-none-eabi-gcc)

### Platform-Specific Setup

#### Windows Users

**Recommended: Use Git Bash** for a Unix-like command-line environment:
- Download Git for Windows: [https://git-scm.com/download/win](https://git-scm.com/download/win)
- Git Bash is included with installation
- Benefits: Use same commands as Linux, forward slashes in paths

**Enable Git Long Path Support** (run once):

```bash
git config --global core.longpaths true
```

#### Linux Users

No additional setup required. Use your standard terminal.

### Hardware Requirements

Refer to the [Platform-Specific Hardware Instructions](#platform-specific-hardware-instructions) section for your hardware platform (e.g., RW612, MCXW71).

---

## Step 1: Clone OpenThread Repository (Platform-Specific)

**Platform Difference**: SDK path location differs between Windows and Linux due to Windows' 260-character path limitation.

### SDK Path Overview

| Platform | SDK Location |
|----------|-------------|
| **Windows** | `mcuxpresso_support/sdk/repo/mcuxsdk` (parallel to ot-nxp) |
| **Linux** | `mcuxpresso_support/ot-nxp/third_party/nxp_matter_support/github_sdk/sdk_next/repo/mcuxsdk` (inside ot-nxp) |

### 1.1 Clone ot-nxp

Navigate to the mcuxpresso_support directory:

```bash
cd <path-to-sdk>/middleware/wireless/coex/third_party/mcuxpresso_support
```

Clone ot-nxp repository:

```bash
git clone https://github.com/NXP/ot-nxp.git -b <ot_github_release_tag>
cd ot-nxp
```

Initialize submodules:

```bash
git submodule update --init
```

### 1.2 Update NXP SDK

Run the SDK update script:

```bash
python third_party/nxp_matter_support/scripts/update_nxp_sdk.py --platform common
```

**Expected Result**:
- **Windows**: SDK downloaded to `mcuxpresso_support/sdk/` (same level as `ot-nxp/`)
- **Linux**: SDK downloaded to `ot-nxp/third_party/nxp_matter_support/github_sdk/sdk_next/repo/mcuxsdk/`

### 1.3 Troubleshooting: Manual SDK Update

If `update_nxp_sdk.py` fails, manually complete the SDK update:

**Windows**:
```bash
cd <path-to-mcuxpresso_support>/sdk/repo/mcuxsdk_manifest
west update
```

**Linux**:
```bash
cd <path-to-ot-nxp>/third_party/nxp_matter_support/github_sdk/sdk_next/repo/mcuxsdk_manifest
west update
```

### 1.4 Set Up Environment

**Windows (Git Bash)**:

```bash
cd <path-to-mcuxpresso_support>/ot-nxp

# Source environment setup (Git Bash supports source command)
source ../sdk/repo/mcuxsdk/mcux-env.sh

# Export SDK configuration
west mcuxsdk-export
```

**Linux**:

```bash
cd <path-to-ot-nxp>

# Source environment setup
source ./third_party/nxp_matter_support/github_sdk/sdk_next/repo/mcuxsdk/mcux-env.sh

# Export SDK configuration
west mcuxsdk-export
```

**Note for Windows CMD users**: If using Windows Command Prompt instead of Git Bash, use `mcux-env.cmd` instead of `source ./mcux-env.sh`.

---

## Step 2: Build OpenThread CLI Library

### 2.1 Set ARM GCC Toolchain Path

**Both Windows (Git Bash) and Linux**:
```bash
export ARMGCC_DIR=<path-to-armgcc>
```

**Windows CMD alternative**:
```cmd
set ARMGCC_DIR=<path-to-armgcc>
```

### 2.2 Build for Your Platform

Navigate to ot-nxp directory:

```bash
cd <path-to-mcuxpresso_support>/ot-nxp
```

**Example: Build for RW612 Platform**

```bash
./script/build_rw612 ot_cli -DOT_NXP_BUILD_APP_AS_LIB=ON -DBOARD_APP_UART_INSTANCE=0 -DOT_NXP_DISABLE_TCP=ON -DOT_NXP_LWIP_IPERF=ON -DOT_APP_CLI_FREERTOS_IPERF=ON -DOT_APP_BR_FREERTOS=OFF -DOT_NXP_ENABLE_WPA_SUPP_MBEDTLS=OFF -DCMAKE_BUILD_TYPE=Debug -DCOEX_RTOS_MAX_PRIO=<value_for_configMAX_PRIORITIES>
```

### 2.3 Troubleshooting: Windows Long Path Build Errors

**Symptom**: Build fails with "subcommand failed" or path-related errors on Windows.

**Solution**: Use virtual drive mapping to shorten paths:

```bash
# Create virtual drive mapping directly to mcuxpresso_support (Git Bash)
# Replace <package_name> with your actual SDK package name (e.g., board_FRDM-RW612)
cmd //c "subst W: <sdk_root>\\<package_name>\\middleware\\wireless\\coex\\third_party\\mcuxpresso_support"

# Verify mapping
cmd //c "subst"

# Navigate to shortened path
cd /w/ot-nxp

# Rebuild
./script/build_rw612 ot_cli <options>
```

**Important Notes about Virtual Drive W:**

**Temporary Drive**: The virtual drive `W:` is **NOT a permanent directory**. It is only a symbolic link that exists in the current Windows session.

- **Session-based**: The mapping only persists until you restart Windows
- **Not a real drive**: No files are copied; it's just a shortcut to the original location
- **Automatic removal**: The mapping disappears after system reboot

**To manually remove the virtual drive mapping**:

```bash
# Delete the virtual drive mapping
cmd //c "subst W: /D"

# Verify it's removed
cmd //c "subst"
```

**Safe to Remove**: Deleting the virtual drive mapping **does NOT affect your original files**. The mapping is only a symbolic link (shortcut) to the original location. All your files remain safely in the original directory.

**Why this works**:
- Original path: `<sdk_root>\<package_name>\middleware\wireless\coex\third_party\mcuxpresso_support\ot-nxp\...` (very long)
- Mapped path: `W:\ot-nxp\...` (much shorter)
- This avoids Windows path length limitations during compilation

**Tip**: Replace `<sdk_root>` with your SDK installation directory and `<package_name>` with your package name. Keep the virtual drive mapped during your entire development session to avoid path issues.

### 2.5 Expected Build Output

After successful build in `build_<platform>/<platform>_ot_cli/`:

- **OpenThread Libraries**: `lib/*.a` files
- **Compilation Database**: `compile_commands.json` (required for Step 3)
- **Build System Files**: `build.ninja`, CMake configuration

---

## Step 3: Update MCUXpresso Project XML Manifest

### 3.1 Run the Automation Script

Navigate to mcuxpresso_support directory:

```bash
cd <path-to-sdk>/middleware/wireless/coex/third_party/mcuxpresso_support
```

Update XML manifest for your application:

```bash
python add_openthread_to_mcuxpresso.py --app <your_application_name>
```

**What this script does**:
1. Analyzes OpenThread header dependencies from `compile_commands.json`
2. Adds include paths to the XML manifest
3. Adds source groups with header files
4. Creates backup of original XML file
5. Generates detailed change report

### 3.2 Script Options

| Option | Description | Example |
|--------|-------------|----------|
| `--app <app_name>` | Specify application name (required) | `--app coex_wifi_edgefast` |
| `--dry-run` | Preview changes without modifying files | `--dry-run` |
| `--verbose` | Show detailed progress information | `--verbose` |

### 3.3 Best Practices

Always run dry-run first:

```bash
python add_openthread_to_mcuxpresso.py --app coex_wifi_edgefast --dry-run
```

**Output Files**:
- Updated XML: `boards/<board>/<example_type>/<app_name>/*.xml`
- Backup: `*.original_backup_<timestamp>.xml`
- Report: `XML_CHANGE_REPORT_<timestamp>.md`

### 3.4 Troubleshooting

To restore the original XML file:

```bash
# Find the backup
ls -lt *.original_backup_*.xml | head -1

# Restore
cp <backup_file> <original_xml_file>
```

---

## Step 4: Enable OpenThread in Project Configuration

Navigate to your application directory:

```bash
cd <path-to-sdk>/boards/<board>/coex_examples/<app_name>
```

Edit `app_mcuxpresso_config.h`:

Set `CONFIG_OT` to 1:

```c
#define CONFIG_OT       1  // Enable OpenThread
```

---

## Step 5: Import and Build in MCUXpresso IDE

### 5.1 Install SDK into MCUXpresso IDE

1. Open MCUXpresso IDE
2. Navigate to the **Installed SDKs** tab (bottom panel)
3. Drag-and-drop your SDK archive or SDK folder into the **Installed SDKs** panel
4. MCUXpresso IDE will install/register the SDK

### 5.2 Import Project from Installed SDK

1. Click "**Import SDK example(s)...**" in the Quickstart Panel (bottom left)
2. Select your installed SDK from the dropdown list
3. Navigate through the project tree:
   - Expand `<board>` (e.g., `frdmrw612`)
   - Expand `<example_type>` (e.g., `coex_examples`)
   - Select `<app_name>` (e.g., `coex_wifi_edgefast`)
4. Click **Next**, then **Finish**

### 5.3 Build Project

1. Right-click on the imported project in Project Explorer
2. Select "Build Configurations" → "Set Active" → "Debug" or "Release"
3. Click "Build Project" or press Ctrl+B
4. Verify that OpenThread library is linked correctly

**Expected Build Output**:
- Binary file: `<build-config>/<app_name>.axf`
- Map file with OpenThread symbols
- No linking errors related to OpenThread library

---

## Platform-Specific Hardware Instructions

### RW612 Platform

#### Hardware Setup

- **Board**: FRDM-RW612
- **USB Connection**: Type-C cable to J7 (MCU-Link port)
- **Serial Settings**:
  - Baud rate: 115200
  - Data bits: 8
  - Parity: None
  - Stop bits: 1
  - Flow control: None

#### Flash and Verify

**Flash the Binary**:

```bash
J-Link> loadbin <path>/frdmrw612_coex_wifi_edgefast.bin, 0x08000000
```

**Verify coex_wifi_edgefast Example**:

After flashing, open a serial terminal (115200 8N1) and verify:

1. **Check Initial Output**:

```
        Coex APP

SHELL build: ========================================
========================================
     Initialize RW612 Module
========================================
Copyright  2020  NXP
========================================

WiFi shell initialization
========================================
BLE shell initialization
@bt> ========================================

@Coex>
```

2. **Verify WiFi Functionality**:

```bash
@Coex> wifi.wlan-version
WLAN Driver Version   : v1.3.r48.p10
WLAN Firmware Version : rw610w-V2, IMU, FP99, 18.99.6.p6, PVE_FIX 1

@Coex> wifi.wlan-scan
Scan scheduled...
2 networks found:
1C:6A:7A:87:FF:BF  "NXP" Infra
        channel: 161
        rssi: -74 dBm
        security: WPA2
```

3. **Verify BLE Functionality**:

```bash
@Coex> bt.init
Bluetooth initialized
Settings Loaded

@Coex> bt.scan on
Bluetooth active scan enabled
[DEVICE]: 44:6D:F5:85:DC:5F (random), RSSI -64
```

4. **Verify OpenThread** (if CONFIG_OT=1):

**Note**: OpenThread CLI commands are input via the UART port specified by `BOARD_APP_UART_INSTANCE` during OT library build.

```bash
> ot ifconfig up
Done
> ot thread start
Done
> ot state
leader
Done
```

---

## Quick Reference: Platform Differences

### SDK Path and Environment

| Aspect | Windows | Linux |
|--------|---------|-------|
| **SDK Location** | `mcuxpresso_support/sdk/repo/mcuxsdk` | `ot-nxp/third_party/.../sdk_next/repo/mcuxsdk` |
| **SDK Relative to ot-nxp** | Same level (parallel) | Inside ot-nxp |
| **Reason** | Avoid 260-char path limit | No path restrictions |
| **Git Long Path Config** | Required | Not needed |
| **Environment Setup Script** | `mcux-env.cmd` | `mcux-env.sh` |
| **Execute Environment** | `cmd.exe //c mcux-env.cmd` | `source mcux-env.sh` |

### Directory Structure Comparison

**Windows**:
```
mcuxpresso_support/
├── add_openthread_to_mcuxpresso.py
├── README.md
├── ot-nxp/                    ← Cloned from GitHub
│   ├── build_rw612/
│   ├── script/build_rw612
│   └── third_party/nxp_matter_support/
└── sdk/                       ← SDK (Windows only!)
    └── repo/mcuxsdk/          ← Actual SDK root
```

**Linux**:
```
mcuxpresso_support/
├── add_openthread_to_mcuxpresso.py
├── README.md
└── ot-nxp/                              ← Cloned from GitHub
    ├── build_rw612/
    ├── script/build_rw612
    └── third_party/nxp_matter_support/
        └── github_sdk/sdk_next/
            └── repo/mcuxsdk/            ← SDK (Linux)
```
---

**Important Notes**:

1. The `ot-nxp` directory is NOT included in the SDK package. Users must clone it manually from GitHub.
2. When using Git Bash on Windows, most Linux commands work without modification.
3. The virtual drive mapping trick (`subst`) is only needed on Windows for very deep directory structures.

---
