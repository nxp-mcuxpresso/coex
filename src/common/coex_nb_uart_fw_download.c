/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Narrow band (BT/OT) CPU2 firmware download over UART for coex applications.
 *
 * Ported from middleware/edgefast_open/source/bluetooth/hci/hci_nxp_setup.c
 */

#include "fsl_common.h"
#include "fsl_adapter_uart.h"
#include "fsl_os_abstraction.h"
#include "edgefast_open_config.h"
#include "coex_nb_uart_fw_download.h"

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

#define DT_DRV_COMPAT nxp_bt_hci_uart

extern int bt_hci_uart_init(const struct device *dev, bool dma_mode, uint32_t speed, bool flow_control);
extern int bt_hci_uart_deinit(const struct device *dev);
extern UART_HANDLE_DEFINE(hci_uart_handle);

LOG_MODULE_REGISTER(coex_nb_uart_fw_dl, LOG_LEVEL_INF);

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define COEX_NB_DI         0x07U
#define COEX_NB_POLYNOMIAL 0x04c11db7UL
#define COEX_NB_CRC32_LEN  4

#define COEX_NB_FW_UPLOAD_CHANGE_TIMEOUT_RETRY_COUNT 6U

#define COEX_NB_CMD4 0x4U
#define COEX_NB_CMD6 0x6U
#define COEX_NB_CMD7 0x7U

#define COEX_NB_V1_HEADER_DATA_REQ  0xa5U
#define COEX_NB_V1_START_INDICATION 0xaaU
#define COEX_NB_V1_REQUEST_ACK      0x5aU

#define COEX_NB_V3_START_INDICATION 0xabU
#define COEX_NB_V3_HEADER_DATA_REQ  0xa7U
#define COEX_NB_V3_REQUEST_ACK      0x7aU
#define COEX_NB_V3_TIMEOUT_ACK      0x7bU
#define COEX_NB_V3_CRC_ERROR        0x7cU

#define COEX_NB_REQ_HEADER_LEN    1U
#define COEX_NB_A6REQ_PAYLOAD_LEN 8U
#define COEX_NB_AbREQ_PAYLOAD_LEN 3U

#define COEX_NB_CRC_ERR_BIT          BIT(0)
#define COEX_NB_NAK_REC_BIT          BIT(1)
#define COEX_NB_TIMEOUT_REC_ACK_BIT  BIT(2)
#define COEX_NB_TIMEOUT_REC_HEAD_BIT BIT(3)
#define COEX_NB_TIMEOUT_REC_DATA_BIT BIT(4)
#define COEX_NB_INVALID_CMD_REC_BIT  BIT(5)
#define COEX_NB_WIFI_MIC_FAIL_BIT    BIT(6)
#define COEX_NB_BT_MIC_FAIL_BIT      BIT(7)

#define COEX_NB_CMD_HDR_LEN 16

#define COEX_NB_SEND_BUFFER_MAX_LENGTH  4096
#define COEX_NB_RECV_RING_BUFFER_LENGTH 1024

/* CMD5 header: change bootloader baud rate */
static uint8_t coex_nb_cmd5_hdr_data[COEX_NB_CMD_HDR_LEN] = {0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                             0x2c, 0x00, 0x00, 0x00, 0x77, 0xdb, 0xfd, 0xe0};
/* CMD7 header: change bootloader timeout */
static uint8_t coex_nb_cmd7_hdr_data[COEX_NB_CMD_HDR_LEN] = {0x07, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00,
                                             0x00, 0x00, 0x00, 0x00, 0x5b, 0x88, 0xf8, 0xba};

enum coex_nb_uart_fw_version {
    COEX_NB_VER1 = 1,
    COEX_NB_VER2,
    COEX_NB_VER3,
};

struct coex_nb_change_speed_config {
    uint32_t clk_div_addr;
    uint32_t clk_div_val;
    uint32_t uart_clk_div_addr;
    uint32_t uart_clk_div_val;
    uint32_t mcr_addr;
    uint32_t mcr_val;
    uint32_t reinit_addr;
    uint32_t reinit_val;
    uint32_t icr_addr;
    uint32_t icr_val;
    uint32_t fcr_addr;
    uint32_t fcr_val;
};

struct coex_nb_uart_fw_upload_state {
    uint8_t version;
    uint8_t hdr_sig;

    uint8_t buffer[COEX_NB_A6REQ_PAYLOAD_LEN + COEX_NB_REQ_HEADER_LEN + 1];
    uint8_t send_buffer[COEX_NB_SEND_BUFFER_MAX_LENGTH];

    struct {
        uint8_t  buffer[COEX_NB_RECV_RING_BUFFER_LENGTH];
        uint32_t head;
        uint32_t tail;
        struct k_sem sem;
    } rx;

    uint16_t length;
    uint32_t offset;
    uint16_t error;
    uint8_t  crc8;

    uint32_t last_offset;
    uint8_t  change_speed_buffer[sizeof(struct coex_nb_change_speed_config) + COEX_NB_CRC32_LEN];

    uint32_t fw_length;
    uint32_t current_length;
    const uint8_t *fw;

    uint32_t cmd7_change_timeout_len;
    uint32_t change_speed_buffer_len;

    bool wait_hdr_sig;
    bool is_hdr_data;
    bool is_error_case;
    bool is_cmd7_req;
    bool is_entry_point_req;

    uint8_t last_5bytes_buffer[6];
};

/* UART device data */
struct coex_nb_uart_dev_data {
    uint32_t primary_speed;
    bool     primary_flowcontrol;
    uint32_t secondary_speed;
    bool     secondary_flowcontrol;
};

/*******************************************************************************
 * Static variables
 ******************************************************************************/

static const struct device *coex_nb_uart_dev = DEVICE_DT_GET(DT_INST_GPARENT(0));

static struct coex_nb_uart_fw_upload_state s_fw_upload;
static struct coex_nb_uart_dev_data        s_uart_dev_data;

