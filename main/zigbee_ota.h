#pragma once

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Static identifiers and helpers for Zigbee OTA.
 *
 * These values will be used by the future Zigbee OTA client to
 * match firmware images provided by the coordinator (e.g. Zigbee2MQTT).
 *
 * For now they are only logged by the OTA skeleton and do not affect
 * runtime behaviour.
 */

/* Manufacturer identifier used in Zigbee OTA headers (private range). */
#define ZB_OTA_MANUFACTURER_ID   0x1234

/* Image type identifier for this device family. */
#define ZB_OTA_IMAGE_TYPE        0x0002

/*
 * Convert semantic version (major.minor.patch) into a monotonic
 * integer used as Zigbee OTA file version.
 */
#define ZB_OTA_VERSION_FROM_SEMVER(major, minor, patch) \
    (((major) * 10000) + ((minor) * 100) + (patch))

/*
 * Current firmware file version for OTA purposes.
 *
 * NOTE: Keep this in sync with the application version in
 * CMakeLists.txt and main.c when releasing new firmware.
 * For the 1.3.0 firmware this evaluates to 10300.
 */
#define ZB_OTA_FIRMWARE_VERSION_MAJOR 2
#define ZB_OTA_FIRMWARE_VERSION_MINOR 0
#define ZB_OTA_FIRMWARE_VERSION_PATCH 0

#define ZB_OTA_CURRENT_FILE_VERSION \
    ZB_OTA_VERSION_FROM_SEMVER(      \
        ZB_OTA_FIRMWARE_VERSION_MAJOR, \
        ZB_OTA_FIRMWARE_VERSION_MINOR, \
        ZB_OTA_FIRMWARE_VERSION_PATCH)

/**
 * @brief Initialize Zigbee OTA skeleton.
 *
 * In phase 0 this only sets up internal state and logs the chosen
 * identifiers. It does not modify flash or partitions and does not
 * start any OTA transfer.
 */
esp_err_t zigbee_ota_init(void);

/**
 * @brief Begin writing a new OTA image into the inactive OTA partition.
 *
 * This prepares the esp_ota context but does not switch the running image.
 *
 * @param expected_version   File version advertised by the OTA server.
 * @param expected_size      Expected total image size in bytes (0 if unknown).
 * @return esp_err_t         ESP_OK on success, error code otherwise.
 */
esp_err_t zigbee_ota_begin(uint32_t expected_version, size_t expected_size);

/**
 * @brief Write a contiguous block of OTA image data.
 *
 * The offset is relative to the start of the OTA payload. The helper will
 * ensure writes are sequential; out-of-order offsets are rejected.
 *
 * @param offset  Offset of this block within the image.
 * @param data    Pointer to block data.
 * @param length  Block size in bytes.
 * @return esp_err_t ESP_OK on success, error code otherwise.
 */
esp_err_t zigbee_ota_write_block(size_t offset, const uint8_t *data, size_t length);

/**
 * @brief Finalize OTA image and optionally switch boot partition.
 *
 * When @p success is true, the function validates the written size against
 * the expected size (if provided), ends the OTA session and marks the new
 * partition as bootable. It does not restart the device.
 *
 * When @p success is false, the function aborts the OTA session and
 * discards partially written data.
 *
 * @param success  true if the download completed successfully.
 * @return esp_err_t ESP_OK on success, error code otherwise.
 */
esp_err_t zigbee_ota_finish(bool success);

#ifdef __cplusplus
}
#endif
