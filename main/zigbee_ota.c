#include "zigbee_ota.h"

#include "esp_log.h"
#include "esp_ota_ops.h"

static const char *TAG_OTA = "ZB_OTA";

/**
 * @brief Internal Zigbee OTA client state.
 */
typedef enum {
    ZB_OTA_STATE_IDLE = 0,
    ZB_OTA_STATE_DOWNLOAD,
    ZB_OTA_STATE_VERIFY,
} zigbee_ota_state_t;

typedef struct {
    const esp_partition_t *target_partition;
    esp_ota_handle_t ota_handle;
    size_t expected_size;
    size_t written_size;
    uint32_t expected_version;
} zigbee_ota_context_t;

static zigbee_ota_state_t s_ota_state = ZB_OTA_STATE_IDLE;
static zigbee_ota_context_t s_ota_ctx = {0};

esp_err_t zigbee_ota_init(void)
{
    s_ota_state = ZB_OTA_STATE_IDLE;
    s_ota_ctx.target_partition = NULL;
    s_ota_ctx.ota_handle = 0;
    s_ota_ctx.expected_size = 0;
    s_ota_ctx.written_size = 0;
    s_ota_ctx.expected_version = 0;

    ESP_LOGI(TAG_OTA,
             "Zigbee OTA initialized (manufacturerId=0x%04X, imageType=0x%04X, fileVersion=%u)",
             (unsigned)ZB_OTA_MANUFACTURER_ID,
             (unsigned)ZB_OTA_IMAGE_TYPE,
             (unsigned)ZB_OTA_CURRENT_FILE_VERSION);

    return ESP_OK;
}

esp_err_t zigbee_ota_begin(uint32_t expected_version, size_t expected_size)
{
    if (s_ota_state != ZB_OTA_STATE_IDLE) {
        ESP_LOGW(TAG_OTA, "Cannot start OTA: state=%d", (int)s_ota_state);
        return ESP_ERR_INVALID_STATE;
    }

    const esp_partition_t *running = esp_ota_get_running_partition();
    const esp_partition_t *ota_0 = esp_ota_get_next_update_partition(NULL);

    if (!running || !ota_0) {
        ESP_LOGE(TAG_OTA, "OTA partitions not configured correctly (running=%p, ota_0=%p)",
                 (const void *)running, (const void *)ota_0);
        return ESP_ERR_NOT_FOUND;
    }

    s_ota_ctx.target_partition = ota_0;
    s_ota_ctx.expected_size = expected_size;
    s_ota_ctx.written_size = 0;
    s_ota_ctx.expected_version = expected_version;

    esp_err_t err = esp_ota_begin(s_ota_ctx.target_partition, expected_size, &s_ota_ctx.ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG_OTA, "esp_ota_begin failed: %s", esp_err_to_name(err));
        s_ota_ctx.target_partition = NULL;
        s_ota_ctx.ota_handle = 0;
        return err;
    }

    s_ota_state = ZB_OTA_STATE_DOWNLOAD;
    ESP_LOGI(TAG_OTA,
             "OTA begin: target label='%s', addr=0x%lx, size=%lu, expected_version=%u",
             s_ota_ctx.target_partition->label,
             (unsigned long)s_ota_ctx.target_partition->address,
             (unsigned long)s_ota_ctx.target_partition->size,
             (unsigned)expected_version);

    return ESP_OK;
}