static unsigned char  s_crc8_table[256U];
static unsigned long  s_crc_table[256U];
static bool           s_made_table;
static uint8_t        s_rx_buffer;
static bool           s_uart_initialized;

/*******************************************************************************
 * Forward declarations
 ******************************************************************************/
static void coex_nb_fw_upload_read_to_clear(void);
static int  coex_nb_fw_upload_read_data(uint8_t *buffer, uint32_t len);
static int  coex_nb_h4_starting_recv(uint8_t *buffer, uint16_t length);
static void coex_nb_hci_uart_transmit_cb(hal_uart_handle_t handle, hal_uart_status_t status, void *userData);

#if CONFIG_COEX_NB_BSP_TRIGGER
/* Interval (ms) to wait for BT header signature before sending BSP trigger */
#define COEX_NB_BSP_TRIGGER_WAIT_MS 500U

/*******************************************************************************
 * BSP trigger (boot-sleep-patch)
 ******************************************************************************/

/* Weak function: board-level implementation should override to toggle RTS and
 * wake BT from boot-sleep in coex (WiFi+BT) scenario with BSP enabled.
 */
__WEAK int coex_nb_bsp_trigger(void)
{
    return 0;
}

static int coex_nb_fw_upload_wake_bt_from_bootsleep(void)
{
    int err;
    int64_t end;
    uint8_t c;
    uint16_t wait_time = COEX_NB_BSP_TRIGGER_WAIT_MS;

    end = k_uptime_get() + (int64_t)(COEX_NB_BSP_TRIGGER_WAIT_MS * 10);

    while (k_uptime_get() < end) {
        wait_time = COEX_NB_BSP_TRIGGER_WAIT_MS + (uint16_t)(k_uptime_get() % 5 + 5);
        err = k_sem_take(&s_fw_upload.rx.sem, K_MSEC(wait_time));
        if (err < 0) {
            /* Timeout: BT CPU is in boot-sleep, send BSP trigger */
            goto send_bsp_trigger;
        }

        c = s_fw_upload.rx.buffer[s_fw_upload.rx.tail];
        s_fw_upload.rx.tail = (s_fw_upload.rx.tail + 1) % sizeof(s_fw_upload.rx.buffer);

        if ((c == COEX_NB_V1_HEADER_DATA_REQ) || (c == COEX_NB_V1_START_INDICATION) ||
            (c == COEX_NB_V3_START_INDICATION) || (c == COEX_NB_V3_HEADER_DATA_REQ)) {
            LOG_DBG("HDR SIG 0x%02X found", c);
            /* Put byte back for fw_uploading to consume */
            s_fw_upload.rx.tail = (s_fw_upload.rx.tail + sizeof(s_fw_upload.rx.buffer) - 1U) %
                                   sizeof(s_fw_upload.rx.buffer);
            k_sem_give(&s_fw_upload.rx.sem);
        }
    }

send_bsp_trigger:
    return coex_nb_bsp_trigger();
}
#endif /* CONFIG_COEX_NB_BSP_TRIGGER */

/*******************************************************************************
 * CRC helpers
 ******************************************************************************/

static void coex_nb_fw_upload_gen_crc32_table(void)
{
    int i, j;
    unsigned long crc_accum;

    for (i = 0; i < 256; i++) {
        crc_accum = ((unsigned long)i << 24);
        for (j = 0; j < 8; j++) {
            if (crc_accum & 0x80000000L)
                crc_accum = (crc_accum << 1) ^ COEX_NB_POLYNOMIAL;
            else
                crc_accum = (crc_accum << 1);
        }
        s_crc_table[i] = crc_accum;
    }
}

static void coex_nb_fw_upload_init_crc8(void)
{
    int i, j, crc;

    if (!s_made_table) {
        for (i = 0; i < 256; i++) {
            crc = i;
            for (j = 0; j < 8; j++)
                crc = (crc << 1) ^ ((crc & 0x80) ? COEX_NB_DI : 0);
            s_crc8_table[i] = (unsigned char)(crc & 0xFF);
        }
        s_made_table = true;
    }
}

static unsigned char coex_nb_fw_upload_crc8(unsigned char *array, unsigned char len)
{
    unsigned char crc_8 = 0xff;

    coex_nb_fw_upload_init_crc8();
    for (; len > 0; len--) {
        crc_8 = s_crc8_table[crc_8 ^ *array];
        array++;
    }
    return crc_8;
}

static unsigned long coex_nb_fw_upload_update_crc32(unsigned long crc_accum, char *data_blk_ptr,
                                            int data_blk_size)
{
    unsigned int i, j;

    for (j = 0; j < data_blk_size; j++) {
        i = ((unsigned int)(crc_accum >> 24) ^ *data_blk_ptr++) & 0xff;
        crc_accum = (crc_accum << 8) ^ s_crc_table[i];
    }
    return crc_accum;
}

/*******************************************************************************
 * UART helpers
 ******************************************************************************/

static int coex_nb_h4_uart_sending(const uint8_t *buffer, uint32_t len)
{
    hal_uart_status_t err = HAL_UartSendBlocking((hal_uart_handle_t)hci_uart_handle,
                                                  buffer, len);
    return (err == kStatus_HAL_UartSuccess) ? 0 : -EIO;
}

static void coex_nb_fw_upload_write_data(const uint8_t *buffer, uint32_t len)
{
    int err = coex_nb_h4_uart_sending(buffer, len);
    if (err != 0)
    {
        LOG_ERR("UART send failed: %d", err);
    }
}

static int coex_nb_h4_starting_recv(uint8_t *buffer, uint16_t length)
{
    hal_uart_status_t err = HAL_UartReceiveNonBlocking((hal_uart_handle_t)hci_uart_handle,
                                                        buffer, length);
    return (err == kStatus_HAL_UartSuccess) ? 0 : -EIO;
}

/* Same as hci_uart_transmit_cb() in hci_nxp_setup.c; the redundant count variable is omitted. */
static void coex_nb_hci_uart_transmit_cb(hal_uart_handle_t handle, hal_uart_status_t status, void *userData)
{
    if (status == (hal_uart_status_t)kStatus_HAL_UartRxIdle) {
        s_fw_upload.rx.buffer[s_fw_upload.rx.head] = s_rx_buffer;
        (void)coex_nb_h4_starting_recv(&s_rx_buffer, sizeof(s_rx_buffer));
        s_fw_upload.rx.head++;
        s_fw_upload.rx.head = s_fw_upload.rx.head % sizeof(s_fw_upload.rx.buffer);
        k_sem_give(&s_fw_upload.rx.sem);
    }
}

static int coex_nb_fw_upload_uart_reconfig(uint32_t speed, bool flow_control)
{
    hal_uart_status_t status;
    int err;

    /* Deinit first - skip on first call when handle not yet initialized. */
    if (s_uart_initialized)
    {
        bt_hci_uart_deinit(coex_nb_uart_dev);
    }

    coex_nb_fw_upload_read_to_clear();

    /* Init UART at requested speed, interrupt mode (dma_mode=false) */
    err = bt_hci_uart_init(coex_nb_uart_dev, false, speed, flow_control);
    if (err != 0) {
        return err;
    }
    s_uart_initialized = true;

    /* Install RX callback */
    status = HAL_UartInstallCallback((hal_uart_handle_t)hci_uart_handle,
                                      coex_nb_hci_uart_transmit_cb, NULL);
    if (status != kStatus_HAL_UartSuccess) {
        return -EIO;
    }

    (void)coex_nb_h4_starting_recv(&s_rx_buffer, sizeof(s_rx_buffer));

    return 0;
}

/*******************************************************************************
 * Protocol helpers (ported from hci_nxp_setup.c, s_ prefix for globals)
 ******************************************************************************/

static int coex_nb_fw_upload_read_data(uint8_t *buffer, uint32_t len)
{
    int err;

    while (len > 0) {
        err = k_sem_take(&s_fw_upload.rx.sem, K_MSEC(CONFIG_BT_H4_NXP_CTLR_WAIT_HDR_SIG_TIMEOUT));
        if (err < 0) {
            LOG_ERR("Fail to read data");
            return err;
        }
        *buffer = s_fw_upload.rx.buffer[s_fw_upload.rx.tail];
        buffer++;
        s_fw_upload.rx.tail++;
        s_fw_upload.rx.tail = s_fw_upload.rx.tail % sizeof(s_fw_upload.rx.buffer);
        len--;
    }
    return 0;
}

static void coex_nb_fw_upload_read_to_clear(void)
{
    uint32_t key = irq_lock();
    k_sem_reset(&s_fw_upload.rx.sem);
    s_fw_upload.rx.head = 0;
    s_fw_upload.rx.tail = 0;
    irq_unlock(key);
}

static int coex_nb_fw_upload_wait_for_hdr_sig(void)
{
    int err;
    int64_t end;
    char c;

    end = k_uptime_get() + CONFIG_BT_H4_NXP_CTLR_WAIT_HDR_SIG_TIMEOUT;
    s_fw_upload.hdr_sig = 0xFF;

    while (k_uptime_get() < end) {
        err = coex_nb_fw_upload_read_data((uint8_t *)&c, 1);
        if (err < 0) {
            k_msleep(1);
            continue;
        }
        if ((c == COEX_NB_V1_HEADER_DATA_REQ) || (c == COEX_NB_V1_START_INDICATION) ||
            (c == COEX_NB_V3_START_INDICATION) || (c == COEX_NB_V3_HEADER_DATA_REQ)) {
            LOG_DBG("HDR SIG found 0x%02X", c);
            s_fw_upload.hdr_sig = c;
            if (s_fw_upload.version == 0) {
                if ((c == COEX_NB_V3_START_INDICATION) || (c == COEX_NB_V3_HEADER_DATA_REQ))
                    s_fw_upload.version = COEX_NB_VER3;
                else
                    s_fw_upload.version = COEX_NB_VER1;
            }
            return 0;
        }
    }
    LOG_ERR("HDR SIG not found");
    return -EIO;
}

static int coex_nb_fw_upload_request_check_crc(uint8_t *buffer, uint8_t request)
{
    uint8_t crc;

    if (request == COEX_NB_V3_HEADER_DATA_REQ) {
        crc = coex_nb_fw_upload_crc8(buffer, COEX_NB_A6REQ_PAYLOAD_LEN + COEX_NB_REQ_HEADER_LEN);
        if (crc != buffer[COEX_NB_A6REQ_PAYLOAD_LEN + COEX_NB_REQ_HEADER_LEN]) {
            LOG_ERR("Request %d, CRC check failed", request);
            return -EINVAL;
        }
    } else if (request == COEX_NB_V3_START_INDICATION) {
        crc = coex_nb_fw_upload_crc8(buffer, COEX_NB_AbREQ_PAYLOAD_LEN + COEX_NB_REQ_HEADER_LEN);
        if (crc != buffer[COEX_NB_AbREQ_PAYLOAD_LEN + COEX_NB_REQ_HEADER_LEN]) {
            LOG_ERR("Request %d, CRC check failed", request);
            return -EINVAL;
        }
    } else {
        LOG_ERR("Invalid request %d", request);
    }
    return 0;
}