esp_err_t zigbee_ota_write_block(size_t offset, const uint8_t *data, size_t length)
{
    if (s_ota_state != ZB_OTA_STATE_DOWNLOAD || !s_ota_ctx.target_partition) {
        ESP_LOGW(TAG_OTA, "OTA write in invalid state=%d", (int)s_ota_state);
        return ESP_ERR_INVALID_STATE;
    }
    if (!data || length == 0U) {
        return ESP_OK;
    }

    if (offset != s_ota_ctx.written_size) {
        ESP_LOGW(TAG_OTA, "Out-of-order OTA block: expected offset=%lu, got=%lu", (unsigned long)s_ota_ctx.written_size, (unsigned long)offset);
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = esp_ota_write(s_ota_ctx.ota_handle, data, length);
    if (err != ESP_OK) {
        ESP_LOGE(TAG_OTA, "esp_ota_write failed at offset=%lu len=%lu: %s",
                 (unsigned long)offset, (unsigned long)length, esp_err_to_name(err));
        return err;
    }

    s_ota_ctx.written_size += length;
    return ESP_OK;
}

esp_err_t zigbee_ota_finish(bool success)
{
    if (s_ota_state != ZB_OTA_STATE_DOWNLOAD || !s_ota_ctx.target_partition) {
        ESP_LOGW(TAG_OTA, "OTA finish in invalid state=%d", (int)s_ota_state);
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = ESP_OK;

    if (!success) {
        ESP_LOGW(TAG_OTA, "OTA cancelled after %lu bytes", (unsigned long)s_ota_ctx.written_size);
        err = esp_ota_end(s_ota_ctx.ota_handle);
        s_ota_state = ZB_OTA_STATE_IDLE;
        s_ota_ctx.target_partition = NULL;
        s_ota_ctx.ota_handle = 0;
        s_ota_ctx.expected_size = 0;
        s_ota_ctx.written_size = 0;
        s_ota_ctx.expected_version = 0;
        return err;
    }

    if (s_ota_ctx.expected_size > 0U && s_ota_ctx.written_size != s_ota_ctx.expected_size) {
        ESP_LOGE(TAG_OTA, "OTA size mismatch: expected=%lu, written=%lu",
                 (unsigned long)s_ota_ctx.expected_size, (unsigned long)s_ota_ctx.written_size);
        err = esp_ota_end(s_ota_ctx.ota_handle);
        s_ota_state = ZB_OTA_STATE_IDLE;
        s_ota_ctx.target_partition = NULL;
        s_ota_ctx.ota_handle = 0;
        s_ota_ctx.expected_size = 0;
        s_ota_ctx.written_size = 0;
        s_ota_ctx.expected_version = 0;
        return ESP_ERR_INVALID_SIZE;
    }

    err = esp_ota_end(s_ota_ctx.ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG_OTA, "esp_ota_end failed: %s", esp_err_to_name(err));
        s_ota_state = ZB_OTA_STATE_IDLE;
        s_ota_ctx.target_partition = NULL;
        s_ota_ctx.ota_handle = 0;
        s_ota_ctx.expected_size = 0;
        s_ota_ctx.written_size = 0;
        s_ota_ctx.expected_version = 0;
        return err;
    }

    err = esp_ota_set_boot_partition(s_ota_ctx.target_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG_OTA, "esp_ota_set_boot_partition failed: %s", esp_err_to_name(err));
        s_ota_state = ZB_OTA_STATE_IDLE;
        s_ota_ctx.target_partition = NULL;
        s_ota_ctx.ota_handle = 0;
        s_ota_ctx.expected_size = 0;
        s_ota_ctx.written_size = 0;
        s_ota_ctx.expected_version = 0;
        return err;
    }

    ESP_LOGI(TAG_OTA,
             "OTA finished successfully: written=%lu bytes, new partition='%s' (addr=0x%lx). Reboot required to apply.",
             (unsigned long)s_ota_ctx.written_size,
             s_ota_ctx.target_partition->label,
             (unsigned long)s_ota_ctx.target_partition->address);

    s_ota_state = ZB_OTA_STATE_VERIFY;

    /* Reset context back to idle; the new image will be used after reboot. */
    s_ota_state = ZB_OTA_STATE_IDLE;
    s_ota_ctx.target_partition = NULL;
    s_ota_ctx.ota_handle = 0;
    s_ota_ctx.expected_size = 0;
    s_ota_ctx.written_size = 0;
    s_ota_ctx.expected_version = 0;

    return ESP_OK;
}