static void coex_nb_fw_upload_send_ack(uint8_t ack)
{
    if ((ack == COEX_NB_V3_REQUEST_ACK) || (ack == COEX_NB_V3_CRC_ERROR)) {
        s_fw_upload.buffer[0] = ack;
        s_fw_upload.buffer[1] = coex_nb_fw_upload_crc8(s_fw_upload.buffer, 1);
        coex_nb_fw_upload_write_data(s_fw_upload.buffer, 2);
        LOG_DBG("ACK = %x, CRC = %x", ack, s_fw_upload.buffer[1]);
    } else if (ack == COEX_NB_V3_TIMEOUT_ACK) {
        s_fw_upload.buffer[0] = ack;
        sys_put_le32(s_fw_upload.offset, &s_fw_upload.buffer[1]);
        s_fw_upload.buffer[5] = coex_nb_fw_upload_crc8(s_fw_upload.buffer, 5);
        coex_nb_fw_upload_write_data(s_fw_upload.buffer, 6);
        LOG_DBG("ACK = %x, CRC = %x", ack, s_fw_upload.buffer[5]);
    } else {
        LOG_ERR("Invalid ack");
    }
}

static int coex_nb_fw_upload_wait_req(bool secondary_speed)
{
    int err;
    uint32_t len;
    uint8_t buffer[10];

    buffer[0] = s_fw_upload.hdr_sig;
    if (s_fw_upload.hdr_sig == COEX_NB_V3_HEADER_DATA_REQ)
        len = COEX_NB_A6REQ_PAYLOAD_LEN + 1;
    else if (s_fw_upload.hdr_sig == COEX_NB_V3_START_INDICATION)
        len = COEX_NB_AbREQ_PAYLOAD_LEN + 1;
    else
        return -EINVAL;

    err = coex_nb_fw_upload_read_data(&buffer[1], len);
    if (err < 0) {
        LOG_ERR("Fail to read req");
        return err;
    }

    err = coex_nb_fw_upload_request_check_crc(buffer, s_fw_upload.hdr_sig);
    if (err != 0) {
        LOG_ERR("Fail to check CRC");
        coex_nb_fw_upload_send_ack(COEX_NB_V3_CRC_ERROR);
        return err;
    }

    if (s_fw_upload.hdr_sig == COEX_NB_V3_HEADER_DATA_REQ) {
        s_fw_upload.length = sys_get_le16(&buffer[1]);
        s_fw_upload.offset = sys_get_le32(&buffer[3]);
        s_fw_upload.error  = sys_get_le16(&buffer[7]);
        s_fw_upload.crc8   = buffer[9];
        LOG_DBG("Req: %hhd, %hd, %d, %hd, %hhd", s_fw_upload.hdr_sig, s_fw_upload.length,
                s_fw_upload.offset, s_fw_upload.error, s_fw_upload.crc8);
    } else if (s_fw_upload.hdr_sig == COEX_NB_V3_START_INDICATION) {
        __maybe_unused uint16_t chip_id;

        coex_nb_fw_upload_send_ack(COEX_NB_V3_REQUEST_ACK);
        chip_id = sys_get_le16(&buffer[1]);
        LOG_DBG("Indicate: %hhd, %hd, %hhd, %hhd", s_fw_upload.hdr_sig, chip_id, buffer[3],
                buffer[4]);

        if (!secondary_speed)
            return -EINVAL;
    }
    return 0;
}

static int coex_nb_fw_upload_change_timeout(void)
{
    int err = 0;
    bool first = true;
    uint8_t retry = COEX_NB_FW_UPLOAD_CHANGE_TIMEOUT_RETRY_COUNT;

    coex_nb_fw_upload_gen_crc32_table();

    while (true) {
        err = coex_nb_fw_upload_wait_for_hdr_sig();
        if (err)
            continue;

        if (s_fw_upload.version == COEX_NB_VER1) {
            return 0;
        } else if (s_fw_upload.version == COEX_NB_VER3) {
            err = coex_nb_fw_upload_wait_req(true);
            if (err)
                continue;
            if (s_fw_upload.length == 0)
                continue;
            if (s_fw_upload.error == 0) {
                if (first || (s_fw_upload.last_offset == s_fw_upload.offset)) {
                    coex_nb_fw_upload_send_ack(COEX_NB_V3_REQUEST_ACK);
                    coex_nb_fw_upload_write_data(coex_nb_cmd7_hdr_data,
                        s_fw_upload.length > COEX_NB_CMD_HDR_LEN ? COEX_NB_CMD_HDR_LEN : s_fw_upload.length);
                    s_fw_upload.last_offset = s_fw_upload.offset;
                    first = false;
                } else {
                    s_fw_upload.cmd7_change_timeout_len = COEX_NB_CMD_HDR_LEN;
                    s_fw_upload.wait_hdr_sig = false;
                    return 0;
                }
            } else {
                if (retry > 0) {
                    retry--;
                    coex_nb_fw_upload_send_ack(COEX_NB_V3_TIMEOUT_ACK);
                } else {
                    LOG_ERR("Fail to change timeout, err %d", s_fw_upload.error);
                    return -ENOTSUP;
                }
            }
        } else {
            LOG_ERR("Unsupported version %d", s_fw_upload.version);
            return -ENOTSUP;
        }
    }
    return -EINVAL;
}

typedef struct {
    uint32_t uartBaudRate;
    uint32_t uartDivisio;
    uint32_t uartClkDivisor;
} coex_nb_uart_baudrate_clkDiv_map_t;

static const coex_nb_uart_baudrate_clkDiv_map_t coex_nb_clk_div_map[] = {
    {115200U,  16U, 0x0075F6FDU},
    {1000000U,  2U, 0x00800000U},
    {3000000U,  1U, 0x00C00000U},
};

static int coex_nb_fw_upload_change_speed_config(struct coex_nb_change_speed_config *config, uint32_t speed)
{
    config->clk_div_addr      = 0x7f00008fU;
    config->uart_clk_div_addr = 0x7f000090U;
    config->mcr_addr          = 0x7f000091U;
    config->reinit_addr       = 0x7f000092U;
    config->icr_addr          = 0x7f000093U;
    config->fcr_addr          = 0x7f000094U;
    config->mcr_val           = 0x00000022U;
    config->reinit_val        = 0x00000001U;
    config->icr_val           = 0x000000c7U;
    config->fcr_val           = 0x000000c7U;

    for (int i = 0; i < ARRAY_SIZE(coex_nb_clk_div_map); i++) {
        if (speed == coex_nb_clk_div_map[i].uartBaudRate) {
            config->clk_div_val      = coex_nb_clk_div_map[i].uartClkDivisor;
            config->uart_clk_div_val = coex_nb_clk_div_map[i].uartDivisio;
            return 0;
        }
    }
    return -ENOTSUP;
}

static uint16_t coex_nb_fw_upload_wait_length(uint8_t flag)
{
    uint8_t buffer[4];
    uint16_t len, len_comp;
    int err;
    uint8_t ack;

    err = coex_nb_fw_upload_read_data(buffer, sizeof(buffer));
    if (err < 0)
        return 0;

    len      = sys_get_le16(buffer);
    len_comp = sys_get_le16(&buffer[2]);

    if ((len ^ len_comp) == 0xFFFF) {
        LOG_DBG("remote asks for %d bytes", len);
        if ((s_fw_upload.hdr_sig == COEX_NB_V1_HEADER_DATA_REQ) ||
            (s_fw_upload.hdr_sig == COEX_NB_V1_START_INDICATION)) {
            ack = COEX_NB_V1_REQUEST_ACK;
            coex_nb_fw_upload_write_data(&ack, 1);
            if (s_fw_upload.hdr_sig == COEX_NB_V1_START_INDICATION)
                return (uint16_t)COEX_NB_V1_START_INDICATION;
        }
    } else {
        LOG_ERR("len mismatch: %d vs %d", len, len_comp);
        ack = 0xbf;
        coex_nb_fw_upload_write_data(&ack, 1);
        if (flag)
            return (uint16_t)COEX_NB_V1_START_INDICATION;
        len = 0;
    }
    return len;
}

static uint32_t coex_nb_fw_upload_get_payload_length(uint8_t *cmd)
{
    return sys_get_le32(&cmd[8]);
}

static void coex_nb_fw_upload_get_hdr_start(uint8_t *buffer)
{
    int err;
    bool done = false;
    uint32_t count = 0;

    while (!done) {
        err = coex_nb_fw_upload_read_data(&s_fw_upload.hdr_sig, 1);
        if (err >= 0) {
            if (s_fw_upload.hdr_sig == COEX_NB_V1_HEADER_DATA_REQ) {
                buffer[count++] = s_fw_upload.hdr_sig;
                done = true;
                LOG_DBG("Found header %x", s_fw_upload.hdr_sig);
            }
        } else {
            LOG_ERR("Fail to read HDR sig %d", err);
            return;
        }
    }
    err = coex_nb_fw_upload_read_data(&buffer[count], 4);
    if (err < 0)
        LOG_ERR("Fail to read HDR payload %d", err);
}

static int coex_nb_fw_upload_len_valid(uint8_t *buffer, uint16_t *length)
{
    uint16_t len      = sys_get_le16(&buffer[1]);
    uint16_t len_comp = sys_get_le16(&buffer[3]);

    if ((len ^ len_comp) == 0xFFFFU) {
        *length = len;
        return 0;
    }
    return -EINVAL;
}

static int coex_nb_fw_upload_get_last_5bytes(uint8_t *buffer)
{
    int err;
    uint32_t payload_len;
    uint16_t len = 0;

    memset(s_fw_upload.last_5bytes_buffer, 0, sizeof(s_fw_upload.last_5bytes_buffer));
    coex_nb_fw_upload_get_hdr_start(s_fw_upload.last_5bytes_buffer);
    err = coex_nb_fw_upload_len_valid(s_fw_upload.last_5bytes_buffer, &len);
    if (err < 0) {
        LOG_ERR("Invalid HDR");
        return err;
    }

    payload_len = coex_nb_fw_upload_get_payload_length(buffer);
    if ((len == COEX_NB_CMD_HDR_LEN) || ((uint32_t)len == payload_len)) {
        s_fw_upload.is_error_case = false;
        return 0;
    }

    s_fw_upload.is_error_case = true;
    return -EINVAL;
}

static void coex_nb_fw_upload_update_result(uint32_t payload_len, uint16_t *sending_len,
                                    bool *first_chunk_sent)
{
    if (s_fw_upload.is_cmd7_req || s_fw_upload.is_entry_point_req) {
        *sending_len = COEX_NB_CMD_HDR_LEN;
        *first_chunk_sent = true;
    } else {
        *sending_len = payload_len;
        *first_chunk_sent = false;
        if (*sending_len == COEX_NB_CMD_HDR_LEN)
            s_fw_upload.is_hdr_data = true;
    }
}

static int coex_nb_fw_upload_write_hdr_and_payload(uint16_t len_to_send, uint8_t *buffer, bool new_speed)
{
    int err;
    uint32_t payload_len;
    bool send_done = false;
    uint16_t sending_len = COEX_NB_CMD_HDR_LEN;
    bool first_chunk_sent = false;

    payload_len = coex_nb_fw_upload_get_payload_length(buffer);

    while (!send_done) {
        if (sending_len == len_to_send) {
            if ((sending_len == COEX_NB_CMD_HDR_LEN) && (!s_fw_upload.is_hdr_data)) {
                if (!first_chunk_sent || (first_chunk_sent && s_fw_upload.is_error_case)) {
                    coex_nb_fw_upload_write_data(buffer, sending_len);
                    coex_nb_fw_upload_update_result(payload_len, &sending_len, &first_chunk_sent);
                } else {
                    send_done = true;
                    break;
                }
            } else {
                if (sending_len) {
                    coex_nb_fw_upload_write_data(&buffer[COEX_NB_CMD_HDR_LEN], sending_len);
                    first_chunk_sent = true;
                    sending_len = COEX_NB_CMD_HDR_LEN;
                    s_fw_upload.is_hdr_data = false;
                    if (new_speed)
                        return 0;
                } else {
                    return 0;
                }
            }
        } else {
            if ((len_to_send & 0x01) == 0x01) {
                if (len_to_send == (COEX_NB_CMD_HDR_LEN + 1)) {
                    coex_nb_fw_upload_write_data(buffer, len_to_send - 1);
                    sending_len = payload_len;
                    first_chunk_sent = false;
                } else if (len_to_send == (payload_len + 1)) {
                    coex_nb_fw_upload_write_data(&buffer[COEX_NB_CMD_HDR_LEN], len_to_send - 1);
                    sending_len = COEX_NB_CMD_HDR_LEN;
                    first_chunk_sent = true;
                }
            } else if (len_to_send == COEX_NB_CMD_HDR_LEN) {
                coex_nb_fw_upload_write_data(buffer, len_to_send);
                sending_len = payload_len;
                first_chunk_sent = false;
            } else if (len_to_send == payload_len) {
                coex_nb_fw_upload_write_data(&buffer[COEX_NB_CMD_HDR_LEN], len_to_send);
                sending_len = COEX_NB_CMD_HDR_LEN;
                first_chunk_sent = true;
            }
        }

        err = coex_nb_fw_upload_get_last_5bytes(buffer);
        if (err < 0) {
            LOG_ERR("Fail to get response");
            return err;
        }

        if (coex_nb_fw_upload_len_valid(s_fw_upload.last_5bytes_buffer, &len_to_send) == 0) {
            uint8_t ack = COEX_NB_V1_REQUEST_ACK;
            coex_nb_fw_upload_write_data(&ack, 1);
        }
    }
    return len_to_send;
}

static int coex_nb_fw_upload_change_speed(uint8_t hdr)
{
    int err;
    uint32_t hdr_len;
    bool load_payload = false;
    bool recovery = false;
    uint16_t len_to_send;
    uint32_t crc;

    err = coex_nb_fw_upload_change_speed_config(
        (struct coex_nb_change_speed_config *)s_fw_upload.change_speed_buffer,
        s_uart_dev_data.secondary_speed);
    if (err)
        return err;

    hdr_len = sizeof(s_fw_upload.change_speed_buffer);

    coex_nb_fw_upload_gen_crc32_table();
    crc = sys_cpu_to_le32(hdr_len);
    memcpy(coex_nb_cmd5_hdr_data + 8, &crc, 4);
    crc = coex_nb_fw_upload_update_crc32(0, (char *)coex_nb_cmd5_hdr_data, 12);
    crc = sys_cpu_to_be32(crc);
    memcpy(coex_nb_cmd5_hdr_data + 12, &crc, COEX_NB_CRC32_LEN);
    crc = coex_nb_fw_upload_update_crc32(0, (char *)s_fw_upload.change_speed_buffer,
                                 (int)sizeof(struct coex_nb_change_speed_config));
    crc = sys_cpu_to_be32(crc);
    memcpy(&s_fw_upload.change_speed_buffer[sizeof(struct coex_nb_change_speed_config)], &crc, COEX_NB_CRC32_LEN);

    while (true) {
        err = coex_nb_fw_upload_wait_for_hdr_sig();

        if (hdr && (err == 0)) {
            if (load_payload) {
                if (s_fw_upload.version == COEX_NB_VER3)
                    s_fw_upload.change_speed_buffer_len = COEX_NB_CMD_HDR_LEN + s_fw_upload.length;
                return 0;
            }
        } else {
            if (recovery)
                return -ETIME;
            if (load_payload) {
                LOG_ERR("HDR not received at secondary speed, recovering");
                err = coex_nb_fw_upload_uart_reconfig(s_uart_dev_data.primary_speed,
                                              s_uart_dev_data.primary_flowcontrol);
                if (err)
                    return err;
                load_payload = false;
                recovery = true;
                continue;
            }
        }

        if (s_fw_upload.version == COEX_NB_VER1) {
            len_to_send = coex_nb_fw_upload_wait_length(0);
            if (len_to_send == COEX_NB_V1_START_INDICATION)
                return -EINVAL;
            else if (len_to_send == 0)
                continue;
            else if (len_to_send == COEX_NB_CMD_HDR_LEN) {
                memcpy(s_fw_upload.send_buffer, coex_nb_cmd5_hdr_data, COEX_NB_CMD_HDR_LEN);
                memcpy(&s_fw_upload.send_buffer[COEX_NB_CMD_HDR_LEN],
                       s_fw_upload.change_speed_buffer, hdr_len);
                err = coex_nb_fw_upload_write_hdr_and_payload(len_to_send, s_fw_upload.send_buffer, true);
                if (err < 0)
                    return err;
                err = coex_nb_fw_upload_uart_reconfig(s_uart_dev_data.secondary_speed,
                                              s_uart_dev_data.secondary_flowcontrol);
                if (err)
                    return err;
                load_payload = true;
            } else {
                coex_nb_fw_upload_write_data(s_fw_upload.change_speed_buffer, hdr_len);
                err = coex_nb_fw_upload_uart_reconfig(s_uart_dev_data.secondary_speed,
                                              s_uart_dev_data.secondary_flowcontrol);
                if (err)
                    return err;
                load_payload = true;
            }
        } else if (s_fw_upload.version == COEX_NB_VER3) {
            err = coex_nb_fw_upload_wait_req(true);
            if (!(!hdr || (err == 0)))
                continue;
            if (s_fw_upload.length && (s_fw_upload.hdr_sig == COEX_NB_V3_HEADER_DATA_REQ)) {
                if (s_fw_upload.error != 0) {
                    coex_nb_fw_upload_send_ack(COEX_NB_V3_TIMEOUT_ACK);
                    continue;
                }
                coex_nb_fw_upload_send_ack(COEX_NB_V3_REQUEST_ACK);
                hdr = true;
                if (s_fw_upload.length == COEX_NB_CMD_HDR_LEN) {
                    coex_nb_fw_upload_write_data(coex_nb_cmd5_hdr_data, s_fw_upload.length);
                    s_fw_upload.last_offset = s_fw_upload.offset;
                } else {
                    coex_nb_fw_upload_write_data(s_fw_upload.change_speed_buffer, s_fw_upload.length);
                    err = coex_nb_fw_upload_uart_reconfig(s_uart_dev_data.secondary_speed,
                                                  s_uart_dev_data.secondary_flowcontrol);
                    if (err)
                        return err;
                    load_payload = true;
                }
            }
        }
    }
    return 0;
}

static int coex_nb_fw_upload_v1_send_data(uint16_t len)
{
    uint32_t cmd, data_len;
    int ret_len;

    memset(s_fw_upload.send_buffer, 0, sizeof(s_fw_upload.send_buffer));
    s_fw_upload.is_cmd7_req = false;
    s_fw_upload.is_entry_point_req = false;

    if ((s_fw_upload.fw_length - s_fw_upload.current_length) < len)
        len = s_fw_upload.fw_length - s_fw_upload.current_length;

    __ASSERT(sizeof(s_fw_upload.send_buffer) >= len,
             "V1: Out of sending buffer range (%u < %u)",
             sizeof(s_fw_upload.send_buffer), len);

    if (sizeof(s_fw_upload.send_buffer) < len) {
        LOG_ERR("V1: Out of sending buffer range (%u < %u)",
                sizeof(s_fw_upload.send_buffer), len);
        return -ENOMEM;
    }

    memcpy(s_fw_upload.send_buffer, s_fw_upload.fw + s_fw_upload.current_length, len);
    s_fw_upload.current_length += len;
    cmd = sys_get_le32(s_fw_upload.send_buffer);

    if (cmd == COEX_NB_CMD7) {
        s_fw_upload.is_cmd7_req = true;
        data_len = 0;
    } else {
        data_len = coex_nb_fw_upload_get_payload_length(s_fw_upload.send_buffer);
        if ((data_len > (sizeof(s_fw_upload.send_buffer) - len)) ||
            ((data_len + s_fw_upload.current_length) > s_fw_upload.fw_length)) {
            LOG_ERR("Invalid FW at %d/%d", s_fw_upload.current_length, s_fw_upload.fw_length);
            return -EINVAL;
        }
        memcpy(&s_fw_upload.send_buffer[len],
               s_fw_upload.fw + s_fw_upload.current_length, data_len);
        s_fw_upload.current_length += data_len;
        if ((s_fw_upload.current_length < s_fw_upload.fw_length) &&
            ((cmd == COEX_NB_CMD6) || (cmd == COEX_NB_CMD4)))
            s_fw_upload.is_entry_point_req = true;
    }

    ret_len = coex_nb_fw_upload_write_hdr_and_payload(len, s_fw_upload.send_buffer, false);
    LOG_DBG("FW upload %d/%d", s_fw_upload.current_length, s_fw_upload.fw_length);
    return ret_len;
}

static int coex_nb_fw_upload_v3_send_data(void)
{
    uint32_t start;

    LOG_DBG("Sending offset %d", s_fw_upload.offset);
    if (s_fw_upload.offset == s_fw_upload.last_offset) {
        LOG_WRN("Resending offset %d ...", s_fw_upload.offset);
        coex_nb_fw_upload_write_data(s_fw_upload.send_buffer, s_fw_upload.length);
        return s_fw_upload.length;
    }

    memset(s_fw_upload.send_buffer, 0, sizeof(s_fw_upload.send_buffer));
    start = s_fw_upload.offset - s_fw_upload.cmd7_change_timeout_len -
            s_fw_upload.change_speed_buffer_len;

    if (start >= s_fw_upload.fw_length) {
        LOG_ERR("Invalid fw offset");
        return -EINVAL;
    }

    if ((s_fw_upload.length + start) > s_fw_upload.fw_length)
        s_fw_upload.length = s_fw_upload.fw_length - start;

    __ASSERT(sizeof(s_fw_upload.send_buffer) >= s_fw_upload.length,
             "V3: Out of sending buffer range (%u < %u)",
             sizeof(s_fw_upload.send_buffer), s_fw_upload.length);

    if (sizeof(s_fw_upload.send_buffer) < s_fw_upload.length) {
        LOG_ERR("V3: Out of sending buffer range (%u < %u)",
                sizeof(s_fw_upload.send_buffer), s_fw_upload.length);
        return -ENOMEM;
    }

    memcpy(s_fw_upload.send_buffer, s_fw_upload.fw + start, s_fw_upload.length);
    s_fw_upload.current_length = start + s_fw_upload.length;
    coex_nb_fw_upload_write_data(s_fw_upload.send_buffer, s_fw_upload.length);
    s_fw_upload.last_offset = s_fw_upload.offset;
    return s_fw_upload.length;
}

static int coex_nb_fw_uploading(const uint8_t *fw, uint32_t fw_length)
{
    int err;
    bool secondary_speed = false;
    uint16_t len_to_send;

    s_fw_upload.wait_hdr_sig      = true;
    s_fw_upload.is_hdr_data       = false;
    s_fw_upload.is_error_case     = false;
    s_fw_upload.is_cmd7_req       = false;
    s_fw_upload.is_entry_point_req = false;
    s_fw_upload.last_offset       = 0xFFFFU;

    err = coex_nb_fw_upload_change_timeout();
    LOG_DBG("Change timeout hdr flag %d (err %d)", s_fw_upload.wait_hdr_sig, err);
    if (err)
        return err;

    coex_nb_fw_upload_read_to_clear();

    if (s_uart_dev_data.secondary_speed &&
        (s_uart_dev_data.secondary_speed != s_uart_dev_data.primary_speed)) {
        LOG_DBG("Change speed to %d", s_uart_dev_data.secondary_speed);
        err = coex_nb_fw_upload_change_speed(s_fw_upload.wait_hdr_sig);
        if (err != 0) {
            LOG_ERR("Fail to change speed");
            return err;
        }
        secondary_speed = true;
    }

    s_fw_upload.fw_length      = fw_length;
    s_fw_upload.current_length = 0;
    s_fw_upload.fw             = fw;

    while (true) {
        err = coex_nb_fw_upload_wait_for_hdr_sig();
        if (secondary_speed && (err != 0))
            return -ETIME;

        secondary_speed = false;

        if (s_fw_upload.version == COEX_NB_VER1) {
            len_to_send = coex_nb_fw_upload_wait_length(true);
            if (len_to_send == COEX_NB_V1_START_INDICATION)
                continue;
            while (len_to_send > 0)
            {
                err = coex_nb_fw_upload_v1_send_data(len_to_send);
                if (err < 0)
                {
                    LOG_ERR("FW V1 upload failed: %d", err);
                    return err;
                }
                len_to_send = (uint16_t)err;
            }
            if (s_fw_upload.current_length >= s_fw_upload.fw_length) {
                LOG_DBG("FW download done");
                return 0;
            }
            LOG_ERR("FW download failed");
            return len_to_send;
        } else if (s_fw_upload.version == COEX_NB_VER3) {
            if (s_fw_upload.hdr_sig == COEX_NB_V3_START_INDICATION) {
                coex_nb_fw_upload_wait_req(false);
                continue;
            }
            err = coex_nb_fw_upload_wait_req(false);
            if (err) {
                LOG_ERR("Fail to wait req");
                return err;
            }
            if (s_fw_upload.length) {
                if (s_fw_upload.error == 0) {
                    coex_nb_fw_upload_send_ack(COEX_NB_V3_REQUEST_ACK);
                    err = coex_nb_fw_upload_v3_send_data();
                    if (err < 0) {
                        LOG_ERR("FW download failed");
                        return err;
                    }
                } else {
                    LOG_DBG("Error occurs %d", s_fw_upload.error);
                    coex_nb_fw_upload_send_ack(COEX_NB_V3_TIMEOUT_ACK);
                    if (s_fw_upload.error & COEX_NB_BT_MIC_FAIL_BIT) {
                        s_fw_upload.change_speed_buffer_len = 0;
                        s_fw_upload.current_length = 0;
                        s_fw_upload.last_offset = 0;
                    }
                }
            } else {
                if (s_fw_upload.error == 0) {
                    coex_nb_fw_upload_send_ack(COEX_NB_V3_REQUEST_ACK);
                    LOG_DBG("FW download done");
                    return 0;
                }
                LOG_DBG("Error occurs %d", s_fw_upload.error);
                coex_nb_fw_upload_send_ack(COEX_NB_V3_TIMEOUT_ACK);
                if (s_fw_upload.error & COEX_NB_BT_MIC_FAIL_BIT) {
                    s_fw_upload.change_speed_buffer_len = 0;
                    s_fw_upload.current_length = 0;
                    s_fw_upload.last_offset = 0;
                }
            }
        } else {
            return -ENOTSUP;
        }
    }
    return -EINVAL;
}

/*******************************************************************************
 * Public API
 ******************************************************************************/

int coex_nb_uart_fw_download(const uint8_t *fw, uint32_t fw_length)
{
    int err;
    uint32_t hci_speed;
    bool hci_flowcontrol;

    if ((fw == NULL) || (fw_length == 0U)) {
        return -EINVAL;
    }

    if (!device_is_ready(coex_nb_uart_dev)) {
        LOG_ERR("UART device not ready");
        return -ENODEV;
    }

    hci_speed = DT_PROP(DT_INST_GPARENT(0), current_speed);

    s_uart_dev_data.primary_speed         = DT_PROP_OR(DT_DRV_INST(0), fw_download_primary_speed, hci_speed);
    s_uart_dev_data.secondary_speed       = DT_PROP_OR(DT_DRV_INST(0), fw_download_secondary_speed, hci_speed);
    hci_flowcontrol                        = (bool)DT_PROP_OR(DT_DRV_INST(0), hw_flow_control, false);
    s_uart_dev_data.primary_flowcontrol   = (bool)DT_PROP_OR(DT_DRV_INST(0), fw_download_primary_flowcontrol, false);
    s_uart_dev_data.secondary_flowcontrol = (bool)DT_PROP_OR(DT_DRV_INST(0), fw_download_secondary_flowcontrol, false);

    /* Initialize state */
    memset(&s_fw_upload, 0, sizeof(s_fw_upload));
    s_made_table = false;
    s_uart_initialized = false;
    k_sem_init(&s_fw_upload.rx.sem, 0, sizeof(s_fw_upload.rx.buffer));

    /* Configure UART at primary (download) speed */
    err = coex_nb_fw_upload_uart_reconfig(s_uart_dev_data.primary_speed,
                                   s_uart_dev_data.primary_flowcontrol);
    if (err) {
        LOG_ERR("Failed to configure UART for FW download");
        return err;
    }

#if CONFIG_COEX_NB_BSP_TRIGGER
    /* Wake narrow band CPU from boot-sleep before FW download */
    err = coex_nb_fw_upload_wake_bt_from_bootsleep();
    if (err) {
        LOG_ERR("Failed to send BSP trigger");
        return err;
    }
#endif /* CONFIG_COEX_NB_BSP_TRIGGER */

    /* Download firmware */
    err = coex_nb_fw_uploading(fw, fw_length);
    if (err) {
        HAL_UartAbortReceive((hal_uart_handle_t)hci_uart_handle);
        LOG_ERR("FW upload failed: %d", err);
        return err;
    }

    /* Reconfigure UART to HCI operation speed */
    err = coex_nb_fw_upload_uart_reconfig(hci_speed, hci_flowcontrol);
    if (err) {
        LOG_ERR("Failed to reconfigure UART to HCI speed");
        return err;
    }

    k_sleep(K_MSEC(CONFIG_BT_H4_NXP_CTLR_WAIT_TIME_AFTER_UPLOAD));

    LOG_INF("Narrow band firmware downloaded successfully via UART");
    return 0;
}
